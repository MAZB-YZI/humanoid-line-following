/*
 * openmv_comm.c
 * OpenMV通信模块
 * 作者: AI Assistant
 * 日期: 2024
 * 版本: V1.0
 */

#include "openmv_comm.h"
#include "delay.h"
#include <string.h>

// Remove wiring.h dependency and implement millis() directly
extern __IO uint32_t TimingMillis; // Reference to global timing variable from wiring.c

static uint32_t millis(void) {
    return TimingMillis;
}

// 全局变量定义
OpenMVState_t openmv_state = OPENMV_IDLE;
OpenMVPacket_t openmv_packet;
volatile uint8_t openmv_rx_buffer[OPENMV_PACKET_SIZE];
volatile uint8_t openmv_rx_index = 0;
volatile uint8_t openmv_rx_flag = 0;

static uint32_t openmv_rx_timeout = 0;
// OpenMV初始化
void OpenMV_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 使能USART1和GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    
    // USART1 GPIO配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;  // PA9 USART1_TX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;  // PA10 USART1_RX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // USART配置
    USART_InitStructure.USART_BaudRate = OPENMV_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);
    
    // 中断配置
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 使能USART1
    USART_Cmd(USART1, ENABLE);
    
    // 初始化全局变量
    openmv_rx_index = 0;
    openmv_rx_flag = 0;
    openmv_state = OPENMV_IDLE;
    memset(openmv_packet.data, 0, sizeof(openmv_packet.data));
    openmv_packet.length = 0;
    openmv_packet.valid = 0;
    openmv_packet.action_code = -1;
    openmv_packet.timestamp = 0;
}

// 获取接收标志
uint8_t OpenMV_GetFlag(void)
{
    return openmv_rx_flag;
}

// 清除接收标志
void OpenMV_ClearFlag(void)
{
    openmv_rx_flag = 0;
    openmv_state = OPENMV_IDLE;
}

// 非阻塞接收OpenMV数据
int OpenMV_ReceiveDataNonBlocking(void)
{
    if (openmv_rx_flag) {
        // 解析数据包
        int action_code = OpenMV_ParsePacket((uint8_t*)openmv_rx_buffer, openmv_rx_index);
        openmv_packet.action_code = action_code;
        
        // 清除标志
        OpenMV_ClearFlag();
        
        return action_code;
    }
    
    return -1;  // 没有接收到数据
}

// 获取动作码
int OpenMV_GetActionCode(void)
{
    return openmv_packet.action_code;
}

// 处理接收到的数据
void OpenMV_ProcessData(void)
{
    if (openmv_rx_flag) {
        // 复制数据到包结构
        memcpy(openmv_packet.data, (uint8_t*)openmv_rx_buffer, openmv_rx_index);
        openmv_packet.length = openmv_rx_index;
        openmv_packet.valid = 1;
        openmv_packet.timestamp = millis();
        
        // 解析动作码
        openmv_packet.action_code = OpenMV_ParsePacket((uint8_t*)openmv_rx_buffer, openmv_rx_index);
        
        openmv_state = OPENMV_COMPLETE;
    }
}

// 解析OpenMV数据包
int OpenMV_ParsePacket(uint8_t *data, uint8_t length)
{
    if (!data || length < 3) {
        return -1;  // 无效数据
    }
    
    // 查找数据包起始符
    uint8_t start_pos = 0;
    uint8_t end_pos = 0;
    uint8_t code_start = 0;
    
    for (uint8_t i = 0; i < length; i++) {
        if (data[i] == OPENMV_PACKET_START) {
            start_pos = i;
        } else if (data[i] == OPENMV_PACKET_END) {
            end_pos = i;
            break;
        } else if (data[i] == OPENMV_CODE_SEPARATOR && code_start == 0) {
            code_start = i + 1;
        }
    }
    
    // 检查数据包格式
    if (start_pos < end_pos && code_start > start_pos && code_start < end_pos) {
        // 查找动作码结束符
        uint8_t code_end = code_start;
        for (uint8_t i = code_start; i < end_pos; i++) {
            if (data[i] == OPENMV_END_SEPARATOR) {
                code_end = i;
                break;
            }
        }
        
        if (code_end > code_start) {
            // 提取动作码
            char code_str[16] = {0};
            uint8_t code_len = code_end - code_start;
            if (code_len < sizeof(code_str)) {
                memcpy(code_str, &data[code_start], code_len);
                
                // 转换为整数
                int action_code = 0;
                for (uint8_t i = 0; i < code_len; i++) {
                    if (code_str[i] >= '0' && code_str[i] <= '9') {
                        action_code = action_code * 10 + (code_str[i] - '0');
                    } else {
                        return -1;  // 非数字字符
                    }
                }
                
                return action_code;
            }
        }
    }
    
    return -1;  // 解析失败
}





// 发送数据到OpenMV
void OpenMV_SendData(uint8_t *data, uint8_t length)
{
    for (uint8_t i = 0; i < length; i++) {
			USART_SendData(USART1, data[i]);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
     //   USART_SendData(USART1, data[i]);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}
uint8_t Serial_RxPacket1[3];
char Serial_RxData_3;
uint8_t Serial_RxFlag_3;
char Serial_RxPacket_3[1];
void USART1_IRQHandler(void)
{
  static uint8_t RxState_3 = 0;		//定义表示当前状态机状态的静态变量
	static uint8_t pRxPacket_3 = 0;	//定义表示当前接收数据位置的静态变量
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)		//判断是否是USART1的接收事件触发的中断
	{
		uint8_t RxData = USART_ReceiveData(USART1);				//读取数据寄存器，存放在接收的数据变量
		
		/*使用状态机的思路，依次处理数据包的不同部分*/
		
		/*当前状态为0，接收数据包包头*/
		if (RxState_3 == 0)
		{
			if (RxData =='{'&&Serial_RxFlag_3==0)			//如果数据确实是包头
			{
				RxState_3 = 1;			//置下一个状态
				pRxPacket_3 = 0;			//数据包的位置归零
			}
		}
		/*当前状态为1，接收数据包数据*/
		else if (RxState_3 == 1)
		{
			Serial_RxPacket1[pRxPacket_3] = RxData;	//将数据存入数据包数组的指定位置
			pRxPacket_3 ++;				//数据包的位置自增
			if (pRxPacket_3 >= 1)			//如果收够1个数据
			{
				RxState_3 = 2;			//置下一个状态
			}
		}
		/*当前状态为2，接收数据包包尾*/
		else if (RxState_3 == 2)
		{
			if (RxData == '}')			//如果数据确实是包尾部
			{
				RxState_3 = 0;			//状态归0
				Serial_RxFlag_3 = 1;		//接收数据包标志位置1，成功接收一个数据包
			}
		}
		
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);		//清除标志位
	}
}

uint8_t Serial_GetRxFlag_3(void)
{
	if (Serial_RxFlag_3 == 1)
	{
		Serial_RxFlag_3 = 0;
		return 1;
	}
	return 0;
}