#ifndef SPI_SLAVE_LINK_H
#define SPI_SLAVE_LINK_H

#include <stdint.h>

/* Fixed-size link used to verify Raspberry Pi <-> STM32 SPI wiring. */
#define SPI_SLAVE_TEST_FRAME_SIZE 4U

void SPI_SlaveLink_Start(void);
void SPI_SlaveLink_Process(void);
uint32_t SPI_SlaveLink_GetTransferCount(void);
uint32_t SPI_SlaveLink_GetErrorCount(void);
const uint8_t *SPI_SlaveLink_GetLastRx(void);

#endif /* SPI_SLAVE_LINK_H */
