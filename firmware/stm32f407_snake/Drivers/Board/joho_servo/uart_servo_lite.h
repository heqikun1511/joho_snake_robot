#ifndef __UART_SERVO_LITE_H
#define __UART_SERVO_LITE_H

#include "stm32f4xx.h"
#include "usart.h"
#include "sys_tick.h"
#include "ring_buffer.h"

// ====== 调试选项: 取消注释可输出原始接收字节 ======
#define DEBUG_SERVO_RAW

// 状态码
#define JOHO_STATUS uint8_t
#define JOHO_STATUS_SUCCESS 0 // 发送/获取成功
#define JOHO_STATUS_FAIL 1 // 发送/获取失败
#define JOHO_STATUS_TIMEOUT 2 // 等待超时 
#define JOHO_STATUS_WRONG_RESPONSE_HEADER 3 // 响应头错误
#define JOHO_STATUS_UNKOWN_US_ID 4 // 未知的舵机ID
#define JOHO_STATUS_SIZE_TOO_BIG 5 // 接收的size超出JOHO_PACK_RESPONSE_MAX_SIZE范围
#define JOHO_STATUS_CHECKSUM_ERROR 6 // 校验和错误
#define JOHO_STATUS_ID_NOT_MATCH 7 // 接收的舵机ID与期望的舵机ID不匹配
#define JOHO_STATUS_INVALID_ARGUMENT 8 // 参数或舵机ID无效
#define JOHO_STATUS_LENGTH_ERROR 9 // 响应数据长度与请求不一致
#define JOHO_STATUS_SERVO_ERROR 10 // 舵机响应状态字非0


//指令类型
#define CMDType_Ping 1
#define CMDType_Read 2
#define CMDType_Write 3
#define CMDType_SyncWrite 0x83  // 同步写 (多个舵机不同角度, 一次指令)

// 8个双轴关节共16只舵机可在一个同步写包中更新
#define SYNC_WRITE_MAX_SERVOS 16

#define JOHO_SERVO_ID_MAX 250 // 厂家协议规定的舵机ID上限
#define JOHO_BROADCAST_ID 0xFE
#define JOHO_US_NUM JOHO_SERVO_ID_MAX

// 115200bps下正常应答仅需数毫秒；较短超时可避免掉线舵机拖死步态循环
#define JOHO_TIMEOUT_MS 50

// 读寄存器失败后的总尝试次数
#define JOHO_READ_ATTEMPTS 2

// 本项目实测使用的状态寄存器，可按具体舵机手册在此调整
#define JOHO_REG_PRESENT_POSITION 0x38
#define JOHO_REG_PRESENT_VOLTAGE  0x3E
#define JOHO_REG_PRESENT_TEMP     0x3F
#define JOHO_REG_PRESENT_CURRENT  0x2E

// 注意: JOHO舵机协议使用Big Endian(大端字节序/大端格式)
// 即多字节数据时: 高字节在前(低地址), 低字节在后(高地址)
// STM32是Little Endian, 所以在发送/接收多字节时需要转换字节序
#define JOHO_PACK_REQUEST_HEADER		0xffff
#define JOHO_PACK_RESPONSE_HEADER		0xf5ff

// 接收的响应数据包最长的长度
#define JOHO_PACK_RESPONSE_MAX_SIZE 96

// 帧头接收完成的标志位
#define JOHO_RECV_FLAG_HEADER 0x01
// 舵机ID接收完成的标志位
#define JOHO_RECV_FLAG_US_ID 0x02
// 数据长度接收完成的标志位
#define JOHO_RECV_FLAG_SIZE 0x04

//状态指令接收完成标志位
#define JOHO_RECV_FLAG_SSTAT 0x06

// 数据接收完成的标志位
#define JOHO_RECV_FLAG_CONTENT 0x08
// 校验和接收的标志位
#define JOHO_RECV_FLAG_CHECKSUM 0x10

// ��������֡�Ľṹ��
typedef struct{
    uint16_t header; // 帧头
    uint8_t usId; // 舵机ID
    uint8_t size; //数据长度：该值=ID号+指令类型+实际数据字节数，即=content+2
		uint8_t sstat; //1.作为响应包时表示舵机状态   2.作为请求包时表示指令类型
    uint8_t content[JOHO_PACK_RESPONSE_MAX_SIZE]; // 数据内容
    uint8_t checksum; // 校验和

    // 协议帧的接收进度状态 flag标志位
    uint8_t status; 
}PackageTypeDef;

// 发送原始数据帧-HEX
void USL_Send_HEX(Usart_DataTypeDef *usart, uint8_t size, uint8_t *content);

//接收协议帧
JOHO_STATUS USL_RecvPackage(Usart_DataTypeDef *usart,PackageTypeDef *pkg);


void JOHO_PackageBuild_Send(Usart_DataTypeDef *usart, uint8_t usId, uint8_t size,uint8_t cmdType, uint8_t *content);



void JOHO_Package2RingBuffer(PackageTypeDef *pkg,  RingBufferTypeDef *ringBuf);
uint8_t JOHO_CalcChecksum(PackageTypeDef *pkg);


JOHO_STATUS US_Ping(Usart_DataTypeDef *usart, uint8_t servo_id);
void USL_SetServoAngle(Usart_DataTypeDef *usart, uint8_t servo_id, \
				float posi, uint16_t interval);

/**
 * @brief 同步写角度 - 一次通讯设置多个舵机的角度 (更高效)
 *
 * 协议帧格式:
 *   FF FF FE <size> 83 2A 04 <ID1> <aH> <aL> <iH> <iL> <ID2> ... <CS>
 *
 * @param usart     串口句柄
 * @param servo_ids 舵机ID数组
 * @param positions 角度数组 (0~4095)
 * @param intervals 时间数组 (ms)
 * @param count     舵机数量 (≤ SYNC_WRITE_MAX_SERVOS, 超长自动分批)
 */
void USL_SyncWriteAngles(Usart_DataTypeDef *usart,
                         uint8_t *servo_ids,
                         uint16_t *positions,
                         uint16_t *intervals,
                         uint8_t count);

uint16_t USL_GETPositionVal(Usart_DataTypeDef *usart, uint8_t servo_id);

/**
 * @brief 读取任意寄存器，并验证响应帧头、ID、状态、长度和校验和
 * @return JOHO_STATUS_SUCCESS 或具体错误码
 */
JOHO_STATUS USL_ReadRegisters(Usart_DataTypeDef *usart, uint8_t servo_id,
                              uint8_t reg_addr, uint8_t data_len,
                              uint8_t *data);

// 读取舵机供电电压原始值 (厂家表标注单位V, 失败返回0xFFFF)
uint16_t USL_GetVoltage(Usart_DataTypeDef *usart, uint8_t servo_id);

// 读取舵机电流 (有符号, 返回 0.1A? 单位取决于具体舵机, 失败返回 0xFFFF)
int16_t USL_GetCurrent(Usart_DataTypeDef *usart, uint8_t servo_id);

// 读取完整状态。当前舵机单次最多读2字节，因此内部按字段分次读取。
// position[0-4095], voltage[V], current[mA], temperature[°C]
// 返回 0=成功, 非0=错误码
uint8_t USL_GetServoStatus(Usart_DataTypeDef *usart, uint8_t servo_id,
                           uint16_t *position, uint16_t *voltage,
                           int16_t *current, int8_t *temperature);

void SET_Torque(Usart_DataTypeDef *usart, uint8_t servo_id,uint8_t isopen);

#endif /* __UART_SERVO_LITE_H */
