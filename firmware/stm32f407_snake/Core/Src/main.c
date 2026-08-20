/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "ring_buffer.h"
#include "uart_servo_lite.h"
#include "test_servo.h"
#include "math.h"
#include "step.h"
#include "gait.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* ====== 蛇形机器人配置 ====== */
#define SERVO_ID_SETUP_MODE  0 // 1=按下方宏自动配置单舵机ID, 0=运行蛇形步态
#define TARGET_SERVO_ID      8
// 每次只连接一个舵机，在这里填写要写入的ID
#define SNAKE_JOINT_COUNT  4    // 当前只连接8只舵机，共4个正交双轴关节
#define GAIT_BASE_ID       1
#define SERPENTINE_AMPLITUDE_DEG        45.0f // 水平蜿蜒振幅，便于观察
#define SERPENTINE_PERIOD_S              2.0f // 一个完整摆动周期
#define SERPENTINE_JOINT_PHASE_DEG \
        (360.0f / SNAKE_JOINT_COUNT) // 4关节时相邻关节相差90°
#define STATUS_POLL_INTERVAL_MS        250U   // 每次只读一个舵机，降低对步态的影响
#define SERVO_CENTER_HOLD_MS          5000U   // 启动时所有舵机回中并保持5秒
#define SERVO_CENTER_RAW              2048U   // 舵机原始中位值
#define SERVO_CENTER_TOLERANCE_RAW      57U   // 约5°，超过则报告零位异常

#if (TARGET_SERVO_ID < 1) || (TARGET_SERVO_ID > 250)
#error "TARGET_SERVO_ID must be in range 1..250"
#endif

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern volatile uint32_t usart3_rx_count;  // USART3接收字节计数(调试用)

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// 使用TARGET_SERVO_ID自动配置单舵机ID并Ping验证
#if SERVO_ID_SETUP_MODE
static void RunServoIdSetup(void);
#endif
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/                                                                                                                                                                                                                                                                                                                                                                                               
/* USER CODE BEGIN 0 */
/* USART1 直接寄存器方式发送一个字节（绕过HAL，纯硬件测试） */
// static void USART1_SendByte(uint8_t byte)
// {
//     /* 等待TXE（发送数据寄存器空） */
//     while (!(USART1->SR & USART_SR_TXE)) {}
//     /* 写入数据寄存器 */
//     USART1->DR = byte;
// }

// ========== 角度换算：以中心(2048)为0°, 向左为负, 向右为正 ==========

// 舵机原始值(0-4095) → 相对中心的角度(°) 范围 -180°~+180°
static float RawToRelativeDegree(uint16_t raw)
{
    if (raw > 4095) return -999.0f;
    return raw * 360.0f / 4095.0f - 180.0f;
}

// 相对角度(°) → 舵机原始值(0-4095)
static uint16_t __attribute__((unused)) RelativeDegreeToRaw(float relDeg)
{
    float absDeg = relDeg + 180.0f;  // 转为 0~360°
    if (absDeg < 0.0f) absDeg = 0.0f;
    if (absDeg > 360.0f) absDeg = 360.0f;
    return (uint16_t)(absDeg * 4095.0f / 360.0f);
}

// 清空舵机接收缓冲区
static void __attribute__((unused)) ClearServoRxBuf(void)
{
    RingBuffer_Reset(servoUsart->recvBuf);
}

// // 调试: 打印USART3硬件寄存器状态和缓冲区内容
// static void USART3_DumpStatus(void)
// {
//     uint32_t sr = USART3->SR;
//     uint32_t cr1 = USART3->CR1;

//     printf("\r\n=== USART3 DEBUG ===\r\n");
//     printf("SR=0x%08lX", sr);
//     if (sr & USART_SR_PE)   printf(" PE");
//     if (sr & USART_SR_FE)   printf(" FE");
//     if (sr & USART_SR_NE)   printf(" NE");
//     if (sr & USART_SR_ORE)  printf(" ORE");
//     if (sr & USART_SR_IDLE) printf(" IDLE");
//     if (sr & USART_SR_RXNE) printf(" RXNE");
//     if (sr & USART_SR_TC)   printf(" TC");
//     if (sr & USART_SR_TXE)  printf(" TXE");
//     printf("\r\n");

//     printf("CR1=0x%08lX", cr1);
//     if (cr1 & USART_CR1_RE)     printf(" RE");
//     if (cr1 & USART_CR1_TE)     printf(" TE");
//     if (cr1 & USART_CR1_RXNEIE) printf(" RXNEIE");
//     if (cr1 & USART_CR1_PEIE)   printf(" PEIE");
//     printf("\r\n");

//     /* RXNE置位时，直接读DR看是否有数据卡住 */
//     if (sr & USART_SR_RXNE) {
//         uint8_t stuck_byte = (uint8_t)(USART3->DR & 0xFF);
//         printf("DR has stuck byte: 0x%02X\r\n", stuck_byte);
//         /* 注意: 读DR会清除RXNE */
//     }

//     /* 查看环形缓冲区状态 */
//     uint16_t used = RingBuffer_GetByteUsed(servoUsart->recvBuf);
//     uint16_t free = RingBuffer_GetByteFree(servoUsart->recvBuf);
//     printf("RingBuf: used=%u free=%u rx_count=%lu\r\n",
//            used, free, usart3_rx_count);

//     /* 打印缓冲区内容 */
//     if (used > 0) {
//         printf("RingBuf data:");
//         for (uint16_t i = 0; i < used && i < 64; i++) {
//             printf(" %02X", RingBuffer_GetValueByIndex(servoUsart->recvBuf, i));
//         }
//         printf("\r\n");
//     }
//     printf("==================\r\n");
// }

// 读取单个寄存器(通用), 返回读取到的值, 失败返回0xFFFF
static uint16_t __attribute__((unused)) ReadReg(uint8_t addr, uint8_t len)
{
    uint8_t content[2];
    
    content[0] = addr;
    content[1] = len;
    JOHO_PackageBuild_Send(servoUsart, 1, 4, CMDType_Read, content);

    SysTick_DelayMs(5);
    PackageTypeDef pkg;
    if (USL_RecvPackage(servoUsart, &pkg) == JOHO_STATUS_SUCCESS) {
        if (len == 1) return pkg.content[0];
        // 大端模式: content[0]=高字节, content[1]=低字节
        return (pkg.content[0] << 8) | pkg.content[1];
    }
    return 0xFFFF;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* === LED 初始化（正点原子F407: DS0=PF9, 低电平亮）=== */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  GPIO_InitTypeDef led = {0};
  led.Pin = GPIO_PIN_9;
  led.Mode = GPIO_MODE_OUTPUT_PP;
  led.Pull = GPIO_NOPULL;
  led.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &led);

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */

  /* === 初始化舵机USART === */
  USART_InitServoUsart(&huart3);

#if SERVO_ID_SETUP_MODE
  /*
   * ID配置模式不会进入步态。程序使用TARGET_SERVO_ID广播修改当前
   * 唯一连接舵机的ID，并自动Ping新ID验证。
   */
  RunServoIdSetup();
#endif

  /* ============================================================
   *  蛇形机器人水平蜿蜒步态调试
   *
   *  配置宏在 USER CODE BEGIN PD 中修改:
   *    SNAKE_JOINT_COUNT   正交双舵机关节模块数
   *    GAIT_BASE_ID        起始舵机ID
   * ============================================================ */
  printf("\r\n========================================\r\n");
  printf("  Planar Serpentine Gait Debug\r\n");
  printf("========================================\r\n");
  printf("  Joints: %u, Base ID: %u\r\n",
         SNAKE_JOINT_COUNT, GAIT_BASE_ID);
  printf("  Status poll: one servo / %u ms\r\n", STATUS_POLL_INTERVAL_MS);
  printf("========================================\r\n\n");

  GaitController gc;
  Gait_Init(&gc, SNAKE_JOINT_COUNT);

  /*
   * 当前实机观察到ID1、ID5产生上下弯曲，因此交换每对舵机的轴映射:
   * 偶数ID作为平面蜿蜒水平轴H，奇数ID作为保持中位的垂直轴V。
   * Joint0 H=2/V=1, Joint1 H=4/V=3,
   * Joint2 H=6/V=5, Joint3 H=8/V=7。
   */
  for (uint8_t joint = 0; joint < SNAKE_JOINT_COUNT; joint++) {
      uint8_t vertical_id = GAIT_BASE_ID + joint * 2;
      uint8_t horizontal_id = vertical_id + 1;
      Gait_SetJointMappingEx(
          &gc, joint,
          horizontal_id, vertical_id,
          1, 1,
          0.0f, 0.0f);
  }

  /*
   * 水平蜿蜒：水平轴形成行波，垂直轴始终保持中位。
   * 45°大振幅便于辨认轴映射和运动方向。
   */
  Gait_ConfigurePlanarSerpentine(
      &gc,
      SERPENTINE_AMPLITUDE_DEG,
      SERPENTINE_PERIOD_S,
      SERPENTINE_JOINT_PHASE_DEG);
  Gait_SetRampDuration(&gc, 3000);
  Gait_Restart(&gc);

  uint8_t map_status = Gait_ValidateMapping(&gc);
  printf("[Map] validate=%u (0=OK)\r\n", map_status);
  for (uint8_t joint = 0; joint < gc.joint_count; joint++) {
      printf("[Map] Joint%u H=ID%u(dir=%d,off=%.1f) "
             "V=ID%u(dir=%d,off=%.1f)\r\n",
             joint,
             gc.mapping[joint].yaw_servo_id,
             gc.mapping[joint].yaw_direction,
             gc.mapping[joint].yaw_offset_deg,
             gc.mapping[joint].pitch_servo_id,
             gc.mapping[joint].pitch_direction,
             gc.mapping[joint].pitch_offset_deg);
  }

  /* 正常步态调试不依赖舵机回包；直接打开映射中所有舵机扭矩。 */
  uint8_t center_ids[GAIT_MAX_JOINTS * 2];
  uint16_t center_positions[GAIT_MAX_JOINTS * 2];
  uint16_t center_intervals[GAIT_MAX_JOINTS * 2];
  uint8_t center_count = 0;

  for (uint8_t joint = 0; joint < gc.joint_count; joint++) {
      uint8_t ids[2] = {
          gc.mapping[joint].yaw_servo_id,
          gc.mapping[joint].pitch_servo_id
      };
      for (uint8_t axis = 0; axis < 2; axis++) {
          if (ids[axis] == 0) continue;
          SET_Torque(servoUsart, ids[axis], 1);
          center_ids[center_count] = ids[axis];
          center_positions[center_count] = SERVO_CENTER_RAW;
          center_intervals[center_count] = 1000;
          center_count++;
      }
  }

  printf("[Init] Centering %u servos and holding for %lu ms...\r\n",
         center_count, (unsigned long)SERVO_CENTER_HOLD_MS);
  if (center_count > 0) {
      USL_SyncWriteAngles(servoUsart,
                          center_ids,
                          center_positions,
                          center_intervals,
                          center_count);
  }
  SysTick_DelayMs(SERVO_CENTER_HOLD_MS);

  /* 回中完成后逐个检查实际位置，找出未到达中位的舵机。 */
  printf("[ZeroCheck] Expected raw=%u, tolerance=+/-%.1f deg\r\n",
         SERVO_CENTER_RAW,
         SERVO_CENTER_TOLERANCE_RAW * 360.0f / 4095.0f);
  uint8_t zero_error_count = 0;
  for (uint8_t i = 0; i < center_count; i++) {
      uint16_t position = USL_GETPositionVal(servoUsart, center_ids[i]);
      if (position == 0xFFFF) {
          printf("[ZeroCheck] ID=%u READ_FAIL\r\n", center_ids[i]);
          zero_error_count++;
          continue;
      }

      uint16_t error_raw = position > SERVO_CENTER_RAW
                               ? position - SERVO_CENTER_RAW
                               : SERVO_CENTER_RAW - position;
      float error_deg = ((int32_t)position - (int32_t)SERVO_CENTER_RAW)
                        * 360.0f / 4095.0f;
      printf("[ZeroCheck] ID=%u raw=%u error=%+.1f deg %s\r\n",
             center_ids[i], position, error_deg,
             error_raw > SERVO_CENTER_TOLERANCE_RAW ? "ERROR" : "OK");
      if (error_raw > SERVO_CENTER_TOLERANCE_RAW) {
          zero_error_count++;
      }
  }
  printf("[ZeroCheck] Result: %u abnormal/read-failed servo(s)\r\n",
         zero_error_count);

  Gait_Restart(&gc);
  printf("[Init] Center done, start planar serpentine gait.\r\n");

  uint32_t last_status_poll = 0;
  uint8_t status_cursor = 0;
  uint32_t last_debug_print = 0;

  while (1)
  {
    uint32_t tick = HAL_GetTick();

    /* 同步写让映射中的所有舵机在同一控制周期更新目标位置 */
    Gait_UpdateSync(&gc, tick);

    /* LED闪烁: 指示程序运行中 (1Hz) */
    if ((tick / 500) % 2 == 0) {
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);  // DS0亮
    } else {
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_SET);    // DS0灭
    }

    if (tick - last_debug_print >= 1000U) {
        last_debug_print = tick;
        printf("[Run] tick=%lu ms usart3_rx=%lu\r\n",
               tick, usart3_rx_count);
    }

    /*
     * 角度/电流读取会占用半双工总线，所以低频、轮转读取。
     * 即使读取失败，也继续发送螺旋翻滚步态。
     */
    if (tick - last_status_poll >= STATUS_POLL_INTERVAL_MS) {
        last_status_poll = tick;

        uint8_t slot_count = gc.joint_count * 2;
        uint8_t slot = status_cursor;
        status_cursor = (uint8_t)((status_cursor + 1) % slot_count);

        uint8_t joint = slot / 2;
        uint8_t axis = slot & 1;
        uint8_t id = axis == 0
                         ? gc.mapping[joint].yaw_servo_id
                         : gc.mapping[joint].pitch_servo_id;

        if (id != 0) {
            uint16_t position = USL_GETPositionVal(servoUsart, id);
            int16_t current = USL_GetCurrent(servoUsart, id);

            if (position != 0xFFFF && current != (int16_t)0xFFFF) {
                printf("[Status] Joint%u %s ID=%u angle=%+.1fdeg raw=%u current=%d\r\n",
                       joint, axis == 0 ? "H" : "V", id,
                       RawToRelativeDegree(position), position, current);
            } else {
                printf("[Status] Joint%u %s ID=%u READ_FAIL pos=%u current=%d\r\n",
                       joint, axis == 0 ? "H" : "V", id, position, current);
            }
        }
    }

    /* 控制周期 ~20ms */
    SysTick_DelayMs(20);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
 * @brief  使用TARGET_SERVO_ID配置单个舵机ID，并Ping新ID验证。
 *
 * 使用时舵机总线上只能连接一只舵机。每次修改TARGET_SERVO_ID并
 * 重新编译烧录；程序启动后自动写ID，然后Ping三次。
 */
#if SERVO_ID_SETUP_MODE
static void RunServoIdSetup(void)
{
    printf("\r\n\r\n");
    printf("============================================\r\n");
    printf("  AUTO SERVO ID SETUP + PING VERIFY\r\n");
    printf("============================================\r\n");
    printf("  Target ID: %u\r\n", TARGET_SERVO_ID);
    printf("  WARNING  : Connect ONLY ONE servo!\r\n");
    printf("============================================\r\n");

    uint8_t write_content[2] = {0x05, TARGET_SERVO_ID};

    printf("[1/3] Broadcast write: servo ID -> %u\r\n", TARGET_SERVO_ID);
    RingBuffer_Reset(servoUsart->recvBuf);
    JOHO_PackageBuild_Send(servoUsart, JOHO_BROADCAST_ID, 4,
                           CMDType_Write, write_content);

    printf("[2/3] Waiting for EEPROM save ...\r\n");
    SysTick_DelayMs(1000);

    JOHO_STATUS ping_status = JOHO_STATUS_FAIL;
    for (uint8_t attempt = 1; attempt <= 3; attempt++)
    {
        printf("[3/3] Ping ID=%u, attempt %u/3 ... ",
               TARGET_SERVO_ID, attempt);
        ping_status = US_Ping(servoUsart, TARGET_SERVO_ID);
        if (ping_status == JOHO_STATUS_SUCCESS)
        {
            printf("OK\r\n");
            break;
        }

        printf("FAILED (status=%u)\r\n", ping_status);
        SysTick_DelayMs(100);
    }

    if (ping_status == JOHO_STATUS_SUCCESS)
    {
        printf("\r\n[SUCCESS] Servo ID is %u and Ping is valid.\r\n",
               TARGET_SERVO_ID);
    }
    else
    {
        printf("\r\n[FAILED] ID=%u did not answer Ping.\r\n",
               TARGET_SERVO_ID);
        printf("Check single-servo wiring and power, then reset the board.\r\n");
    }

    printf("Change TARGET_SERVO_ID and re-flash for the next servo.\r\n");
    printf("Power off before replacing the servo.\r\n");

    /* 配置只执行一次，避免循环擦写舵机EEPROM。 */
    while (1)
    {
        SysTick_DelayMs(1000);
    }
}
#endif

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
