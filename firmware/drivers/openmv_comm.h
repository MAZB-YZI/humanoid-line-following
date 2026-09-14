  #ifndef __OPENMV_COMM_H
#define __OPENMV_COMM_H

#include "stm32f10x.h"

// OpenMV通信相关定义
#define OPENMV_UART USART1           // OpenMV使用USART1
#define OPENMV_BAUDRATE 19200       // OpenMV波特率
#define OPENMV_PACKET_SIZE 64        // 数据包最大长度
#define OPENMV_TIMEOUT 100           // 接收超时时间(ms)

// OpenMV数据包格式定义
#define OPENMV_PACKET_START '{'      // 数据包起始符
#define OPENMV_PACKET_END '}'        // 数据包结束符
#define OPENMV_CODE_SEPARATOR '<'    // 动作码分隔符
#define OPENMV_END_SEPARATOR '&'     // 数据结束符

// OpenMV通信状态
typedef enum {
    OPENMV_IDLE = 0,
    OPENMV_RECEIVING,
    OPENMV_COMPLETE,
    OPENMV_ERROR,
    //OPENMV_TIMEOUT
} OpenMVState_t;

// OpenMV数据包结构
typedef struct {
    uint8_t data[OPENMV_PACKET_SIZE];
    uint8_t length;
    uint8_t valid;
    int32_t action_code;
    uint32_t timestamp;
} OpenMVPacket_t;

// 全局变量声明
extern OpenMVState_t openmv_state;
extern OpenMVPacket_t openmv_packet;
extern volatile uint8_t openmv_rx_buffer[OPENMV_PACKET_SIZE];
extern volatile uint8_t openmv_rx_index;
extern volatile uint8_t openmv_rx_flag;

// 函数声明
void OpenMV_Init(void);
uint8_t OpenMV_GetFlag(void);
void OpenMV_ClearFlag(void);
int OpenMV_GetActionCode(void);
int OpenMV_ReceiveDataNonBlocking(void);
void OpenMV_ProcessData(void);
int OpenMV_ParsePacket(uint8_t *data, uint8_t length);

// 中断处理函数
void USART1_IRQHandler(void);

// 兼容性函数（对应原有代码中的函数）
static inline int receive_openmv_code_nonblocking(void)
{
    return OpenMV_ReceiveDataNonBlocking();
}

#endif /* __OPENMV_COMM_H */
