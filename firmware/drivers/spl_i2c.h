/*
 * Copyright (c) 2022 感为智能科技(济南)
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 */


#ifndef __SPL_I2C_H__
#define __SPL_I2C_H__

#include <stdint.h>
#include <stm32f10x_i2c.h>

/* 设置应答超时 */
__inline void i2c_set_timeout(uint32_t timeout);

/**
 * 向I2Cx写入一个字节
 * 起始位 + 设备地址 + 写入位 + 写入一个字节
 * @param I2Cx I2C 通道
 * @param addr 设备地址
 * @param data 写入的字节
 * @return 0为成功, 大于0出错
 */
int8_t spl_i2c_write_byte(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t data);

/**
 * 向I2Cx读取一个字节
 * 起始位 + 设备地址 + 读取位 + 读取一个字节 + 停止位
 * @param I2Cx I2C 通道
 * @param addr 设备地址
 * @param data 读取的字节
 * @return 0为成功, 大于0出错
 */
int8_t spl_i2c_read_byte(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *data);

/**
 * 向I2Cx读取多个字节
 * 起始位 + 设备地址 + 读取位 + 读取字节 + ACK + 读取字节 + ACK +  ... + 读取字节 + 停止位
 * @param I2Cx I2C 通道
 * @param addr 设备地址
 * @param data 读取的字节
 * @return 0为成功, 大于0出错
 */
int8_t spl_i2c_read(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *data, uint8_t data_size);

/**
 * 向I2Cx读取多个字节
 * 起始位 + 设备地址 + 写入位 + 写入字节 + ... + 写入字节 + 停止位
 * @param I2Cx I2C 通道
 * @param addr 设备地址
 * @param data 写入的字节
 * @return 0为成功, 大于0出错
 */
int8_t spl_i2c_write(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *data, uint8_t data_size);

/**
 * 向I2Cx写入mem_addr + 写入多个字节
 * 起始位 + 设备地址 + 写入位 + 写入mem_addr + 写入字节 + ... + 写入字节 + 停止位
 * @param I2Cx I2C 通道
 * @param dev_addr 设备地址
 * @param mem_addr 寄存器地址
 * @param data 写入的字节
 * @return 0为成功, 大于0出错
 */
int8_t spl_i2c_mem8_write(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t mem_addr, uint8_t *data, uint8_t data_size);

/**
 * 向I2Cx写入mem_addr + 读取多个字节
 * 起始位 + 设备地址 + 读取位 + 写入mem_addr + 起始位 + 设备地址 + 读取位 + 读取字节 + ACK + 读取字节 + ACK +  ... + 读取字节 + 停止位
 * @param I2Cx I2C 通道
 * @param dev_addr 设备地址
 * @param mem_addr 寄存器地址
 * @param data 读取的字节
 * @return 0为成功, 大于0出错
 */
int8_t spl_i2c_mem8_read(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t mem_addr, uint8_t *data, uint8_t data_size);

void i2c_begin_transmission(I2C_TypeDef* I2Cx, uint8_t address);
uint8_t i2c_write(I2C_TypeDef* I2Cx, uint8_t* data, uint8_t length);
void i2c_end_transmission(I2C_TypeDef* I2Cx);

#endif // __SPL_I2C_H__
