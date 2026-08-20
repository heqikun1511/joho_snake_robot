#ifndef __GAIT_H
#define __GAIT_H

#include <stdint.h>
#include "usart.h"     /* Usart_DataTypeDef, servoUsart */

/* ================================================================
 * 蛇形机器人步态控制模块
 *
 * 核心公式 (弧度制):
 *   θ_yi = α_y · cos(ω_y · t + (i-1) · β_y) + γ_y
 *   θ_pi = α_p · cos(ω_p · t + (i-1) · β_p + φ) + γ_p
 *
 * 其中 i = 蛇身关节模块编号(1-based, 从头到尾), t = 时间(秒)
 *
 * 每个关节模块由两个正交舵机组成:
 *   Horizontal/Yaw: 水平弯曲轴
 *   Vertical/Pitch: 垂直弯曲轴
 *
 * ID映射示例:
 *   Joint0 → Horizontal舵机ID=1, Vertical舵机ID=2
 *   Joint1 → Horizontal舵机ID=3, Vertical舵机ID=4
 *   ...
 *
 * 使用方法:
 *   1. Gait_Init() 初始化
 *   2. Gait_SetJointMapping() 设置每个关节模块的物理舵机ID
 *   3. Gait_SetParams() 设置步态参数
 *   4. 在主循环中周期调用 Gait_Update( HAL_GetTick() )
 * ================================================================ */

/* ========== 常量 ========== */

/** 最大支持的正交双舵机关节模块数 */
#define GAIT_MAX_JOINTS  8
/** 旧代码兼容名称 */
#define GAIT_MAX_LEGS GAIT_MAX_JOINTS

/* ========== 步态参数 ========== */

/**
 * 步态参数结构体
 * 所有角度单位: 弧度 (rad)
 * 频率单位: rad/s
 */
typedef struct {
    float alpha_y;   /**< 偏航振幅 α_y (rad) */
    float alpha_p;   /**< 俯仰振幅 α_p (rad) */
    float omega_y;   /**< 偏航角频率 ω_y (rad/s) */
    float omega_p;   /**< 俯仰角频率 ω_p (rad/s) */
    float beta_y;    /**< 相邻关节水平波相位差 β_y (rad) */
    float beta_p;    /**< 相邻关节垂直波相位差 β_p (rad) */
    float phi;       /**< 偏航-俯仰相位差 φ (rad) */
    float gamma_y;   /**< 偏航中心偏移 γ_y (rad) */
    float gamma_p;   /**< 俯仰中心偏移 γ_p (rad) */
} GaitParams;

/* ========== 预设步态 ========== */

/** 旧测试预设 */
extern const GaitParams GAIT_SPIRAL;

/** 旧接口保留 */
extern const GaitParams GAIT_TRIPOD;

/** 旧测试预设 */
extern const GaitParams GAIT_WAVE;
/*s水平扭动步态*/
extern const GaitParams GAITFLAT;
/*test*/
extern const GaitParams  GAIT_TEST;

/** 行波步态 */
extern const GaitParams GAIT_TESTWAVE;

/** 蛇形机器人平面蜿蜒：仅水平轴产生行波 */
extern const GaitParams GAIT_SERPENTINE;

/** 蛇形机器人侧向蜿蜒：水平/垂直行波相差90° */
extern const GaitParams GAIT_SIDEWINDING;
/* ========== ID映射 ========== */

/**
 * 单个蛇身关节模块的舵机映射。
 * 两个物理舵机正交安装，组成水平+垂直两个转动轴。
 */
typedef struct {
    uint8_t  yaw_servo_id;     /**< 水平轴舵机物理ID (1~250) */
    uint8_t  pitch_servo_id;   /**< 垂直轴舵机物理ID (1~250) */
    int8_t   yaw_direction;    /**< 安装方向: +1正常, -1反向 */
    int8_t   pitch_direction;  /**< 安装方向: +1正常, -1反向 */
    float    yaw_offset_deg;   /**< 偏航安装偏移补偿 (度), 用于机械结构校准 */
    float    pitch_offset_deg; /**< 俯仰安装偏移补偿 (度) */
} SnakeJointMap;

/** 旧代码兼容类型名 */
typedef SnakeJointMap LegServoMap;

/* ========== 步态控制器 ========== */

typedef struct {
    GaitParams  params;                /**< 当前步态参数 */
    SnakeJointMap mapping[GAIT_MAX_JOINTS]; /**< 从蛇头到蛇尾的关节映射表 */
    union {
        uint8_t joint_count;           /**< 实际关节模块数 */
        uint8_t leg_count;             /**< 旧代码兼容字段 */
    };
    uint32_t    start_tick;            /**< 步态起始时间戳 (ms) */
    uint32_t    last_update;           /**< 上次更新时间戳 (ms) */
    uint32_t    ramp_duration_ms;       /**< 启动振幅渐增时间，0表示关闭 */
} GaitController;

/* ========== API 函数 ========== */

/**
 * @brief 初始化步态控制器
 * @param gc        控制器指针
 * @param joint_count 正交双舵机关节模块数 (≤ GAIT_MAX_JOINTS)
 */
void Gait_Init(GaitController *gc, uint8_t joint_count);

void Gait_SetJointMapping(GaitController *gc, uint8_t joint_index,
                          uint8_t horizontal_servo_id,
                          uint8_t vertical_servo_id,
                          float horizontal_offset_deg,
                          float vertical_offset_deg);

void Gait_SetJointMappingEx(GaitController *gc, uint8_t joint_index,
                            uint8_t horizontal_servo_id,
                            uint8_t vertical_servo_id,
                            int8_t horizontal_direction,
                            int8_t vertical_direction,
                            float horizontal_offset_deg,
                            float vertical_offset_deg);

void Gait_SetJointMappingSequential(GaitController *gc, uint8_t base_id);

/**
 * @brief 旧名称兼容接口；新代码使用 Gait_SetJointMapping
 * @param gc            控制器指针
 * @param leg_index     逻辑腿编号 (0-based)
 * @param yaw_servo_id  偏航舵机物理ID
 * @param pitch_servo_id 俯仰舵机物理ID
 * @param yaw_offset_deg   偏航安装偏移 (度)
 * @param pitch_offset_deg 俯仰安装偏移 (度)
 */
void Gait_SetMapping(GaitController *gc, uint8_t leg_index,
                     uint8_t yaw_servo_id, uint8_t pitch_servo_id,
                     float yaw_offset_deg, float pitch_offset_deg);

/**
 * @brief 旧名称兼容接口；新代码使用 Gait_SetJointMappingEx
 * @param yaw_direction/pitch_direction 只能为 +1 或 -1
 */
void Gait_SetMappingEx(GaitController *gc, uint8_t leg_index,
                       uint8_t yaw_servo_id, uint8_t pitch_servo_id,
                       int8_t yaw_direction, int8_t pitch_direction,
                       float yaw_offset_deg, float pitch_offset_deg);

/**
 * @brief 旧名称兼容接口；新代码使用 Gait_SetJointMappingSequential
 * @param gc            控制器指针
 * @param base_id       起始舵机ID (第0腿偏航=base_id, 俯仰=base_id+1, 以此类推)
 *
 * 例如 base_id=1, leg_count=4:
 *   Leg0: Yaw=1,  Pitch=2
 *   Leg1: Yaw=3,  Pitch=4
 *   Leg2: Yaw=5,  Pitch=6
 *   Leg3: Yaw=7,  Pitch=8
 */
void Gait_SetMappingSequential(GaitController *gc, uint8_t base_id);

/**
 * @brief 检查映射中的ID范围、方向和重复ID
 * @return 0=有效, 1=参数无效, 2=ID/方向无效, 3=存在重复物理ID
 */
uint8_t Gait_ValidateMapping(const GaitController *gc);

/**
 * @brief 设置步态参数
 */
void Gait_SetParams(GaitController *gc, const GaitParams *params);

/**
 * @brief 配置平面蜿蜒步态
 * @param amplitude_deg 水平关节振幅，建议初次测试15~30度
 * @param period_s 一个完整摆动周期，单位秒
 * @param phase_deg 相邻关节的相位差，6关节形成一条完整波时为60度
 */
void Gait_ConfigurePlanarSerpentine(GaitController *gc,
                                    float amplitude_deg,
                                    float period_s,
                                    float phase_deg);

/**
 * @brief 配置螺旋翻滚步态
 * @param horizontal_amplitude_deg 水平轴振幅，初次测试建议20~30度
 * @param vertical_amplitude_deg 垂直轴振幅，通常与水平轴相同
 * @param period_s 一个完整翻滚周期，单位秒
 * @param joint_phase_deg 相邻关节空间相位差；4关节一条完整波时为90度
 * @param axis_phase_deg 水平与垂直自由度相位差；螺旋翻滚通常为90度
 */
void Gait_ConfigureSpiralRolling(GaitController *gc,
                                 float horizontal_amplitude_deg,
                                 float vertical_amplitude_deg,
                                 float period_s,
                                 float joint_phase_deg,
                                 float axis_phase_deg);

/** 设置启动时从0逐渐增加到目标振幅的时间 */
void Gait_SetRampDuration(GaitController *gc, uint32_t ramp_duration_ms);

/**
 * @brief 获取当前时间t (从步态启动开始的秒数)
 */
float Gait_GetTime(GaitController *gc, uint32_t current_tick_ms);

/**
 * @brief 计算指定腿的偏航角和俯仰角
 * @param gc        控制器指针
 * @param leg_index 逻辑腿编号 (0-based)
 * @param t         时间 (秒)
 * @param theta_y   输出: 偏航角 (度)
 * @param theta_p   输出: 俯仰角 (度)
 */
void Gait_CalcLeg(GaitController *gc, uint8_t leg_index, float t,
                  float *theta_y, float *theta_p);

/**
 * @brief 计算从蛇头起第 joint_index 个关节的水平角和垂直角
 */
void Gait_CalcJoint(GaitController *gc, uint8_t joint_index, float t,
                    float *theta_horizontal, float *theta_vertical);

/**
 * @brief 更新所有蛇身关节的舵机角度
 * @param gc              控制器指针
 * @param current_tick_ms 当前时间戳 (ms), 传入 HAL_GetTick()
 *
 * 内部流程:
 *   1. 计算当前时间 t
 *   2. 对每个关节计算水平角和垂直角 (加上安装偏移补偿)
 *   3. 通过串口发送角度指令到对应的物理舵机
 *
 * 注意: 此函数会阻塞发送, 每个关节耗时约 10ms
 *       建议控制周期 ≥ 20ms (50Hz)
 */
void Gait_Update(GaitController *gc, uint32_t current_tick_ms);

/**
 * @brief 同步写模式更新 - 所有舵机角度一次通讯发送 (更高效)
 * @param gc              控制器指针
 * @param current_tick_ms 当前时间戳 (ms), 传入 HAL_GetTick()
 *
 * 与 Gait_Update 的区别:
 *   - Gait_Update:     逐个发送, 每个舵机一条指令 (N个关节 × 2次串口通讯)
 *   - Gait_UpdateSync: 收集所有角度, 通过同步写一次发送 (只需1~2次串口通讯)
 *
 * 优势: 所有舵机几乎同时接收到指令, 运动更同步
 *       串口通讯次数大幅减少, 释放CPU时间
 *
 * 注意: 需要舵机固件支持 SyncWrite 指令 (CMDType_SyncWrite = 0x83)
 *       每批最多 SYNC_WRITE_MAX_SERVOS 个舵机, 超长自动分批
 */
void Gait_UpdateSync(GaitController *gc, uint32_t current_tick_ms);

/**
 * @brief 重启步态 (重置计时器)
 */
void Gait_Restart(GaitController *gc);

/**
 * @brief 停止所有舵机 (关闭扭矩)
 */
void Gait_StopAll(GaitController *gc, Usart_DataTypeDef *usart);

/**
 * @brief 弧度 → 舵机原始值 (0~4095, 中心2048=0°)
 */
uint16_t Gait_RadianToRaw(float rad);

/**
 * @brief 角度(度) → 舵机原始值 (0~4095)
 */
uint16_t Gait_DegToRaw(float deg);

#endif /* __GAIT_H */
