# STM32F407 firmware

This directory is the standalone build and flash directory for the STM32F407.

## Build

Requirements: `make` and the Arm GNU toolchain (`arm-none-eabi-gcc`).

```bash
cd firmware/stm32f407_snake
make clean
make -j
```

Generated images are placed in `build/`:

- `F407_JOHO.elf`
- `F407_JOHO.hex`
- `F407_JOHO.bin`

## Flash with ST-Link

Connect SWDIO, SWCLK, GND and the target reference voltage, then run:

```bash
make flash
```

The command writes `build/F407_JOHO.bin` at address `0x08000000` and resets the
MCU. `STFLASH=/path/to/st-flash make flash` can be used to select another
`st-flash` executable.

## Current SPI3 slave test

STM32 pins:

| Signal | STM32F407 pin |
| --- | --- |
| NSS | PA15 |
| SCK | PC10 |
| MISO | PC11 |
| MOSI | PC12 |

The interface uses SPI mode 0, 8-bit words and MSB first. For every exact
4-byte transaction initiated by the Raspberry Pi, STM32 transmits:

```text
AA 55 12 34
```

The Raspberry Pi must act as the SPI master, assert CE/NSS and provide 32 clock
pulses. For example, Python `spidev` can use:

```python
rx = spi.xfer2([0x00, 0x00, 0x00, 0x00])
print(" ".join(f"{value:02X}" for value in rx))
```

The board LED on PF9 toggles after each completed 4-byte transfer. The latest
bytes received from Raspberry Pi and transfer/error counters are available
through `SPI_SlaveLink_GetLastRx()`, `SPI_SlaveLink_GetTransferCount()` and
`SPI_SlaveLink_GetErrorCount()` for debugger inspection.

Keep every Raspberry Pi transaction at exactly four bytes during this test.
