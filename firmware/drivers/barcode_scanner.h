#ifndef __BARCODE_SCANNER_H
#define __BARCODE_SCANNER_H

#include "stm32f10x.h"

// 扫码枪通信相关定义
#define BARCODE_UART USART2          // 扫码枪使用USART2
#define BARCODE_BAUDRATE 115200    // 扫码枪波特率
#define BARCODE_PACKET_SIZE 32       // 数据包最大长度
#define BARCODE_TIMEOUT 1000         // 接收超时时间(ms)

// 扫码枪数据包结构
typedef struct {
    uint8_t data[BARCODE_PACKET_SIZE];
    uint8_t length;
    uint8_t valid;
    uint32_t timestamp;
} BarcodePacket_t;

// 扫码枪状态
typedef enum {
    BARCODE_IDLE = 0,
    BARCODE_RECEIVING,
    BARCODE_COMPLETE,
    BARCODE_TIMEOUT_ERROR
} BarcodeState_t;

// 全局变量声明
extern uint8_t Serial_RxPacket[BARCODE_PACKET_SIZE];
extern uint8_t Serial_GetRxFlag_2(void);
extern BarcodeState_t barcode_state;
extern BarcodePacket_t barcode_packet;

// 函数声明
void BarcodeScanner_Init(void);
uint8_t BarcodeScanner_GetFlag(void);
void BarcodeScanner_ClearFlag(void);
uint8_t BarcodeScanner_GetData(uint8_t *data, uint8_t max_len);
void BarcodeScanner_ProcessData(void);
int BarcodeScanner_ParseActionCode(uint8_t *data, uint8_t length);

// 中断处理函数
void USART2_IRQHandler(void);

#endif /* __BARCODE_SCANNER_H */
