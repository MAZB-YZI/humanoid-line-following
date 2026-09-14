#ifndef __COMM_MANAGER_H
#define __COMM_MANAGER_H

#include "stm32f10x.h"
#include "barcode_scanner.h"
#include "openmv_comm.h"

// 通信优先级定义
typedef enum {
    COMM_PRIORITY_GRAYSCALE = 0,    // 灰度传感器优先级最低
    COMM_PRIORITY_BARCODE = 1,      // 扫码枪优先级中等
    COMM_PRIORITY_OPENMV = 2        // OpenMV优先级最高
} CommPriority_t;

// 动作码定义
typedef enum {
    ACTION_NONE = -1,
    ACTION_FORWARD = 1,
    ACTION_LEFT_SMALL = 2,
    ACTION_RIGHT_SMALL = 3,
    ACTION_LEFT_LARGE = 4,
    ACTION_RIGHT_LARGE = 5,
    ACTION_RESET = 6,
    
    // 扫码枪动作码 (71-77)
    ACTION_BARCODE_71 = 71,  // 举左手
    ACTION_BARCODE_72 = 72,  // 举右手
    ACTION_BARCODE_73 = 73,  // 举双手
    ACTION_BARCODE_74 = 74,  // 抬左脚
    ACTION_BARCODE_75 = 75,  // 抬右脚
    ACTION_BARCODE_76 = 76,  // 摇头1
    ACTION_BARCODE_77 = 77,  // 摇头3
    
    // OpenMV动作码 (96, 11, 97)
    ACTION_OPENMV_96 = 96,
    ACTION_OPENMV_11 = 11,
    ACTION_OPENMV_97 = 97
} ActionCode_t;

// 通信状态
typedef struct {
    uint8_t barcode_active;
    uint8_t openmv_active;
    uint8_t grayscale_active;
    ActionCode_t current_action;
    CommPriority_t current_priority;
} CommState_t;

// 全局变量声明
extern CommState_t comm_state;

// 函数声明
void CommManager_Init(void);
void CommManager_Update(void);
ActionCode_t CommManager_GetActionCode(void);
void CommManager_ClearActionCode(void);
uint8_t CommManager_IsActionAvailable(void);

// 各模块状态检查函数
uint8_t CommManager_CheckBarcodeScanner(void);
uint8_t CommManager_CheckOpenMV(void);
uint8_t CommManager_CheckGrayscale(void);
ActionCode_t CommManager_GetBarcodeActionCode(void);

// 动作执行函数
void CommManager_ExecuteAction(ActionCode_t action_code);

#endif /* __COMM_MANAGER_H */
