/*
 * barcode_scanner.c
 * 扫码枪通信模块
 * 作者: sh Assistant
 * 日期: 2024
 * 版本: V1.0
 */

#include "barcode_scanner.h"
#include "delay.h"
#include <string.h>

// Remove wiring.h dependency and implement millis() directly
extern __IO uint32_t TimingMillis; // Reference to global timing variable from wiring.c

static uint32_t millis(void) {
    return TimingMillis;
}

// 全局变量定义
uint8_t Serial_RxPacket[BARCODE_PACKET_SIZE];
static uint8_t barcode_rx_buffer[BARCODE_PACKET_SIZE];
static uint8_t barcode_rx_index = 0;
static uint8_t barcode_rx_flag = 0;
static uint32_t barcode_rx_timeout = 0;

BarcodeState_t barcode_state = BARCODE_IDLE;
BarcodePacket_t barcode_packet;

// 扫码枪初始化
void BarcodeScanner_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 使能USART2和GPIOA时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // USART2 GPIO配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;  // PA2 USART2_TX
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; // PA3 USART2_RX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // USART配置
    USART_InitStructure.USART_BaudRate = BARCODE_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &USART_InitStructure);
    
    // 中断配置
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 使能USART2
    USART_Cmd(USART2, ENABLE);
    
    // 初始化全局变量
    barcode_rx_index = 0;
    barcode_rx_flag = 0;
    barcode_state = BARCODE_IDLE;
    memset(Serial_RxPacket, 0, sizeof(Serial_RxPacket));
    memset(barcode_packet.data, 0, sizeof(barcode_packet.data));
    barcode_packet.length = 0;
    barcode_packet.valid = 0;
    barcode_packet.timestamp = 0;
}

// 获取接收标志
uint8_t Serial_GetRxFlag_2(void)
{
    return barcode_rx_flag;
}

// 获取接收标志（别名）
uint8_t BarcodeScanner_GetFlag(void)
{
    return barcode_rx_flag;
}

// 清除接收标志
void BarcodeScanner_ClearFlag(void)
{
    barcode_rx_flag = 0;
    barcode_state = BARCODE_IDLE;
}

// 获取扫码数据
uint8_t BarcodeScanner_GetData(uint8_t *data, uint8_t max_len)
{
    if (barcode_rx_flag && data) {
        uint8_t len = (barcode_rx_index > max_len) ? max_len : barcode_rx_index;
        memcpy(data, barcode_rx_buffer, len);
        return len;
    }
    return 0;
}

// 处理扫码数据
void BarcodeScanner_ProcessData(void)
{
    if (barcode_rx_flag) {
        // 复制数据到全局包
        memcpy(barcode_packet.data, barcode_rx_buffer, barcode_rx_index);
        barcode_packet.length = barcode_rx_index;
        barcode_packet.valid = 1;
        barcode_packet.timestamp = millis();
        
        // 复制到Serial_RxPacket（兼容原有代码）
        memcpy(Serial_RxPacket, barcode_rx_buffer, barcode_rx_index);
        
        barcode_state = BARCODE_COMPLETE;
    }
}

// 解析动作码
int BarcodeScanner_ParseActionCode(uint8_t *data, uint8_t length)
{
    if (!data || length < 3) {
        return -1;  // 无效数据
    }
    
    // 根据扫码枪数据格式解析动作码
    // 假设数据格式：起始符 + 数据 + 结束符
    // 这里根据实际扫码枪协议调整
    
    // 查找数据部分（跳过可能的起始符）
    uint8_t data_start = 0;
    for (uint8_t i = 0; i < length - 1; i++) {
        if (data[i] >= '1' && data[i] <= '9') {
            data_start = i;
            break;
        }
    }
    
    if (data_start < length) {
        // 提取数字字符并转换为动作码
        uint8_t action_char = data[data_start];
        
        // 根据ASCII码转换为数字
        if (action_char >= '1' && action_char <= '9') {
            return (action_char - '0');
        }
    }
    
    return -1;  // 无法解析
}

// USART2中断处理函数
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        uint8_t received_data = USART_ReceiveData(USART2);
        
        // 清除溢出错误标志
        USART_ClearITPendingBit(USART2, USART_IT_ORE);
        
        // 处理接收到的数据
        if (barcode_state == BARCODE_IDLE) {
            barcode_state = BARCODE_RECEIVING;
            barcode_rx_index = 0;
            barcode_rx_timeout = millis();
        }
        
        // 检查缓冲区是否已满
        if (barcode_rx_index < BARCODE_PACKET_SIZE - 1) {
            barcode_rx_buffer[barcode_rx_index++] = received_data;
            
            // 检查是否接收到完整数据包
            // 扫码枪通常以回车符(0x0D)或换行符(0x0A)结束
            if (received_data == 0x0D || received_data == 0x0A) {
                barcode_rx_buffer[barcode_rx_index] = '\0';  // 添加字符串结束符
                barcode_rx_flag = 1;
                barcode_state = BARCODE_COMPLETE;
                
                // 处理数据
                BarcodeScanner_ProcessData();
            }
        } else {
            // 缓冲区满，重置
            barcode_rx_index = 0;
            barcode_state = BARCODE_IDLE;
        }
    }
}

// 检查接收超时
void BarcodeScanner_CheckTimeout(void)
{
    if (barcode_state == BARCODE_RECEIVING) {
        if ((millis() - barcode_rx_timeout) > BARCODE_TIMEOUT) {
            barcode_state = BARCODE_TIMEOUT_ERROR;
            barcode_rx_index = 0;
            // 可以选择清除标志或保持错误状态
        }
    }
}
