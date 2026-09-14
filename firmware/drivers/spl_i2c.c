/*
 * Copyright (c) 2025 北京感为科技
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 */

#include "spl_i2c.h"

/* i2c超时时间变量,仅供参考,需要实际更改 */
static uint32_t i2c_timeout = 1000;

/* 等待事件发生,或者超时退出 */
static ErrorStatus spl_i2c_wait_event(I2C_TypeDef *I2Cx, uint32_t I2C_EVENT, uint32_t timeout);

void i2c_set_timeout(uint32_t timeout)
{
	i2c_timeout = timeout;
}

int8_t spl_i2c_read(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *data, uint8_t data_size)
{
	int i;
	ErrorStatus status;
	I2C_GenerateSTART(I2Cx, ENABLE);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT, i2c_timeout);
	if (status != SUCCESS)
		return 1;
	
	I2C_Send7bitAddress(I2Cx, addr, I2C_Direction_Receiver);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, i2c_timeout);
	if (status != SUCCESS)
		return 2;
	
	for (i = 0; i < data_size-1; ++i) {
		/* 因为需要应答,所以需要先准备应答位,然后再接受数据 */
		status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED, i2c_timeout);
		if (status != SUCCESS)
			return 3;
		*data++ = I2C_ReceiveData(I2Cx);
	}
	
	/* 发送数据前,拉低应答位,推高停止位,不然最后一个数据发送完再配置会导致停止位发送不及时 */
	/* 暂时关闭应答 */
	I2C_AcknowledgeConfig(I2Cx, DISABLE);
	I2C_GenerateSTOP(I2Cx, ENABLE);
	
	/* 接收最后一个数据 */
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED, i2c_timeout);
	if (status != SUCCESS)
		return 3;
	*data++ = I2C_ReceiveData(I2Cx);
	
	/* 重新打开默认的应答设置 */
	I2C_AcknowledgeConfig(I2Cx, ENABLE);

	return 0;
}

int8_t spl_i2c_write_byte(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t data)
{
	return spl_i2c_write(I2Cx, addr, &data, 1);
}

int8_t spl_i2c_read_byte(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *data)
{
	return spl_i2c_read(I2Cx, addr, data, 1);
}


int8_t spl_i2c_write(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *data, uint8_t data_size)
{
	int i;
	ErrorStatus status;
	I2C_GenerateSTART(I2Cx, ENABLE);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT, i2c_timeout);
	if (status == ERROR)
		return 1;
	
	I2C_Send7bitAddress(I2Cx, addr, I2C_Direction_Transmitter);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, i2c_timeout);
	if (status == ERROR)
		return 2;
	
	for (i = 0; i < data_size-1; ++i) {
		I2C_SendData(I2Cx, *data++);
		status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTING, i2c_timeout);
		if (status == ERROR)
			return 3;
	}
	
	I2C_SendData(I2Cx, *data++);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED, i2c_timeout);
	if (status == ERROR)
		return 4;
	
	I2C_GenerateSTOP(I2Cx, ENABLE);
	
	return 0;
}


int8_t spl_i2c_mem8_read(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t mem_addr, uint8_t *data, uint8_t data_size)
{
	int i;
	ErrorStatus status;

	/* 
		发送设备地址
	*/
	I2C_GenerateSTART(I2Cx, ENABLE);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT, i2c_timeout);
	if (status == ERROR)
		return 1;
	
	I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Transmitter);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, i2c_timeout);
	if (status == ERROR)
		return 2;

	/*
		发送寄存器地址
	*/
	
	I2C_SendData(I2Cx, mem_addr);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED, i2c_timeout);
	if (status == ERROR)
		return 3;


	/*
		无停止位重新使能,读取数据
	 */
	I2C_GenerateSTART(I2Cx, ENABLE);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT, i2c_timeout);
	if (status == ERROR)
		return 4;
	
	I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Receiver);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, i2c_timeout);
	if (status == ERROR)
		return 5;
	
	for (i = 0; i < data_size-1; ++i) {
		status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED, i2c_timeout);
		if (status != SUCCESS)
			return 6;
		*data++ = I2C_ReceiveData(I2Cx);
	}
	
	I2C_AcknowledgeConfig(I2Cx, DISABLE);
	I2C_GenerateSTOP(I2Cx, ENABLE);
	
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED, i2c_timeout);
	if (status != SUCCESS)
		return 7;
	*data++ = I2C_ReceiveData(I2Cx);
	
	I2C_AcknowledgeConfig(I2Cx, ENABLE);
	
	return 0;
}

int8_t spl_i2c_mem8_write(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t mem_addr, uint8_t *data, uint8_t data_size)
{
	int i;
	ErrorStatus status;
	I2C_GenerateSTART(I2Cx, ENABLE);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT, i2c_timeout);
	if (status == ERROR)
		return 1;
	
	I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Transmitter);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, i2c_timeout);
	if (status == ERROR)
		return 2;

	/* 发送寄存器地址 */
	I2C_SendData(I2Cx, mem_addr);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTING, i2c_timeout);
	if (status == ERROR)
		return 3;

	/* 发送数据 */
	for (i = 0; i < data_size-1; ++i) {
		I2C_SendData(I2Cx, *data++);
		status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTING, i2c_timeout);
		if (status == ERROR)
			return 3;
	}
	
	I2C_SendData(I2Cx, *data++);
	status = spl_i2c_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED, i2c_timeout);
	if (status == ERROR)
		return 4;
	
	I2C_GenerateSTOP(I2Cx, ENABLE);
	
	return 0;
}

static ErrorStatus spl_i2c_wait_event(I2C_TypeDef *I2Cx, uint32_t I2C_EVENT, uint32_t timeout)
{
	volatile uint32_t local_timeout = timeout;
	while(local_timeout != 0) {
		if (I2C_CheckEvent(I2Cx, I2C_EVENT) == SUCCESS) {
			return SUCCESS;
		}
		--local_timeout;
	}
	
	return ERROR;
}

/**
 * @brief 启动I2C传输
 * @param I2Cx I2C外设(如I2C1, I2C2)
 * @param address 7位设备地址
 * @note 发送起始信号和设备地址(写模式)
 *       若设备无应答则立即终止传输
 */
void i2c_begin_transmission(I2C_TypeDef* I2Cx, uint8_t address)
{
    /* 生成起始条件 */
    I2C_GenerateSTART(I2Cx, ENABLE);
    
    /* 等待EV5事件(起始条件已发送) */
    while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));
    
    /* 发送设备地址(写模式) */
    I2C_Send7bitAddress(I2Cx, address << 1, I2C_Direction_Transmitter);
    
    /* 检查设备是否应答 */
    if(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        /* 无应答则发送停止条件 */
        I2C_GenerateSTOP(I2Cx, ENABLE);
    }
}

/**
 * @brief 写入数据到I2C设备
 * @param I2Cx I2C外设(如I2C1, I2C2)
 * @param data 待写入数据指针
 * @param length 数据长度
 * @return 实际成功写入的字节数
 * @note 遇到设备无应答时停止写入
 */
uint8_t i2c_write(I2C_TypeDef* I2Cx, uint8_t* data, uint8_t length)
{
    uint8_t count = 0;
    
    for(uint8_t i = 0; i < length; i++) 
    {
        /* 发送数据字节 */
        I2C_SendData(I2Cx, data[i]);
        
        /* 等待EV8事件(数据寄存器空) */
        if(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
        {
            break;  // 发送失败
        }
        
        count++;
    }
    
    /* 等待最后一个字节发送完成 */
    if(count == length)
    {
        while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }
    
    return count;
}

/**
 * @brief 结束I2C传输
 * @param I2Cx I2C外设(如I2C1, I2C2)
 * @note 发送停止信号终止当前传输
 */
void i2c_end_transmission(I2C_TypeDef* I2Cx)
{
    /* 生成停止条件 */
    I2C_GenerateSTOP(I2Cx, ENABLE);
    
    /* 等待总线空闲 */
    while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY));
}
