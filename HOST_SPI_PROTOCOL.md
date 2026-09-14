# 树莓派与 STM32 SPI 接口协议

## 物理配置

- 树莓派：SPI0 CE0，Linux 设备 `/dev/spidev0.0`。
- STM32：SPI3 从机，PA15=NSS、PC10=SCK、PC11=MISO、PC12=MOSI。
- SPI Mode 0，8 bit，MSB first。
- 初始时钟 1 MHz，稳定后再测试更高频率。
- 每次交换固定 32 字节。树莓派是主机，负责发起每次传输。

## 帧格式

| 偏移 | 长度 | 内容 |
| --- | ---: | --- |
| 0 | 1 | Magic `0x53` |
| 1 | 1 | Magic `0x52` |
| 2 | 1 | 协议版本，当前为 1 |
| 3 | 1 | 消息类型 |
| 4 | 2 | 序号，uint16，小端 |
| 6 | 1 | 有效关节数量，最大 8 |
| 7 | 1 | flags，当前保留 |
| 8 | 22 | payload |
| 30 | 2 | CRC16-CCITT，小端 |

关节位置使用有符号 `int16` 毫弧度。例如 `0.25 rad` 编码为 `250`。
8 个关节占用 payload 的前 16 字节，顺序与 ros2_control 配置一致。

## 消息类型

- `0x01 PING`
- `0x02 ENABLE`
- `0x03 DISABLE`
- `0x04 SET_JOINTS`
- `0x05 GET_JOINTS`
- `0x06 GET_STATUS`
- `0x07 ESTOP`
- `0x80 ACK`
- `0x81 JOINTS`
- `0x82 STATUS`
- `0xFF ERROR`

## 实现顺序

1. PC 单元测试验证编码、解码和 CRC。
2. STM32 使用 SPI3 中断或 DMA 持续接收固定长度帧。
3. 先实现 `PING`，不连接舵机。
4. 再实现 `SET_JOINTS`，只缓存和打印目标值。
5. 验证限位、超时和急停后，才调用舵机 `SyncWrite`。
6. 最后在 ROS 侧实现 `snake_mcu_hardware`。

SPI 从机无法在收到完整请求前生成同一帧的响应，因此响应采用流水线方式：
第 N 次传输接收请求，第 N+1 次传输返回对应结果；序号用于匹配请求和响应。
