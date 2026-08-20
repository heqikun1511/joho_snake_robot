/**
 * test_servo.c - 舵机通讯裸数据测试
 * 使用方法: 在 main.c 中 #include 此文件，在 USER CODE 中调用 RunServoTest()
 * 
 * 此测试会:
 * 1. 发送一个0xFF字节到舵机（唤醒/同步）
 * 2. 发送一个Ping指令
 * 3. 打印接收到的所有原始字节（不解析、不清除）
 */

#include "main.h"
#include "usart.h"
#include "ring_buffer.h"
#include "uart_servo_lite.h"
#include <stdio.h>

extern Usart_DataTypeDef *servoUsart;
extern volatile uint32_t usart3_rx_count;

// 读取环形缓冲区中所有可用字节并打印
static void DumpAllRxBytes(const char *label)
{
    uint16_t available = RingBuffer_GetByteUsed(servoUsart->recvBuf);
    printf("[%s] RX count=%lu, buffer has %u bytes: ", 
           label, usart3_rx_count, available);
    
    if (available == 0) {
        printf("(empty)\r\n");
        return;
    }
    
    // 读取所有字节
    for (uint16_t i = 0; i < available; i++) {
        uint8_t b = RingBuffer_ReadByte(servoUsart->recvBuf);
        printf("%02X ", b);
    }
    printf("\r\n");
}

// 裸数据测试函数
void RunServoTest(void)
{
    printf("\r\n========================================\r\n");
    printf("  Servo UART Raw Data Test\r\n");
    printf("========================================\r\n");
    
    // 先清空缓冲区
    RingBuffer_Reset(servoUsart->recvBuf);
    uint32_t old_count = usart3_rx_count;
    
    // === 测试1: 发送单个0xFF字节 ===
    printf("\r\n[Test 1] Send single 0xFF byte...\r\n");
    uint8_t syncByte = 0xFF;
    USL_Send_HEX(servoUsart, 1, &syncByte);
    SysTick_DelayMs(50);
    DumpAllRxBytes("After 0xFF");
    
    // === 测试2: 发送Ping指令 (ID=1) ===
    printf("\r\n[Test 2] Send Ping ID=1 (no buffer clear)...\r\n");
    RingBuffer_Reset(servoUsart->recvBuf);
    
    // 手动构建并发送Ping包: FF FF 01 02 01 <cs>
    uint8_t pingPkt[6];
    pingPkt[0] = 0xFF;
    pingPkt[1] = 0xFF;
    pingPkt[2] = 0x01;  // ID=1
    pingPkt[3] = 0x02;  // size=2
    pingPkt[4] = 0x01;  // CMDType_Ping
    // 计算校验和: ~(0x01 + 0x02 + 0x01) & 0xFF = ~0x04 & 0xFF = 0xFB
    pingPkt[5] = (~(pingPkt[2] + pingPkt[3] + pingPkt[4])) & 0xFF;
    
    printf("  Sending: ");
    for (int i = 0; i < 6; i++) printf("%02X ", pingPkt[i]);
    printf("\r\n");
    
    USL_Send_HEX(servoUsart, 6, pingPkt);
    
    SysTick_DelayMs(50);
    DumpAllRxBytes("After Ping");
    
    SysTick_DelayMs(100);
    DumpAllRxBytes("After +100ms");
    
    // === 测试3: 先广播扭矩使能,再Ping ===
    printf("\r\n[Test 3] Torque enable (broadcast ID=254), then Ping ID=1...\r\n");
    RingBuffer_Reset(servoUsart->recvBuf);
    
    // 扭矩使能: FF FF FE 04 03 28 01 00(checksum)
    uint8_t torquePkt[8];
    torquePkt[0] = 0xFF; torquePkt[1] = 0xFF;
    torquePkt[2] = 0xFE;  // 广播ID=254
    torquePkt[3] = 0x04;  // size=4
    torquePkt[4] = 0x03;  // CMDType_Write
    torquePkt[5] = 0x28;  // Torque register
    torquePkt[6] = 0x01;  // Torque ON
    torquePkt[7] = (~(torquePkt[2] + torquePkt[3] + torquePkt[4] + torquePkt[5] + torquePkt[6])) & 0xFF;
    
    printf("  Torque cmd: ");
    for (int i = 0; i < 8; i++) printf("%02X ", torquePkt[i]);
    printf("\r\n");
    USL_Send_HEX(servoUsart, 8, torquePkt);
    SysTick_DelayMs(200);  // 等扭矩生效
    
    // 清空缓存,重新Ping
    RingBuffer_Reset(servoUsart->recvBuf);
    USL_Send_HEX(servoUsart, 6, pingPkt);
    SysTick_DelayMs(50);
    DumpAllRxBytes("After Torque+Ping");
    SysTick_DelayMs(200);
    DumpAllRxBytes("After Torque+Ping+200ms");
    
    // === 测试4: 扭矩开后发送Read指令 ===
    printf("\r\n[Test 4] Send Read ID=1 reg=0x38 len=2 (after torque ON)...\r\n");
    RingBuffer_Reset(servoUsart->recvBuf);
    
    uint8_t readPkt[8];
    readPkt[0] = 0xFF; readPkt[1] = 0xFF;
    readPkt[2] = 0x01;  // ID=1
    readPkt[3] = 0x04;  // size=
    readPkt[4] = 0x02;  // CMDType_Read
    readPkt[5] = 0x38;  // 寄存器地址(角度)
    readPkt[6] = 0x02;  // 读取2字节
    readPkt[7] = (~(readPkt[2] + readPkt[3] + readPkt[4] + readPkt[5] + readPkt[6])) & 0xFF;
    
    printf("  Sending: ");
    for (int i = 0; i < 8; i++) printf("%02X ", readPkt[i]);
    printf("\r\n");
    
    USL_Send_HEX(servoUsart, 8, readPkt);
    
    SysTick_DelayMs(50);
    DumpAllRxBytes("After Read +50ms");
    SysTick_DelayMs(200);
    DumpAllRxBytes("After Read +250ms");
    
    printf("\r\n========================================\r\n");
    printf("  Raw Data Test Complete\r\n");
    printf("========================================\r\n");
}

/**
 * @brief Scan servo IDs 1~20 using Ping.
 *
 * Ping works now (Test 2 shows FF F5 01 02 00 FC).
 * Strategy: clear → send Ping → wait 10ms → USL_RecvPackage.
 * USL_RecvPackage's frame detector skips echo bytes and finds FF F5.
 *
 * @param found_ids Output: array of found servo IDs
 * @param max_scan  Max ID to scan
 * @return Number of servos found
 */
uint8_t ScanAllServoIDs(uint8_t *found_ids, uint8_t max_scan)
{
    uint8_t count = 0;
    
    printf("\r\n========================================\r\n");
    printf("  Servo Ping Scan (1~%u)\r\n", max_scan);
    printf("========================================\n\n");
    
    for (uint8_t id = 1; id <= max_scan; id++) {
        RingBuffer_Reset(servoUsart->recvBuf);
        
        /* Send Ping */
        JOHO_PackageBuild_Send(servoUsart, id, 2, CMDType_Ping, NULL);
        SysTick_DelayMs(10);
        
        /* Receive response (USL_RecvPackage skips echo, finds FF F5) */
        PackageTypeDef pkg;
        JOHO_STATUS status = USL_RecvPackage(servoUsart, &pkg);
        
        printf("  ID=%2u: ", id);
        if (status == JOHO_STATUS_SUCCESS && pkg.usId == id) {
            found_ids[count++] = id;
            printf("OK\n");
        } else {
            printf("err=%d\n", status);
        }
    }
    
    printf("\n========================================\n");
    printf("  Found %u servo(s)\n", count);
    if (count > 0) {
        printf("  Servo IDs: ");
        for (uint8_t i = 0; i < count; i++) {
            printf("%u", found_ids[i]);
            if (i < count - 1) printf(", ");
        }
        printf("\n");
    }
    printf("========================================\n");
    
    return count;
}

/* Scan raw bytes for FF F5 pattern (more robust than USL_RecvPackage) */
static uint8_t ScanRawForFF_F5(uint8_t servo_id)
{
    uint16_t avail = RingBuffer_GetByteUsed(servoUsart->recvBuf);
    if (avail < 4) return 0;  /* need at least header(2)+ID(1)+size(1) */
    
    for (uint16_t i = 0; i < avail - 1; i++) {
        uint8_t b0 = RingBuffer_GetValueByIndex(servoUsart->recvBuf, i);
        uint8_t b1 = RingBuffer_GetValueByIndex(servoUsart->recvBuf, i + 1);
        
        /* Check for FF F5 (both byte orders) */
        if ((b0 == 0xF5 && b1 == 0xFF) || (b0 == 0xFF && b1 == 0xF5)) {
            /* Found header, check ID at position i+2 */
            if (i + 2 < avail) {
                uint8_t id = RingBuffer_GetValueByIndex(servoUsart->recvBuf, i + 2);
                if (id == servo_id) return 1;
            }
        }
    }
    return 0;
}

/**
 * @brief Detect all servos on the bus and assign sequential IDs.
 *
 * Handles the case where multiple servos all have the factory default ID=1.
 * Uses raw byte scanning for FF F5 instead of USL_RecvPackage for robustness.
 *
 * Process:
 *   Phase 1: Repeatedly Ping ID=1, relocate found servo to temp ID
 *   Phase 2: Reassign all to sequential IDs (1,2,3...)
 *
 * @return Number of servos detected
 */
uint8_t DetectAndAssignIDs(void)
{
    uint8_t found = 0;
    uint8_t temp_id = 200;
    
    printf("\r\n========================================\r\n");
    printf("  Servo ID Detection & Assignment\r\n");
    printf("========================================\n");
    printf("  Scanning for servos with duplicate ID=1...\n\n");
    
    /* ===== Phase 1: Relocate each ID=1 servo to a temp ID ===== */
    while (1) {
        uint8_t detected = 0;
        
        /* Try up to 3 times with longer wait for servo to respond */
        for (uint8_t attempt = 0; attempt < 3; attempt++) {
            RingBuffer_Reset(servoUsart->recvBuf);
            JOHO_PackageBuild_Send(servoUsart, 1, 2, CMDType_Ping, NULL);
            SysTick_DelayMs(100);  /* longer wait for echo + servo response */
            
                if (ScanRawForFF_F5(1)) {
                detected = 1;
                break;
            }
        }
        
        if (!detected) {
            break;  /* No more servo at ID=1 */
        }
        
        found++;
        printf("  Found servo #%u at ID=1, moving to ID=%u... ", found, temp_id);
        
        /* Write new ID to register 0x05 */
        uint8_t content[2] = {0x05, temp_id};
        JOHO_PackageBuild_Send(servoUsart, 1, 4, CMDType_Write, content);
        SysTick_DelayMs(50);  /* Give servo time to save to EEPROM */
        
        /* Verify: Ping the new ID using raw scan */
        RingBuffer_Reset(servoUsart->recvBuf);
        JOHO_PackageBuild_Send(servoUsart, temp_id, 2, CMDType_Ping, NULL);
        SysTick_DelayMs(100);
        
        if (ScanRawForFF_F5(temp_id)) {
            printf("OK\n");
        } else {
            printf("verify failed\n");
        }
        
        temp_id++;
        if (found >= 8 || temp_id > 250) {
            printf("  Reached safety limit (%u servos)\n", found);
            break;
        }
    }
    
    printf("\n  Total servos detected: %u\n\n", found);
    
    /* ===== Phase 2: Reassign to sequential IDs starting from 1 ===== */
    if (found > 0) {
        printf("  Reassigning to sequential IDs...\n");
        for (uint8_t i = 0; i < found; i++) {
            uint8_t old_id = 200 + i;
            uint8_t new_id = i + 1;
            
            uint8_t wc[2] = {0x05, new_id};
            JOHO_PackageBuild_Send(servoUsart, old_id, 4, CMDType_Write, wc);
            SysTick_DelayMs(50);
            
            /* Verify */
            RingBuffer_Reset(servoUsart->recvBuf);
            JOHO_PackageBuild_Send(servoUsart, new_id, 2, CMDType_Ping, NULL);
            SysTick_DelayMs(100);
            
            if (ScanRawForFF_F5(new_id)) {
                printf("  ID=%u -> ID=%u: OK\n", old_id, new_id);
            } else {
                printf("  ID=%u -> ID=%u: failed\n", old_id, new_id);
            }
        }
    } else {
        printf("  No servos found at ID=1.\n");
    }
    
    printf("\r\n========================================\r\n");
    return found;
}

/**
 * @brief Set ALL servos on the bus to a specific ID using broadcast.
 *
 * IMPORTANT: Only connect ONE servo to the bus when calling this!
 * Broadcast ID=254 sends to ALL servos simultaneously.
 *
 * @param new_id Target ID (1~250)
 */
void SetServoID_Broadcast(uint8_t new_id)
{
    uint8_t content[2] = {0x05, new_id};
    /* FF FF FE 04 03 05 <new_id> <CS> */
    JOHO_PackageBuild_Send(servoUsart, 0xFE, 4, CMDType_Write, content);
    SysTick_DelayMs(100);
    printf("  Broadcast: set servo ID -> %u\n", new_id);
}

/**
 * @brief 测试扫描总线上所有ID=1的舵机
 *
 * 场景：总线上可能有多个舵机，出厂默认ID都是1。
 * 本测试逐个探测并临时移走，统计总数。
 *
 * 流程：
 *   1. Ping ID=1，如果能收到回复 → 找到1个舵机
 *   2. 将该舵机的ID改为临时ID (200, 201, 202...)
 *   3. 重复步骤1~2，直到Ping ID=1无回复
 *   4. 统计并打印找到的总数
 *   5. 最后Ping各临时ID验证
 *
 * @return 检测到的舵机总数
 */
uint8_t TestScanAllID1Servos(void)
{
    uint8_t found = 0;
    uint8_t temp_id_base = 200;  /* 临时ID起始值 */
    uint8_t max_scan = 8;        /* 最多扫描8个 */

    printf("\r\n============================================\r\n");
    printf("  Test: Scan All ID=1 Servos on Bus\r\n");
    printf("============================================\r\n");
    printf("  Senerio: Multiple servos with ID=1\r\n");
    printf("  Strategy: Ping → relocate → repeat\r\n\n");

    /* ===== Phase 1: 逐个探测并移走 ===== */
    for (uint8_t i = 0; i < max_scan; i++) {
        uint8_t detected = 0;

        /* 尝试多次Ping以确保能收到回复 */
        for (uint8_t attempt = 0; attempt < 3; attempt++) {
            RingBuffer_Reset(servoUsart->recvBuf);

            /* 发Ping到ID=1 */
            JOHO_PackageBuild_Send(servoUsart, 1, 2, CMDType_Ping, NULL);
            SysTick_DelayMs(50);

            /* 用原始字节扫描FF F5响应头 */
            if (ScanRawForFF_F5(1)) {
                detected = 1;
                break;
            }
        }

        if (!detected) {
            printf("  [%u] No more servo at ID=1, scan complete.\n", i);
            break;
        }

        /* 找到1个舵机，移到临时ID */
        uint8_t temp_id = temp_id_base + i;
        found++;

        printf("  [%u] Found #%u at ID=1 → moving to ID=%u ... ",
               i, found, temp_id);

        /* 写入新ID到寄存器0x05 */
        uint8_t content[2] = {0x05, temp_id};
        JOHO_PackageBuild_Send(servoUsart, 1, 4, CMDType_Write, content);
        SysTick_DelayMs(100);  /* 给舵机时间保存到EEPROM */

        /* ===== 验证: Ping临时ID ===== */
        RingBuffer_Reset(servoUsart->recvBuf);
        JOHO_PackageBuild_Send(servoUsart, temp_id, 2, CMDType_Ping, NULL);
        SysTick_DelayMs(50);

        if (ScanRawForFF_F5(temp_id)) {
            printf("OK (verified at ID=%u)\n", temp_id);
        } else {
            printf("WARNING: verify failed!\n");
        }
    }

    /* ===== Phase 2: 报告结果 ===== */
    printf("\n============================================\n");
    printf("  Scan Result\n");
    printf("============================================\n");
    if (found > 0) {
        printf("  Total servos found: %u\n", found);
        printf("  Assigned temp IDs: ");
        for (uint8_t i = 0; i < found; i++) {
            printf("%u", temp_id_base + i);
            if (i < found - 1) printf(", ");
        }
        printf("\n");

        /* ===== 验证所有临时ID ===== */
        printf("\n  --- Verification Ping ---\n");
        for (uint8_t i = 0; i < found; i++) {
            uint8_t tid = temp_id_base + i;
            RingBuffer_Reset(servoUsart->recvBuf);
            JOHO_PackageBuild_Send(servoUsart, tid, 2, CMDType_Ping, NULL);
            SysTick_DelayMs(50);

            if (ScanRawForFF_F5(tid)) {
                printf("  Ping ID=%u: OK\n", tid);
            } else {
                printf("  Ping ID=%u: FAIL\n", tid);
            }
        }
    } else {
        printf("  No servos found!\n");
        printf("  Please check:\n");
        printf("    1. Servo power supply\n");
        printf("    2. USART3 wiring (PB10=TX, PB11=RX)\n");
        printf("    3. Level shifter / conversion board\n");
    }
    printf("============================================\n");

    return found;
}

/**
 * @brief 修改单个舵机的ID
 *
 * 向指定舵机发送写寄存器指令，将ID改为新值。
 * 过程:
 *   1. 发送 Write(reg=0x05, new_id) 到 old_id
 *   2. 等待EEPROM写入完成 (100ms)
 *   3. Ping 新ID 验证
 *
 * @param old_id  当前舵机ID
 * @param new_id  新ID (1~250)
 * @return JOHO_STATUS_SUCCESS=成功, 其他=失败
 */
JOHO_STATUS SetServoID_Single(uint8_t old_id, uint8_t new_id)
{
    printf("\r\n[SetServoID] Changing ID: %u -> %u ... ", old_id, new_id);

    if (old_id < 1 || old_id > 250 || new_id < 1 || new_id > 250) {
        printf("ERR: invalid ID range (must be 1~250)\r\n");
        return JOHO_STATUS_FAIL;
    }

    /* 清空接收缓冲区 */
    RingBuffer_Reset(servoUsart->recvBuf);

    /* 发送写ID指令: Write reg=0x05, value=new_id */
    uint8_t content[2] = {0x05, new_id};
    JOHO_PackageBuild_Send(servoUsart, old_id, 4, CMDType_Write, content);

    /* 等待舵机写入EEPROM (至少需要50ms) */
    SysTick_DelayMs(100);

    /* ===== 验证: Ping 新ID ===== */
    RingBuffer_Reset(servoUsart->recvBuf);
    JOHO_PackageBuild_Send(servoUsart, new_id, 2, CMDType_Ping, NULL);
    SysTick_DelayMs(10);

    PackageTypeDef pkg;
    JOHO_STATUS status = USL_RecvPackage(servoUsart, &pkg);

    if (status == JOHO_STATUS_SUCCESS && pkg.usId == new_id) {
        printf("OK (verified at ID=%u)\r\n", new_id);
        return JOHO_STATUS_SUCCESS;
    } else {
        printf("FAIL (err=%d)\r\n", status);
        return status;
    }
}

/**
 * @brief 交互式逐舵机修改ID
 *
 * 适用于新舵机(出厂默认ID=1)或已知ID的舵机。
 * 流程:
 *   1. 先扫描查找当前总线上有哪些舵机
 *   2. 逐个询问用户希望把该舵机改成什么ID
 *   3. 执行修改并验证
 *
 * 注意: 此函数会自动扫描ID 1~20并列出在线舵机。
 *       由于无法在嵌入式端等待用户输入，此版本使用
 *       预定义的映射表(从ID 1~N 映射到用户指定的新ID列表)。
 *
 * @param old_ids      当前舵机ID列表
 * @param new_ids      目标舵机ID列表
 * @param count        舵机数量
 * @return 成功修改的数量
 */
uint8_t SetServoIDs_FromList(uint8_t *old_ids, uint8_t *new_ids, uint8_t count)
{
    uint8_t success = 0;

    printf("\r\n========================================\r\n");
    printf("  Batch Set Servo ID (one at a time)\r\n");
    printf("========================================\r\n");

    for (uint8_t i = 0; i < count; i++) {
        JOHO_STATUS st = SetServoID_Single(old_ids[i], new_ids[i]);
        if (st == JOHO_STATUS_SUCCESS) {
            success++;
        }
        /* 每次修改间隔，确保EEPROM稳定 */
        SysTick_DelayMs(50);
    }

    printf("\r\n========================================\r\n");
    printf("  Done: %u/%u IDs changed successfully\r\n", success, count);
    printf("========================================\r\n");

    return success;
}

/**
 * @brief 演示: 逐个修改舵机ID
 *
 * 这是一个示例程序，展示如何一步步修改舵机ID。
 * 修改流程:
 *   步骤1: 扫描当前总线，发现所有舵机
 *   步骤2: 将扫描到的舵机逐个改成指定的新ID
 *   步骤3: 验证所有新ID
 *
 * 使用方法:
 *   在 main.c 中调用 RunSetServoID_Demo()；
 *   或自定义 old_ids/new_ids 数组后调用 SetServoIDs_FromList()。
 */
void RunSetServoID_Demo(void)
{
    printf("\r\n========================================\r\n");
    printf("  Servo ID Modification Demo\r\n");
    printf("========================================\r\n");
    printf("\r\n  [Step 1] Scan bus for servos...\r\n");

    /* 扫描总线上有哪些舵机 */
    uint8_t found_ids[20];
    uint8_t found = ScanAllServoIDs(found_ids, 20);

    if (found == 0) {
        printf("\r\n  No servos found. Aborting.\r\n");
        return;
    }

    printf("\r\n  [Step 2] Change IDs one by one...\r\n");
    printf("  Found %u servo(s). Press Enter after each change.\r\n", found);

    /* === 修改方案: 将扫描到的舵机重新编号为 1,2,3... ===
     *
     * 示例: 场景1 - 出厂默认ID都是1的舵机
     *   逐个Ping ID=1 → 改到临时ID → 最后统一分配
     *   (此场景建议使用 TestScanAllID1Servos())
     *
     * 示例: 场景2 - 已知不同ID的舵机，重新规划ID
     *   例如扫描到 ID=[1,3,5], 想改成 [2,4,6]
     */
    printf("\r\n  --- Option A: Sequential reassignment ---\r\n");
    uint8_t success_a = 0;
    for (uint8_t i = 0; i < found; i++) {
        uint8_t new_id = i + 1;  /* 按顺序改为 1,2,3... */
        JOHO_STATUS st = SetServoID_Single(found_ids[i], new_id);
        if (st == JOHO_STATUS_SUCCESS) {
            found_ids[i] = new_id;  /* 更新本地记录的ID */
            success_a++;
        }
        SysTick_DelayMs(50);
    }
    printf("\r\n  Option A result: %u/%u OK\r\n", success_a, found);

    /* ===== 验证所有新ID ===== */
    printf("\r\n  [Step 3] Verification ping...\r\n");
    uint8_t verified = 0;
    for (uint8_t i = 0; i < found; i++) {
        RingBuffer_Reset(servoUsart->recvBuf);
        JOHO_PackageBuild_Send(servoUsart, found_ids[i], 2, CMDType_Ping, NULL);
        SysTick_DelayMs(10);

        PackageTypeDef pkg;
        if (USL_RecvPackage(servoUsart, &pkg) == JOHO_STATUS_SUCCESS &&
            pkg.usId == found_ids[i]) {
            printf("  Ping ID=%u: OK\r\n", found_ids[i]);
            verified++;
        } else {
            printf("  Ping ID=%u: FAIL\r\n", found_ids[i]);
        }
    }
    printf("\r\n========================================\r\n");
    printf("  Demo complete: %u/%u verified\r\n", verified, found);
    printf("========================================\r\n");
}
