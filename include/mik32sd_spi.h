#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include "spi_transaction.h"
#include "mik32_hal_gpio.h"

typedef struct
{
    SPI_TypeDef *host;
    GPIO_TypeDef *cs_gpio;
    HAL_PinsTypeDef cs_pin;
    uint32_t frequency;
} mik32_sd_spi_cfg_t;

typedef struct
{
    spi_transaction_t spi_trans;
    spi_transaction_t spi_res;
    GPIO_TypeDef *cs_gpio;
    HAL_PinsTypeDef cs_pin;
    uint32_t frequency;
} mik32_sd_spi_t;


bool mik32_sd_spi_init(mik32_sd_spi_t *desc, mik32_sd_spi_cfg_t *cfg);
dma_status_t mik32_sd_spi_cs_down(mik32_sd_spi_t *desc);
dma_status_t mik32_sd_spi_cs_up(mik32_sd_spi_t *desc);
dma_status_t mik32_sd_spi_increase_clock_speed(mik32_sd_spi_t *desc);
dma_status_t mik32_sd_spi_reduce_clock_speed(mik32_sd_spi_t *desc);
uint8_t mik32_sd_spi_ex(mik32_sd_spi_t *desc, uint8_t data);
dma_status_t mik32_sd_spi_sector_read(mik32_sd_spi_t *desc, void *dst, uint32_t len_bytes);
dma_status_t mik32_sd_spi_sector_write(mik32_sd_spi_t *desc, const void *src, uint32_t len_bytes);

#if defined(__cplusplus)
}
#endif
