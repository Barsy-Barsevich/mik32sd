#include "mik32sd_spi.h"


bool mik32_sd_spi_init(mik32_sd_spi_t *desc, mik32_sd_spi_cfg_t *cfg)
{
    desc->cs_gpio = cfg->cs_gpio;
    desc->cs_pin = cfg->cs_pin;
    desc->frequency = cfg->frequency;

    spi_transaction_cfg_t trans_cfg = {
        .host = cfg->host,
        .dma_channel = DMA_CH_AUTO,
        .dma_priority = 0,
        .direction = SPI_TRANSACTION_TRANSMIT,
        .token = 0,
        .pre_cb = NULL,
        .post_cb = NULL
    };
    dma_status_t dma_res;
    dma_res = spi_transaction_init(&desc->spi_trans, &trans_cfg);
    if (dma_res != DMA_STATUS_OK) return false;
    spi_transaction_cfg_t res_cfg = {
        .host = cfg->host,
        .dma_channel = DMA_CH_AUTO,
        .dma_priority = 0,
        .direction = SPI_TRANSACTION_RECEIVE,
        .token = 1,
        .pre_cb = NULL,
        .post_cb = NULL
    };
    dma_res = spi_transaction_init(&desc->spi_res, &res_cfg);
    if (dma_res != DMA_STATUS_OK) return false;

    switch ((uint32_t)cfg->cs_gpio)
    {
        case (uint32_t)GPIO_0: __HAL_PCC_GPIO_0_CLK_ENABLE(); break;
        case (uint32_t)GPIO_1: __HAL_PCC_GPIO_1_CLK_ENABLE(); break;
        case (uint32_t)GPIO_2: __HAL_PCC_GPIO_2_CLK_ENABLE(); break;
    }
    GPIO_InitTypeDef cs_cfg = {
        .Pin = cfg->cs_pin,
        .DS = HAL_GPIO_DS_2MA,
        .Mode = HAL_GPIO_MODE_GPIO_OUTPUT,
        .Pull = HAL_GPIO_PULL_NONE
    };
    HAL_StatusTypeDef hal_res;
    hal_res = HAL_GPIO_Init(cfg->cs_gpio, &cs_cfg);
    if (hal_res != HAL_OK) return false;
    mik32_sd_spi_cs_up(desc);
    return true;
}

dma_status_t mik32_sd_spi_cs_down(mik32_sd_spi_t *desc)
{
    desc->spi_trans.host->ENABLE = SPI_ENABLE_M;
    if (desc == NULL)
    {
        return DMA_STATUS_INCORRECT_ARGUMENT;
    }
    HAL_GPIO_WritePin(desc->cs_gpio, desc->cs_pin, 0);
    return DMA_STATUS_OK;
}

dma_status_t mik32_sd_spi_cs_up(mik32_sd_spi_t *desc)
{
    if (desc == NULL)
    {
        return DMA_STATUS_INCORRECT_ARGUMENT;
    }
    HAL_GPIO_WritePin(desc->cs_gpio, desc->cs_pin, 1);
    return DMA_STATUS_OK;
}

dma_status_t mik32_sd_spi_increase_clock_speed(mik32_sd_spi_t *desc)
{
    desc->spi_trans.host->CONFIG &= ~SPI_CONFIG_BAUD_RATE_DIV_M;
    switch (desc->frequency)
    {
    case 16000000:
        desc->spi_trans.host->CONFIG |= SPI_CONFIG_BAUD_RATE_DIV_2_M;
        break;
    case 8000000:
        desc->spi_trans.host->CONFIG |= SPI_CONFIG_BAUD_RATE_DIV_4_M;
        break;
    case 4000000:
        desc->spi_trans.host->CONFIG |= SPI_CONFIG_BAUD_RATE_DIV_8_M;
        break;
    case 2000000:
        desc->spi_trans.host->CONFIG |= SPI_CONFIG_BAUD_RATE_DIV_16_M;
        break;
    case 1000000:
        desc->spi_trans.host->CONFIG |= SPI_CONFIG_BAUD_RATE_DIV_32_M;
        break;
    case 500000:
        desc->spi_trans.host->CONFIG |= SPI_CONFIG_BAUD_RATE_DIV_64_M;
        break;
    case 250000:
        desc->spi_trans.host->CONFIG |= SPI_CONFIG_BAUD_RATE_DIV_128_M;
        break;
    case 125000:
        desc->spi_trans.host->CONFIG |= SPI_CONFIG_BAUD_RATE_DIV_256_M;
        break;
    default:
        return DMA_STATUS_INCORRECT_ARGUMENT;
    }
    return DMA_STATUS_OK;
}

dma_status_t mik32_sd_spi_reduce_clock_speed(mik32_sd_spi_t *desc)
{
    // use minimal SPI frequency (<400kHz)
    desc->spi_trans.host->CONFIG |= SPI_CONFIG_BAUD_RATE_DIV_M;
    return DMA_STATUS_OK;
}

uint8_t mik32_sd_spi_ex(mik32_sd_spi_t *desc, uint8_t data)
{
    // desc->spi_trans.host->ENABLE = SPI_ENABLE_M;
    // dma_status_t res = spi_transmit(&desc->spi_trans, (char*)&data, 1, 1000);
    // if (res != DMA_STATUS_OK)
    // {
    //     printf("%u", res);
    // }
    // return desc->spi_trans.host->RXDATA;

    desc->spi_trans.host->ENABLE = SPI_ENABLE_M;
    desc->spi_trans.host->TXDATA = data;
    while ((desc->spi_trans.host->INT_STATUS & (1<<4)) == 0);
    return desc->spi_trans.host->RXDATA;
}

dma_status_t mik32_sd_spi_sector_read(mik32_sd_spi_t *desc, void *dst, uint32_t len_bytes)
{
    if (desc == NULL || dst == NULL)
    {
        return DMA_STATUS_INCORRECT_ARGUMENT;
    }
    memset(dst, 0xFF, len_bytes);
    dma_status_t res;
    res = spi_receive_start(&desc->spi_res, (char*)dst, len_bytes);
    res = spi_transmit_start(&desc->spi_trans, (char*)dst, len_bytes);
    res = spi_transaction_end(&desc->spi_res, DMA_NO_TIMEOUT);
    res = spi_transaction_end(&desc->spi_trans, DMA_NO_TIMEOUT);
    return res;
}

dma_status_t mik32_sd_spi_sector_write(mik32_sd_spi_t *desc, const void *src, uint32_t len_bytes)
{
    if (desc == NULL || src == NULL)
    {
        return DMA_STATUS_INCORRECT_ARGUMENT;
    }
    dma_status_t res;
    res = spi_transmit(&desc->spi_trans, (char*)src, len_bytes, DMA_NO_TIMEOUT);
    return res;
}