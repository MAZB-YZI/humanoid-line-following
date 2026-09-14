/*
 * comm_manager.c
 * 通信管理模块
 * 作者: AI Assistant
 * 日期: 2024
 * 版本: V1.0
 */

#include "comm_manager.h"
#include "uart.h"
#include <string.h>
#include <stdio.h>

// Remove wiring.h dependency and implement millis() directly
extern __IO uint32_t TimingMillis; // Reference to global timing variable from wiring.c

static uint32_t millis(void) {
    return TimingMillis;
}

// 全局变量定义
CommState_t comm_state = {0};

// 通信管理器初始化
void CommManager_Init(void)
{
    // 初始化各通信模块
    BarcodeScanner_Init();
    OpenMV_Init();
    
    // 初始化状态
    memset(&comm_state, 0, sizeof(CommState_t));
    comm_state.current_action = ACTION_NONE;
    comm_state.current_priority = COMM_PRIORITY_GRAYSCALE;
}

// 通信管理器更新
void CommManager_Update(void)
{
    // 检查各模块状态
    comm_state.barcode_active = CommManager_CheckBarcodeScanner();
    comm_state.openmv_active = CommManager_CheckOpenMV();
    comm_state.grayscale_active = CommManager_CheckGrayscale();
    
    // 根据优先级确定当前动作
    if (comm_state.openmv_active) {
        comm_state.current_priority = COMM_PRIORITY_OPENMV;
        comm_state.current_action = OpenMV_GetActionCode();
    } else if (comm_state.barcode_active) {
        comm_state.current_priority = COMM_PRIORITY_BARCODE;
        comm_state.current_action = CommManager_GetBarcodeActionCode();
    } else if (comm_state.grayscale_active) {
        comm_state.current_priority = COMM_PRIORITY_GRAYSCALE;
        // 灰度传感器的动作码由gray()函数返回，这里不直接处理
    }
}

// 获取动作码
ActionCode_t CommManager_GetActionCode(void)
{
    return comm_state.current_action;
}

// 清除动作码
void CommManager_ClearActionCode(void)
{
    comm_state.current_action = ACTION_NONE;
    
    // 清除各模块标志
    if (comm_state.barcode_active) {
        BarcodeScanner_ClearFlag();
    }
}

// 检查是否有可用动作
uint8_t CommManager_IsActionAvailable(void)
{
    return (comm_state.current_action != ACTION_NONE);
}

// 检查扫码枪状态
uint8_t CommManager_CheckBarcodeScanner(void)
{
    if (BarcodeScanner_GetFlag()) {
        BarcodeScanner_ProcessData();
        return 1;
    }
    return 0;
}

// 检查OpenMV状态
uint8_t CommManager_CheckOpenMV(void)
{
    int action_code = OpenMV_ReceiveDataNonBlocking();
    if (action_code > 0) {
        return 1;
    }
    return 0;
}

// 检查灰度传感器状态
uint8_t CommManager_CheckGrayscale(void)
{
    // 灰度传感器总是活跃的，由主循环调用gray()函数
    return 1;
}

// 获取扫码枪动作码
ActionCode_t CommManager_GetBarcodeActionCode(void)
{
    if (BarcodeScanner_GetFlag()) {
        uint8_t data[BARCODE_PACKET_SIZE];
        uint8_t length = BarcodeScanner_GetData(data, sizeof(data));
        
        if (length > 0) {
            int action_code = BarcodeScanner_ParseActionCode(data, length);
            
            // 将扫码枪的动作码映射到对应的动作
            switch (action_code) {
                case 1: return ACTION_BARCODE_71;
                case 2: return ACTION_BARCODE_72;
                case 3: return ACTION_BARCODE_74;
                case 4: return ACTION_BARCODE_75;
                case 5: return ACTION_BARCODE_73;
                case 6: return ACTION_BARCODE_76;
               // case 7: return ACTION_BARCODE_77;
                default: return 73;
            }
        }
    }
    return ACTION_NONE;
}

// 执行动作
void CommManager_ExecuteAction(ActionCode_t action_code)
{
    // 这里需要调用run_action_code函数
    // 由于run_action_code在WalkingControl.c中定义，这里只做声明
    extern void run_action_code(int code);
    
    if (action_code != ACTION_NONE) {
        run_action_code((int)action_code);
    }
}

// 获取当前通信状态信息
void CommManager_GetStatus(char *status_str, uint8_t max_len)
{
    if (status_str && max_len > 0) {
        snprintf(status_str, max_len, 
                "B:%d O:%d G:%d A:%d P:%d", 
                comm_state.barcode_active,
                comm_state.openmv_active,
                comm_state.grayscale_active,
                comm_state.current_action,
                comm_state.current_priority);
    }
}

// 设置通信优先级
void CommManager_SetPriority(CommPriority_t priority)
{
    comm_state.current_priority = priority;
}

// 获取当前优先级
CommPriority_t CommManager_GetPriority(void)
{
    return comm_state.current_priority;
}

// 重置通信管理器
void CommManager_Reset(void)
{
    memset(&comm_state, 0, sizeof(CommState_t));
    comm_state.current_action = ACTION_NONE;
    comm_state.current_priority = COMM_PRIORITY_GRAYSCALE;
    
    BarcodeScanner_ClearFlag();
    OpenMV_ClearFlag();
}
