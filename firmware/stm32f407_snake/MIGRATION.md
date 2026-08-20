# STM32F407 firmware migration

This directory is a buildable copy migrated from `/home/amoy/F407_active/F407_JOHO`.
The source repository remains unchanged.

## Directory mapping

| Source | Migrated location | Responsibility |
| --- | --- | --- |
| `Core/` | `Core/` | CubeMX startup, interrupts, GPIO, UART and main entry |
| `APP/gait.*` | `Application/gait.*` | Snake gait generation |
| `APP/step.*` | `Application/step.*` | Gait step data |
| `HARDWARE/uart_servo/` | `Drivers/Board/joho_servo/` | JOHO bus-servo protocol |
| `HARDWARE/ring_buffer/` | `Drivers/Board/ring_buffer/` | UART receive buffering |
| `Drivers/CMSIS/` | `Drivers/CMSIS/` | MCU/CMSIS vendor support |
| `Drivers/STM32F4xx_HAL_Driver/` | unchanged | STM32 HAL vendor support |

## Build

```bash
cd firmware/stm32f407_snake
make -j2
```

The resulting images are generated under `build/` with the target name
`STM32F407_SNAKE`.

## Current boundary

This migration preserves the existing standalone gait and servo behavior. It
does not yet implement the ROS 2 host protocol. The next firmware layer should
add `Application/host_protocol.*`, `Application/servo_manager.*` and
`Application/safety_monitor.*`, then replace blocking demo control in `main.c`
with a non-blocking scheduler.

## Migration-only correction

The copied `Core/Src/main.c` contained the invalid statement
`HAL_GPIO_Init(GPIOF, &led);55`. The trailing `55` was removed only in this
migrated copy so that it builds; the source repository was not edited.
