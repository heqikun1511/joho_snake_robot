#include "spi_slave_link.h"

#include "main.h"
#include "spi.h"

#include <string.h>

static uint8_t spi_tx[SPI_SLAVE_TEST_FRAME_SIZE] = {
    0xAAU, 0x55U, 0x12U, 0x34U
};
static uint8_t spi_rx[SPI_SLAVE_TEST_FRAME_SIZE];
static uint8_t last_rx[SPI_SLAVE_TEST_FRAME_SIZE];

static volatile uint32_t transfer_count;
static volatile uint32_t error_count;
static volatile uint8_t restart_required;

static HAL_StatusTypeDef SPI_SlaveLink_ArmTransfer(void)
{
    return HAL_SPI_TransmitReceive_IT(&hspi3,
                                      spi_tx,
                                      spi_rx,
                                      SPI_SLAVE_TEST_FRAME_SIZE);
}

void SPI_SlaveLink_Start(void)
{
    transfer_count = 0U;
    error_count = 0U;
    restart_required = 0U;
    memset(spi_rx, 0, sizeof(spi_rx));
    memset(last_rx, 0, sizeof(last_rx));

    if (SPI_SlaveLink_ArmTransfer() != HAL_OK)
    {
        restart_required = 1U;
    }
}

void SPI_SlaveLink_Process(void)
{
    if (restart_required != 0U)
    {
        restart_required = 0U;
        (void)HAL_SPI_Abort(&hspi3);

        if (SPI_SlaveLink_ArmTransfer() != HAL_OK)
        {
            restart_required = 1U;
        }
    }
}

uint32_t SPI_SlaveLink_GetTransferCount(void)
{
    return transfer_count;
}

uint32_t SPI_SlaveLink_GetErrorCount(void)
{
    return error_count;
}

const uint8_t *SPI_SlaveLink_GetLastRx(void)
{
    return last_rx;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance != SPI3)
    {
        return;
    }

    memcpy(last_rx, spi_rx, sizeof(last_rx));
    transfer_count++;
    HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_9);

    if (SPI_SlaveLink_ArmTransfer() != HAL_OK)
    {
        restart_required = 1U;
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI3)
    {
        error_count++;
        restart_required = 1U;
    }
}
