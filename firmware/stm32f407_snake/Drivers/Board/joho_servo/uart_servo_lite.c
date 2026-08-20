/**
 * JOHO UART舵机控制_精简版
 * JOHO SDK
 ***/
#include "uart_servo_lite.h"

extern volatile uint32_t usart3_rx_count;

//构建并发送需要的协议帧: 帧头	ID号	数据长度	指令类型	内容	校验和
void JOHO_PackageBuild_Send(Usart_DataTypeDef *usart, uint8_t usId, uint8_t size,uint8_t cmdType, uint8_t *content){
  // 使用栈上分配的内存，避免动态分配，此处数据包大小固定
    PackageTypeDef pkg;

    if (!usart || !usart->sendBuf || size < 2 ||
        (uint8_t)(size - 2) > JOHO_PACK_RESPONSE_MAX_SIZE ||
        (size > 2 && !content)) {
        return;
    }
	
    // 设置帧头
    pkg.header = JOHO_PACK_REQUEST_HEADER;
    // 设置ID号
    pkg.usId = usId;
    // 数据长度		
    pkg.size = size;
	
	  //指令类型
		pkg.sstat = cmdType;
		// 将内容复制到数据区
		for(int i=0; i<size-2; i++){
			pkg.content[i] = content[i];
		}
    // 将pkg写入到发送缓冲区sendBuf中
    JOHO_Package2RingBuffer(&pkg, usart->sendBuf);
		// 通过串口将数据发送出去
    Usart_SendAll(usart);
}




// 发送原始数据帧-HEX
void USL_Send_HEX(Usart_DataTypeDef *usart, uint8_t size, uint8_t *content){

	
	RingBuffer_WriteByteArray( usart->sendBuf,content,size);
	
	// 通过串口将数据发送出去
    Usart_SendAll(usart);
}


// 接收协议帧。先搜索响应头 FF F5（兼容 F5 FF），因此会自动跳过发送回显 FF FF。
JOHO_STATUS USL_RecvPackage(Usart_DataTypeDef *usart, PackageTypeDef *pkg)
{
    enum {
        RX_SEARCH_HEADER = 0,
        RX_ID,
        RX_SIZE,
        RX_STATUS,
        RX_CONTENT,
        RX_CHECKSUM
    } state = RX_SEARCH_HEADER;

    uint8_t previous = 0;
    uint8_t content_idx = 0;
    uint8_t content_len = 0;

#ifdef DEBUG_SERVO_RAW
    uint8_t rawBytes[128];
    uint8_t rawIdx = 0;
#endif

    if (!usart || !usart->recvBuf || !pkg) {
        return JOHO_STATUS_INVALID_ARGUMENT;
    }

    pkg->status = 0;
    SysTick_CountdownBegin(JOHO_TIMEOUT_MS);

    while (!SysTick_CountdownIsTimeout()) {
        if (RingBuffer_GetByteUsed(usart->recvBuf) == 0) {
            continue;
        }

        uint8_t byte = RingBuffer_ReadByte(usart->recvBuf);
#ifdef DEBUG_SERVO_RAW
        if (rawIdx < sizeof(rawBytes)) rawBytes[rawIdx++] = byte;
#endif

        switch (state) {
        case RX_SEARCH_HEADER:
            if ((previous == 0xFF && byte == 0xF5) ||
                (previous == 0xF5 && byte == 0xFF)) {
                pkg->header = JOHO_PACK_RESPONSE_HEADER;
                pkg->status = JOHO_RECV_FLAG_HEADER;
                state = RX_ID;
                previous = 0;
            } else {
                previous = byte;
            }
            break;

        case RX_ID:
            pkg->usId = byte;
            if (pkg->usId > JOHO_US_NUM) {
                SysTick_CountdownCancel();
                return JOHO_STATUS_UNKOWN_US_ID;
            }
            pkg->status = JOHO_RECV_FLAG_US_ID;
            state = RX_SIZE;
            break;

        case RX_SIZE:
            pkg->size = byte;
            if (pkg->size < 2) {
                SysTick_CountdownCancel();
                return JOHO_STATUS_LENGTH_ERROR;
            }
            content_len = pkg->size - 2;
            if (content_len > JOHO_PACK_RESPONSE_MAX_SIZE) {
                SysTick_CountdownCancel();
                return JOHO_STATUS_SIZE_TOO_BIG;
            }
            pkg->status = JOHO_RECV_FLAG_SIZE;
            state = RX_STATUS;
            break;

        case RX_STATUS:
            pkg->sstat = byte;
            pkg->status = JOHO_RECV_FLAG_SSTAT;
            state = (content_len == 0) ? RX_CHECKSUM : RX_CONTENT;
            break;

        case RX_CONTENT:
            pkg->content[content_idx++] = byte;
            if (content_idx == content_len) {
                pkg->status = JOHO_RECV_FLAG_CONTENT;
                state = RX_CHECKSUM;
            }
            break;

        case RX_CHECKSUM:
            pkg->checksum = byte;
            pkg->status |= JOHO_RECV_FLAG_CHECKSUM;
            SysTick_CountdownCancel();
            return (JOHO_CalcChecksum(pkg) == pkg->checksum)
                       ? JOHO_STATUS_SUCCESS
                       : JOHO_STATUS_CHECKSUM_ERROR;
        }
    }

#ifdef DEBUG_SERVO_RAW
    printf("\r\n[RAW RX] %u bytes:", rawIdx);
    for (uint8_t i = 0; i < rawIdx; i++) printf(" %02X", rawBytes[i]);
    printf("\r\n");
#endif
    return JOHO_STATUS_TIMEOUT;
}


// 将协议帧转换为字节流
void JOHO_Package2RingBuffer(PackageTypeDef *pkg,  RingBufferTypeDef *ringBuf){
    uint8_t checksum; // 校验和
//    // 写入帧头 不参与校验，在计算校验和时排除
    RingBuffer_WriteUShort(ringBuf, pkg->header);
    // 写入舵机ID
    RingBuffer_WriteByte(ringBuf, pkg->usId);
    // 写入数据的长度
    RingBuffer_WriteByte(ringBuf, pkg->size);
	//写入状态码or 指令类型
	RingBuffer_WriteByte(ringBuf, pkg->sstat);
    // 写入数据内容
	if(pkg->size != 2)
    RingBuffer_WriteByteArray(ringBuf, pkg->content, pkg->size-2);
    // 计算校验和
    checksum = RingBuffer_GetChecksum(ringBuf);
    // 写入校验和
    RingBuffer_WriteByte(ringBuf, checksum);
	
//	
//	printf("Auto get ServoID usId %d \r\n", pkg->usId);
//	printf("Auto get ServoID pkg->size %d \r\n", pkg->size);
//	printf("Auto get ServoID pkg->sstat %d \r\n", pkg->sstat);
//	printf("Auto get ServoID pkg->content %d \r\n", pkg->content[0]);
//	printf("Auto get ServoID checksum %d \r\n", checksum);

}



// 计算Package的校验和
uint8_t JOHO_CalcChecksum(PackageTypeDef *pkg){
    uint32_t sum;

    if (!pkg || pkg->size < 2) return 0;

    sum = pkg->usId + pkg->size + pkg->sstat;
    for (uint8_t i = 0; i < (uint8_t)(pkg->size - 2); i++) {
        sum += pkg->content[i];
    }
    return (uint8_t)(~sum);
}



/**
 * 同步写角度 - 一次通讯设置多个舵机不同角度
 *
 * 协议帧格式 (JOHO SyncWrite):
 *   FF FF FE <size> 83 <reg> <data_len> <ID1> <data1...> <ID2> <data2...> <CS>
 *
 * 对角度控制 (reg=0x2A, 每个舵机4字节数据: angle 2B + interval 2B):
 *   FF FF FE <size> 83 2A 04 <ID1> <aH> <aL> <iH> <iL> <ID2> ... <CS>
 *
 * 若count超过 SYNC_WRITE_MAX_SERVOS, 自动分多次发送
 **/
void USL_SyncWriteAngles(Usart_DataTypeDef *usart,
                         uint8_t *servo_ids,
                         uint16_t *positions,
                         uint16_t *intervals,
                         uint8_t count)
{
    if (count == 0 || !usart || !servo_ids || !positions || !intervals) return;

    uint8_t sent = 0;
    while (sent < count) {
        /* 每批最多 SYNC_WRITE_MAX_SERVOS 个舵机 */
        uint8_t batch = count - sent;
        if (batch > SYNC_WRITE_MAX_SERVOS) batch = SYNC_WRITE_MAX_SERVOS;

        /* 构建内容: [reg=0x2A] [data_len=0x04] [ID1] [a1H] [a1L] [i1H] [i1L] [ID2] ... */
        uint8_t content[2 + SYNC_WRITE_MAX_SERVOS * 5];
        uint8_t idx = 0;

        content[idx++] = 0x2A;  // 目标角度寄存器地址
        content[idx++] = 0x04;  // 每个舵机4字节数据 (angle 2B + interval 2B)

        for (uint8_t i = 0; i < batch; i++) {
            uint8_t  sid  = servo_ids[sent + i];
            uint16_t pos  = positions[sent + i];
            uint16_t intr = intervals[sent + i];

            content[idx++] = sid;
            content[idx++] = (pos >> 8) & 0xFF;    // angle high byte (大端)
            content[idx++] = pos & 0xFF;            // angle low byte
            content[idx++] = (intr >> 8) & 0xFF;    // interval high byte (大端)
            content[idx++] = intr & 0xFF;           // interval low byte
        }

        /* size = 2(usId+cmdType) + idx(content) */
        uint8_t size = 2 + idx;

        /* 发送同步写帧, 使用广播ID 0xFE */
        JOHO_PackageBuild_Send(usart, JOHO_BROADCAST_ID, size,
                               CMDType_SyncWrite, content);

        sent += batch;
    }

    /* 等待所有数据发送完成 + 回显到达 */
    SysTick_DelayMs(5);
}

/**
 * 舵机控制SDK
 **/

//Ping 舵机 状态查询
JOHO_STATUS US_Ping(Usart_DataTypeDef *usart, uint8_t servo_id){
	uint8_t statusCode; // 状态码
	uint8_t ehcoServoId; // PING得到的舵机ID
    if (!usart || !usart->recvBuf ||
        servo_id == 0 || servo_id > JOHO_SERVO_ID_MAX) {
        return JOHO_STATUS_INVALID_ARGUMENT;
    }
    // 丢弃上一条写指令留下的回显/应答，再开始一个完整事务
    RingBuffer_Reset(usart->recvBuf);
//	printf("[PING]Send Ping Package\r\n");
	// 发送Ping请求
	JOHO_PackageBuild_Send(usart, servo_id, 2,CMDType_Ping, NULL);
	// 解析器等待数据并自动跳过发送回显，只接受响应帧头
	PackageTypeDef pkg;
	statusCode = USL_RecvPackage(usart, &pkg);
	if(statusCode == JOHO_STATUS_SUCCESS){
		// 校验返回的ID号是否匹配
		ehcoServoId = (uint8_t)pkg.usId;
		
		if (ehcoServoId != servo_id){
			// 如果得到的舵机ID号不匹配
			return JOHO_STATUS_ID_NOT_MATCH;
		}
        if (pkg.size != 2) {
            return JOHO_STATUS_LENGTH_ERROR;
        }
        if (pkg.sstat != 0) {
            return JOHO_STATUS_SERVO_ERROR;
        }
//		printf("[succ]Auto get ServoID %d \r\n", ehcoServoId);
	}
	return statusCode;
}


//控制舵机变换位置
void USL_SetServoAngle(Usart_DataTypeDef *usart, uint8_t servo_id, \
				float posi, uint16_t interval){
//参数校验
if(posi > 4095)posi = 4095;
if(posi <0 )		posi  = 0;			

uint16_t posit = posi;
uint8_t content[5];
					//示例：角度0~4095  FF FF 01 07 03 2A 00 00 03 E8 DF
content[0] =0x2A;
content[1] = posit	>> 8&0XFF;			
content[2] = posit	&0XFF;						
content[3] = interval	>> 8&0XFF;			
content[4] = interval	&0XFF;	
	/// 发送协议帧
	JOHO_PackageBuild_Send(usart, servo_id, 7,CMDType_Write,content);
	
	// 指令发送完毕后等待5ms，确保UART发送完成且回显数据到达
	SysTick_DelayMs(5);

}

JOHO_STATUS USL_ReadRegisters(Usart_DataTypeDef *usart, uint8_t servo_id,
                              uint8_t reg_addr, uint8_t data_len,
                              uint8_t *data)
{
    uint8_t request[2] = {reg_addr, data_len};
    JOHO_STATUS last_status = JOHO_STATUS_FAIL;

    if (!usart || !usart->recvBuf || !data || data_len == 0 ||
        data_len > JOHO_PACK_RESPONSE_MAX_SIZE ||
        servo_id == 0 || servo_id > JOHO_SERVO_ID_MAX) {
        return JOHO_STATUS_INVALID_ARGUMENT;
    }

    for (uint8_t attempt = 0; attempt < JOHO_READ_ATTEMPTS; attempt++) {
        PackageTypeDef pkg;

        /*
         * 只在发送前清旧数据。绝不能在发送后清缓冲区，否则会把已经
         * 到达的舵机响应和回显一起删除。
         */
        RingBuffer_Reset(usart->recvBuf);
        JOHO_PackageBuild_Send(usart, servo_id, 4, CMDType_Read, request);

        last_status = USL_RecvPackage(usart, &pkg);
        if (last_status != JOHO_STATUS_SUCCESS) {
            continue;
        }
        if (pkg.usId != servo_id) {
            last_status = JOHO_STATUS_ID_NOT_MATCH;
            continue;
        }
        if (pkg.sstat != 0) {
            return JOHO_STATUS_SERVO_ERROR;
        }
        if (pkg.size != (uint8_t)(data_len + 2)) {
            last_status = JOHO_STATUS_LENGTH_ERROR;
            continue;
        }

        for (uint8_t i = 0; i < data_len; i++) {
            data[i] = pkg.content[i];
        }
        return JOHO_STATUS_SUCCESS;
    }

    return last_status;
}

//角度查询  返回值为0-4095 为有效值，失败返回0xFFFF
uint16_t USL_GETPositionVal(Usart_DataTypeDef *usart, uint8_t servo_id)
{
    uint8_t data[2];
    if (USL_ReadRegisters(usart, servo_id, JOHO_REG_PRESENT_POSITION, 2, data)
        != JOHO_STATUS_SUCCESS) {
        return 0xFFFF;
    }
    return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

//设置扭矩控制 0关闭 1开启
void SET_Torque(Usart_DataTypeDef *usart, uint8_t servo_id,uint8_t isopen){
//	uint8_t statusCode; // 状态码
	uint8_t content[2];
	content[0] =0x28;content[1] =0x01;
	if(isopen == 0){content[1] =0x00;}
	// 发送协议帧-示例：FF FF 01 04 03 28 01 CE
	JOHO_PackageBuild_Send(usart, servo_id, 4,CMDType_Write, content);
	// 等待5ms确保UART发送完成
	SysTick_DelayMs(5);

}

/**
 * @brief 读取舵机供电电压
 * @param usart 串口句柄
 * @param servo_id 舵机ID
 * @return 电压原始值（厂家寄存器表标注单位V）；失败返回0xFFFF
 * @note 读取 JOHO_REG_PRESENT_VOLTAGE (1字节)
 */
uint16_t USL_GetVoltage(Usart_DataTypeDef *usart, uint8_t servo_id)
{
    uint8_t data;
    if (USL_ReadRegisters(usart, servo_id, JOHO_REG_PRESENT_VOLTAGE, 1, &data)
        == JOHO_STATUS_SUCCESS) {
        return data;
    }
    return 0xFFFF;
}

/**
 * @brief 读取舵机电流
 * @param usart 串口句柄
 * @param servo_id 舵机ID
 * @return 电流值(有符号)，失败返回0xFFFF
 * @note 读取寄存器 0x2E (2字节) — 实测此寄存器有效
 */
int16_t USL_GetCurrent(Usart_DataTypeDef *usart, uint8_t servo_id)
{
    uint8_t data[2];
    if (USL_ReadRegisters(usart, servo_id, JOHO_REG_PRESENT_CURRENT, 2, data)
        == JOHO_STATUS_SUCCESS) {
        return (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    }
    return (int16_t)0xFFFF;
}

/**
 * @brief 读取舵机状态: 角度+电压+电流+温度
 * @param usart 串口句柄
 * @param servo_id 舵机ID
 * @param position 输出: 位置值 (0-4095)
 * @param voltage  输出: 电压值 (V)
 * @param current  输出: 电流值 (有符号)
 * @param temperature 输出: 温度值 (°C)
 * @return 0=成功, 非0=错误码
 * @note 该型号单次最多读取2字节，因此各字段使用独立、完整校验的事务读取。
 */
uint8_t USL_GetServoStatus(Usart_DataTypeDef *usart, uint8_t servo_id,
                           uint16_t *position, uint16_t *voltage,
                           int16_t *current, int8_t *temperature)
{
    uint8_t data[2];
    JOHO_STATUS status;

    if (!position && !voltage && !current && !temperature) {
        return JOHO_STATUS_INVALID_ARGUMENT;
    }

    if (position) {
        *position = 0xFFFF;
        status = USL_ReadRegisters(usart, servo_id,
                                   JOHO_REG_PRESENT_POSITION, 2, data);
        if (status != JOHO_STATUS_SUCCESS) return status;
        *position = (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
    }

    if (voltage) {
        *voltage = 0xFFFF;
        status = USL_ReadRegisters(usart, servo_id,
                                   JOHO_REG_PRESENT_VOLTAGE, 1, data);
        if (status != JOHO_STATUS_SUCCESS) return status;
        *voltage = data[0];
    }

    if (temperature) {
        *temperature = INT8_MIN;
        status = USL_ReadRegisters(usart, servo_id,
                                   JOHO_REG_PRESENT_TEMP, 1, data);
        if (status != JOHO_STATUS_SUCCESS) return status;
        *temperature = (int8_t)data[0];
    }

    if (current) {
        *current = (int16_t)0xFFFF;
        status = USL_ReadRegisters(usart, servo_id,
                                   JOHO_REG_PRESENT_CURRENT, 2, data);
        if (status != JOHO_STATUS_SUCCESS) return status;
        *current = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    }

    return JOHO_STATUS_SUCCESS;
}
