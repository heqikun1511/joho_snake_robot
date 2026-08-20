#ifndef __TEST_SERVO_H
#define __TEST_SERVO_H

#include <stdint.h>
#include "uart_servo_lite.h"

void RunServoTest(void);

/**
 * @brief 扫描舵机ID（1~max_scan），报告所有响应舵机
 * @param found_ids    输出：找到的舵机ID数组
 * @param max_scan     最大扫描ID
 * @return 找到的舵机数量
 */
uint8_t ScanAllServoIDs(uint8_t *found_ids, uint8_t max_scan);

/**
 * @brief 检测总线上所有舵机（处理多个舵机都是ID=1的情况）
 *        逐个把ID=1的舵机挪到临时ID，统计数量，最后重新分配为1,2,3...
 * @return 检测到的舵机总数
 */
uint8_t DetectAndAssignIDs(void);

/**
 * @brief 通过广播ID(254)设置总线上的舵机ID
 *        使用前确保总线上只有1个舵机！
 * @param new_id 新ID (1~250)
 */
void SetServoID_Broadcast(uint8_t new_id);

/**
 * @brief 测试扫描总线上所有ID=1的舵机（出厂默认ID）
 *        逐个Ping→移走→统计总数
 * @return 检测到的舵机总数
 */
uint8_t TestScanAllID1Servos(void);

/**
 * @brief 修改单个舵机的ID
 * @param old_id  当前舵机ID (1~250)
 * @param new_id  新ID (1~250)
 * @return JOHO_STATUS_SUCCESS=成功, 其他=失败
 */
JOHO_STATUS SetServoID_Single(uint8_t old_id, uint8_t new_id);

/**
 * @brief 批量逐个修改舵机ID (依次修改，一次一个)
 * @param old_ids  当前舵机ID列表
 * @param new_ids  目标舵机ID列表
 * @param count    舵机数量
 * @return 成功修改的数量
 */
uint8_t SetServoIDs_FromList(uint8_t *old_ids, uint8_t *new_ids, uint8_t count);

/**
 * @brief 舵机ID修改演示程序
 *        自动扫描→逐个修改→验证结果
 */
void RunSetServoID_Demo(void);

#endif
