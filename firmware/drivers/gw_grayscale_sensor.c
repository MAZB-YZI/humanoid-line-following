/*
 * Copyright (c) 2025 北京感为科技
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 */

#include "gw_grayscale_sensor.h"
#include "spl_i2c.h"
#include "stm32f10x.h"
#include "delay.h"

// 灰度传感器I2C地址
#define GRAYSCALE_I2C_ADDR GW_GRAY_ADDR_DEF

// 全局变量存储传感器数据
static uint8_t grayscale_digital_data = 0;
static uint16_t grayscale_analog_data[8] = {0};

/**
 * @brief 初始化灰度传感器
 * @return 0: 成功, 非0: 失败
 */
int8_t grayscale_sensor_init(void)
{
    uint8_t response;
    
    // 测试传感器连接 - 发送ping命令
    if(spl_i2c_write_byte(I2C1, GRAYSCALE_I2C_ADDR, GW_GRAY_PING) != 0) {
        return -1; // I2C写入失败
    }
    
    delay_ms(10); // 等待传感器响应
    
    // 读取ping响应
    if(spl_i2c_read_byte(I2C1, GRAYSCALE_I2C_ADDR, &response) != 0) {
        return -2; // I2C读取失败
    }
    
    // 检查响应是否正确
    if(response != GW_GRAY_PING_OK) {
        return -3; // 传感器响应错误
    }
    
    // 设置传感器为数字模式
    if(spl_i2c_write_byte(I2C1, GRAYSCALE_I2C_ADDR, GW_GRAY_DIGITAL_MODE) != 0) {
        return -4; // 设置数字模式失败
    }
    
    delay_ms(10); // 等待设置生效
    
    return 0; // 初始化成功
}

/**
 * @brief 读取灰度传感器数字数据
 * @return 8位数字数据，每位代表一个传感器的状态
 */
uint8_t grayscale_read_digital(void)
{
    uint8_t data = 0;
    
    // 读取数字数据
    if(spl_i2c_read_byte(I2C1, GRAYSCALE_I2C_ADDR, &data) == 0) {
        grayscale_digital_data = data;
    }
    
    return grayscale_digital_data;
}

/**
 * @brief 读取灰度传感器模拟数据
 * @param channel_data 存储8个通道数据的数组
 * @return 0: 成功, 非0: 失败
 */
int8_t grayscale_read_analog(uint16_t channel_data[8])
{
    uint8_t data[2];
    int8_t result;
    
    // 设置传感器为模拟模式
    if(spl_i2c_write_byte(I2C1, GRAYSCALE_I2C_ADDR, GW_GRAY_ANALOG_MODE) != 0) {
        return -1; // 设置模拟模式失败
    }
    
    delay_ms(10); // 等待模式切换
    
    // 读取8个通道的模拟数据
    for(uint8_t i = 0; i < 8; i++) {
        // 读取指定通道的数据
        result = spl_i2c_mem8_read(I2C1, GRAYSCALE_I2C_ADDR, GW_GRAY_ANALOG(i+1), data, 2);
        if(result == 0) {
            // 组合高低字节为16位数据
            channel_data[i] = (uint16_t)((data[1] << 8) | data[0]);
            grayscale_analog_data[i] = channel_data[i];
        } else {
            // 读取失败，使用上一次的值
            channel_data[i] = grayscale_analog_data[i];
        }
        delay_ms(5); // 通道间延时
    }
    
    return 0;
}

/**
 * @brief 获取指定传感器的数字状态
 * @param sensor_num 传感器编号 (1-8)
 * @return 1: 检测到黑线, 0: 检测到白线
 */
uint8_t grayscale_get_sensor_state(uint8_t sensor_num)
{
    if(sensor_num < 1 || sensor_num > 8) {
        return 0; // 无效的传感器编号
    }
    
    return GET_NTH_BIT(grayscale_digital_data, sensor_num);
}

/**
 * @brief 获取指定传感器的模拟值
 * @param sensor_num 传感器编号 (1-8)
 * @return 模拟值 (0-4095)
 */
uint16_t grayscale_get_sensor_analog(uint8_t sensor_num)
{
    if(sensor_num < 1 || sensor_num > 8) {
        return 0; // 无效的传感器编号
    }
    
    return grayscale_analog_data[sensor_num - 1];
}

/**
 * @brief 获取所有传感器的数字状态
 * @param sensor_states 存储8个传感器状态的数组
 */
void grayscale_get_all_sensor_states(uint8_t sensor_states[8])
{
    SEP_ALL_BIT8(grayscale_digital_data, 
                 sensor_states[0], sensor_states[1], sensor_states[2], sensor_states[3],
                 sensor_states[4], sensor_states[5], sensor_states[6], sensor_states[7]);
}

/**
 * @brief 设置传感器的黑线阈值
 * @param threshold 阈值 (0-4095)
 * @return 0: 成功, 非0: 失败
 */
int8_t grayscale_set_black_threshold(uint16_t threshold)
{
    uint8_t data[2];
    data[0] = threshold & 0xFF;        // 低字节
    data[1] = (threshold >> 8) & 0xFF; // 高字节
    
    return spl_i2c_mem8_write(I2C1, GRAYSCALE_I2C_ADDR, GW_GRAY_CALIBRATION_BLACK, data, 2);
}

/**
 * @brief 设置传感器的白线阈值
 * @param threshold 阈值 (0-4095)
 * @return 0: 成功, 非0: 失败
 */
int8_t grayscale_set_white_threshold(uint16_t threshold)
{
    uint8_t data[2];
    data[0] = threshold & 0xFF;        // 低字节
    data[1] = (threshold >> 8) & 0xFF; // 高字节
    
    return spl_i2c_mem8_write(I2C1, GRAYSCALE_I2C_ADDR, GW_GRAY_CALIBRATION_WHITE, data, 2);
}

/**
 * @brief 获取传感器固件版本
 * @return 固件版本号
 */
uint8_t grayscale_get_firmware_version(void)
{
    uint8_t version = 0;
    
    spl_i2c_mem8_read(I2C1, GRAYSCALE_I2C_ADDR, GW_GRAY_FIRMWARE, &version, 1);
    
    return version;
}

/**
 * @brief 重启传感器
 * @return 0: 成功, 非0: 失败
 */
int8_t grayscale_sensor_reboot(void)
{
    return spl_i2c_write_byte(I2C1, GRAYSCALE_I2C_ADDR, GW_GRAY_REBOOT);
}



