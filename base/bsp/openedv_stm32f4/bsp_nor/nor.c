/**
 * @file    nor.c
 * @brief   SPI NOR flash storage: device-independent command algorithms
 *          (read / page program / sector erase) over the chip driver
 *          (nor_w25q256jv, which owns the SPI bus and chip select).
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "stm32f4xx_hal.h"
#include "nor.h"
#include "bsp_log.h"

#define NOR_W25Q256JV_SIZE_BYTE  (32U * 1024U * 1024U)

typedef enum
{
    SPI_BUS_NORFLASH = 0,
    SPI_BUS_NRF24L01 = 1,
} spi_bus_t;

void nor_w25q256jv_dev_init(void);
uint16_t nor_w25q256jv_probe(void);
uint8_t nor_w25q256jv_addr_bytes(void);
void nor_w25q256jv_cs_low(void);
void nor_w25q256jv_cs_high(void);
uint8_t nor_w25q256jv_spi_rw(uint8_t data);

void spi_init(spi_bus_t bus);
void spi_set_speed(spi_bus_t bus, uint8_t prescaler);
uint8_t spi_read_write_byte(spi_bus_t bus, uint8_t txdata);

/* Command set. */
#define FLASH_WriteEnable       0x06U
#define FLASH_ReadStatusReg1    0x05U
#define FLASH_ReadStatusReg2    0x35U
#define FLASH_ReadStatusReg3    0x15U
#define FLASH_WriteStatusReg3   0x11U
#define FLASH_ReadData          0x03U
#define FLASH_PageProgram       0x02U
#define FLASH_SectorErase       0x20U
#define FLASH_BlockErase64k     0xD8U
#define FLASH_Enable4ByteAddr   0xB7U

#define FLASH_SR3_ADP_BIT       0x02U
#define FLASH_SR1_BUSY_BIT      0x01U

#define NOR_4BYTE_MODE_ADDR_BYTES   4U
#define NOR_4BYTE_MODE_SETTLE_MS    20U
#define NOR_LOCK_TIMEOUT_MS         1000U
#define NOR_BUSY_TIMEOUT_MS         2000U

static uint8_t g_nor_buf[NOR_SECTOR_SIZE_BYTE];
static SemaphoreHandle_t g_nor_lock;
static volatile uint32_t g_nor_timeouts;

#define NOR_CS_LOW()   nor_w25q256jv_cs_low()
#define NOR_CS_HIGH()  nor_w25q256jv_cs_high()

static uint8_t nor_spi_rw(uint8_t data)
{
    return nor_w25q256jv_spi_rw(data);
}

static int nor_lock(uint32_t timeout_ms)
{
    TickType_t ticks;

    if (g_nor_lock == 0)
    {
        return -1;
    }
    ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xSemaphoreTake(g_nor_lock, ticks) == pdTRUE) ? 0 : -1;
}

static void nor_unlock(void)
{
    if (g_nor_lock != 0)
    {
        (void)xSemaphoreGive(g_nor_lock);
    }
}

/** @brief  Read one of the three status registers (1..3). */
static uint8_t nor_read_sr(uint8_t regno)
{
    uint8_t command;
    uint8_t byte;

    switch (regno)
    {
        case 2:
            command = FLASH_ReadStatusReg2;
            break;
        case 3:
            command = FLASH_ReadStatusReg3;
            break;
        case 1:
        default:
            command = FLASH_ReadStatusReg1;
            break;
    }

    NOR_CS_LOW();
    (void)nor_spi_rw(command);
    byte = nor_spi_rw(0xFFU);
    NOR_CS_HIGH();

    return byte;
}

static int nor_wait_busy(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while ((nor_read_sr(1) & FLASH_SR1_BUSY_BIT) == FLASH_SR1_BUSY_BIT)
    {
        if ((HAL_GetTick() - start) > timeout_ms)
        {
            g_nor_timeouts++;
            return -1;
        }
    }
    return 0;
}

static void nor_write_enable(void)
{
    NOR_CS_LOW();
    (void)nor_spi_rw(FLASH_WriteEnable);
    NOR_CS_HIGH();
}

static void nor_send_address(uint32_t address)
{
    if (nor_w25q256jv_addr_bytes() == NOR_4BYTE_MODE_ADDR_BYTES)
    {
        (void)nor_spi_rw((uint8_t)(address >> 24));
    }

    (void)nor_spi_rw((uint8_t)(address >> 16));
    (void)nor_spi_rw((uint8_t)(address >> 8));
    (void)nor_spi_rw((uint8_t)address);
}

static void nor_write_status_reg3(uint8_t sr)
{
    NOR_CS_LOW();
    (void)nor_spi_rw(FLASH_WriteStatusReg3);
    (void)nor_spi_rw(sr);
    NOR_CS_HIGH();
}

static int nor_read_locked(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t i;

    if ((pbuf == 0) || (addr >= NOR_DEVICE_SIZE_BYTE))
    {
        return -1;
    }

    if ((addr + datalen) > NOR_DEVICE_SIZE_BYTE)
    {
        datalen = (uint16_t)(NOR_DEVICE_SIZE_BYTE - addr);
    }

    NOR_CS_LOW();
    (void)nor_spi_rw(FLASH_ReadData);
    nor_send_address(addr);

    for (i = 0U; i < datalen; i++)
    {
        pbuf[i] = nor_spi_rw(0xFFU);
    }

    NOR_CS_HIGH();
    return 0;
}

static int nor_write_page_locked(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t i;

    if (nor_wait_busy(NOR_BUSY_TIMEOUT_MS) != 0)
    {
        return -1;
    }
    nor_write_enable();

    NOR_CS_LOW();
    (void)nor_spi_rw(FLASH_PageProgram);
    nor_send_address(addr);

    for (i = 0U; i < datalen; i++)
    {
        (void)nor_spi_rw(pbuf[i]);
    }

    NOR_CS_HIGH();
    return nor_wait_busy(NOR_BUSY_TIMEOUT_MS);
}

static int nor_write_nocheck_locked(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t pageremain = (uint16_t)(NOR_PAGE_SIZE_BYTE - (addr % NOR_PAGE_SIZE_BYTE));

    if (datalen <= pageremain)
    {
        pageremain = datalen;
    }

    for (;;)
    {
        if (nor_write_page_locked(pbuf, addr, pageremain) != 0)
        {
            return -1;
        }

        if (datalen == pageremain)
        {
            break;
        }

        pbuf      += pageremain;
        addr      += pageremain;
        datalen   -= pageremain;
        pageremain = (datalen > NOR_PAGE_SIZE_BYTE) ? NOR_PAGE_SIZE_BYTE : datalen;
    }
    return 0;
}

static int nor_erase_sector_locked(uint32_t saddr)
{
    saddr *= NOR_SECTOR_SIZE_BYTE;

    if (saddr >= NOR_DEVICE_SIZE_BYTE)
    {
        return -1;
    }

    if (nor_wait_busy(NOR_BUSY_TIMEOUT_MS) != 0)
    {
        return -1;
    }
    nor_write_enable();

    NOR_CS_LOW();
    (void)nor_spi_rw(FLASH_SectorErase);
    nor_send_address(saddr);
    NOR_CS_HIGH();
    return nor_wait_busy(NOR_BUSY_TIMEOUT_MS);
}

static int nor_write_locked(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint32_t secpos;
    uint16_t secoff;
    uint16_t secremain;
    uint16_t i;

    if ((pbuf == 0) || (addr >= NOR_DEVICE_SIZE_BYTE))
    {
        return -1;
    }

    if ((addr + datalen) > NOR_DEVICE_SIZE_BYTE)
    {
        datalen = (uint16_t)(NOR_DEVICE_SIZE_BYTE - addr);
    }

    secpos    = addr / NOR_SECTOR_SIZE_BYTE;
    secoff    = (uint16_t)(addr % NOR_SECTOR_SIZE_BYTE);
    secremain = (uint16_t)(NOR_SECTOR_SIZE_BYTE - secoff);

    if (datalen <= secremain)
    {
        secremain = datalen;
    }

    for (;;)
    {
        if (nor_read_locked(g_nor_buf, secpos * NOR_SECTOR_SIZE_BYTE,
                            NOR_SECTOR_SIZE_BYTE) != 0)
        {
            return -1;
        }

        for (i = 0U; i < secremain; i++)
        {
            if (g_nor_buf[secoff + i] != 0xFFU)
            {
                break;
            }
        }

        if (i < secremain) /* sector is not blank: erase and merge */
        {
            if (nor_erase_sector_locked(secpos) != 0)
            {
                return -1;
            }

            for (i = 0U; i < secremain; i++)
            {
                g_nor_buf[i + secoff] = pbuf[i];
            }

            if (nor_write_nocheck_locked(g_nor_buf, secpos * NOR_SECTOR_SIZE_BYTE,
                                         NOR_SECTOR_SIZE_BYTE) != 0)
            {
                return -1;
            }
        }
        else
        {
            if (nor_write_nocheck_locked(pbuf, addr, secremain) != 0)
            {
                return -1;
            }
        }

        if (datalen == secremain)
        {
            break;
        }

        secpos++;
        secoff = 0U;
        pbuf   += secremain;
        addr   += secremain;
        datalen -= secremain;
        secremain = (datalen > NOR_SECTOR_SIZE_BYTE) ? NOR_SECTOR_SIZE_BYTE : datalen;
    }
    return 0;
}

void nor_init(void)
{
    g_nor_lock = xSemaphoreCreateMutex();
    if (g_nor_lock == 0)
    {
        BSP_LOGE("nor", "bus mutex create failed");
        return;
    }

    nor_w25q256jv_dev_init();
    if (nor_lock(UINT32_MAX) != 0)
    {
        return;
    }

    {
        uint16_t jedec = nor_w25q256jv_probe();

        BSP_LOGI("nor", "JEDEC=0x%04x size=%lu MiB",
                 (unsigned)jedec,
                 (unsigned long)(NOR_DEVICE_SIZE_BYTE / (1024U * 1024U)));

        if (nor_w25q256jv_addr_bytes() == NOR_4BYTE_MODE_ADDR_BYTES)
        {
            uint8_t sr3 = nor_read_sr(3);

            if ((sr3 & 0x01U) == 0U) /* not yet in 4-byte address mode */
            {
                nor_write_enable();
                sr3 |= FLASH_SR3_ADP_BIT;
                nor_write_status_reg3(sr3);
                HAL_Delay(NOR_4BYTE_MODE_SETTLE_MS);

                NOR_CS_LOW();
                (void)nor_spi_rw(FLASH_Enable4ByteAddr);
                NOR_CS_HIGH();
            }
        }
    }

    nor_unlock();
}

uint16_t nor_read_id(void)
{
    uint16_t id;

    if (nor_lock(NOR_LOCK_TIMEOUT_MS) != 0)
    {
        return 0xFFFFU;
    }
    id = nor_w25q256jv_probe();
    nor_unlock();
    return id;
}

int nor_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    int ret;

    if (nor_lock(NOR_LOCK_TIMEOUT_MS) != 0)
    {
        return -1;
    }
    ret = nor_read_locked(pbuf, addr, datalen);
    nor_unlock();
    return ret;
}

int nor_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    int ret;

    if (nor_lock(NOR_LOCK_TIMEOUT_MS) != 0)
    {
        return -1;
    }
    ret = nor_write_locked(pbuf, addr, datalen);
    nor_unlock();
    return ret;
}

int nor_write_erased(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    int ret;

    if (nor_lock(NOR_LOCK_TIMEOUT_MS) != 0)
    {
        return -1;
    }
    if (addr >= NOR_DEVICE_SIZE_BYTE)
    {
        nor_unlock();
        return -1;
    }
    if ((addr + datalen) > NOR_DEVICE_SIZE_BYTE)
    {
        datalen = (uint16_t)(NOR_DEVICE_SIZE_BYTE - addr);
    }
    ret = nor_write_nocheck_locked(pbuf, addr, datalen);
    nor_unlock();
    return ret;
}

int nor_erase_sector(uint32_t saddr)
{
    int ret;

    if (nor_lock(NOR_LOCK_TIMEOUT_MS) != 0)
    {
        return -1;
    }
    ret = nor_erase_sector_locked(saddr);
    nor_unlock();
    return ret;
}

int nor_erase_block64(uint32_t addr)
{
    int ret = -1;

    if (nor_lock(NOR_LOCK_TIMEOUT_MS) != 0)
    {
        return -1;
    }
    if (addr < NOR_DEVICE_SIZE_BYTE)
    {
        if (nor_wait_busy(NOR_BUSY_TIMEOUT_MS) == 0)
        {
            nor_write_enable();
            NOR_CS_LOW();
            (void)nor_spi_rw(FLASH_BlockErase64k);
            nor_send_address(addr);
            NOR_CS_HIGH();
            ret = nor_wait_busy(NOR_BUSY_TIMEOUT_MS);
        }
    }
    nor_unlock();
    return ret;
}


/* ---- SPI bus ---- */
/**
 * @file    spi.c
 * @brief   SPI master driver: SPI5 for the NOR flash, SPI2 for NRF24L01.
 *          MSP content (clock + GPIO) is inlined into spi_init().
 */


#define SPI5_GPIO_PORT      GPIOF
#define SPI5_SCK_PIN        GPIO_PIN_7
#define SPI5_MISO_PIN       GPIO_PIN_8
#define SPI5_MOSI_PIN       GPIO_PIN_9
#define SPI5_GPIO_AF        GPIO_AF5_SPI5

#define SPI2_GPIO_PORT      GPIOB
#define SPI2_SCK_PIN        GPIO_PIN_13
#define SPI2_MISO_PIN       GPIO_PIN_14
#define SPI2_MOSI_PIN       GPIO_PIN_15
#define SPI2_GPIO_AF        GPIO_AF5_SPI2

#define SPI_DUMMY_BYTE      0xFFU
#define SPI_TIMEOUT_MS      1000U

static SPI_HandleTypeDef g_spi5_handle;
static SPI_HandleTypeDef g_spi2_handle;

static SPI_HandleTypeDef *spi_handle(spi_bus_t bus)
{
    return (bus == SPI_BUS_NRF24L01) ? &g_spi2_handle : &g_spi5_handle;
}

void spi_init(spi_bus_t bus)
{
    GPIO_InitTypeDef   gpio_init = {0};
    SPI_HandleTypeDef *hspi      = spi_handle(bus);

    if (bus == SPI_BUS_NRF24L01)
    {
        /* ---- MSP begin: SPI2 clock + PB13/14/15 ---- */
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_SPI2_CLK_ENABLE();

        gpio_init.Pin       = SPI2_SCK_PIN | SPI2_MISO_PIN | SPI2_MOSI_PIN;
        gpio_init.Mode      = GPIO_MODE_AF_PP;
        gpio_init.Pull      = GPIO_PULLUP;
        gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = SPI2_GPIO_AF;
        HAL_GPIO_Init(SPI2_GPIO_PORT, &gpio_init);
        /* ---- MSP end ---- */

        hspi->Instance       = SPI2;
        hspi->Init.CLKPolarity = SPI_POLARITY_LOW;  /* mode 0 for the radio */
        hspi->Init.CLKPhase    = SPI_PHASE_1EDGE;
    }
    else
    {
        /* ---- MSP begin: SPI5 clock + PF7/8/9 ---- */
        __HAL_RCC_GPIOF_CLK_ENABLE();
        __HAL_RCC_SPI5_CLK_ENABLE();

        gpio_init.Pin       = SPI5_SCK_PIN | SPI5_MISO_PIN | SPI5_MOSI_PIN;
        gpio_init.Mode      = GPIO_MODE_AF_PP;
        gpio_init.Pull      = GPIO_PULLUP;
        gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = SPI5_GPIO_AF;
        HAL_GPIO_Init(SPI5_GPIO_PORT, &gpio_init);
        /* ---- MSP end ---- */

        hspi->Instance       = SPI5;
        hspi->Init.CLKPolarity = SPI_POLARITY_HIGH; /* mode 3 for the flash */
        hspi->Init.CLKPhase    = SPI_PHASE_2EDGE;
    }

    hspi->Init.Mode              = SPI_MODE_MASTER;
    hspi->Init.Direction         = SPI_DIRECTION_2LINES;
    hspi->Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi->Init.NSS               = SPI_NSS_SOFT;
    /* f_SCK = f_PCLK/prescaler: SPI5 (APB2 90 MHz)/256 = 351.56 kHz,
     * SPI2 (APB1 45 MHz)/256 = 175.78 kHz. */
    hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi->Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi->Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi->Init.CRCPolynomial     = 7U;
    (void)HAL_SPI_Init(hspi);

    __HAL_SPI_ENABLE(hspi);

    (void)spi_read_write_byte(bus, SPI_DUMMY_BYTE); /* flush the shift register */
}

/* Baud-rate prescaler lives in CR1[5:3]; keep every other CR1 bit. */
static void spi_baudrate_set(SPI_HandleTypeDef *hspi, uint8_t prescaler)
{
    MODIFY_REG(hspi->Instance->CR1, SPI_CR1_BR, prescaler);
}

void spi_set_speed(spi_bus_t bus, uint8_t prescaler)
{
    SPI_HandleTypeDef *hspi = spi_handle(bus);

    __HAL_SPI_DISABLE(hspi);
    spi_baudrate_set(hspi, prescaler);
    __HAL_SPI_ENABLE(hspi);
}

uint8_t spi_read_write_byte(spi_bus_t bus, uint8_t txdata)
{
    uint8_t rxdata = 0U;

    (void)HAL_SPI_TransmitReceive(spi_handle(bus), &txdata, &rxdata, 1U, SPI_TIMEOUT_MS);

    return rxdata;
}

/* ---- W25Q256JV chip ---- */
/**
 * @file    nor_w25q256jv.c
 * @brief   Winbond W25Q256JV (256 Mbit / 32 MB) SPI NOR chip driver: the SPI
 *          bus (SPI_BUS_NORFLASH), chip select (PF6), JEDEC probe and the
 *          address width. NOR command algorithms stay in nor.c.
 */


#define NOR_W25Q256JV_CS_PORT   GPIOF
#define NOR_W25Q256JV_CS_PIN    GPIO_PIN_6

#define NOR_W25Q256JV_ADDR_BYTES 4U
#define NOR_MANUFACT_DEVICE_ID   0x90U


void nor_w25q256jv_cs_low(void)
{
    HAL_GPIO_WritePin(NOR_W25Q256JV_CS_PORT, NOR_W25Q256JV_CS_PIN, GPIO_PIN_RESET);
}

void nor_w25q256jv_cs_high(void)
{
    HAL_GPIO_WritePin(NOR_W25Q256JV_CS_PORT, NOR_W25Q256JV_CS_PIN, GPIO_PIN_SET);
}

uint8_t nor_w25q256jv_spi_rw(uint8_t data)
{
    return spi_read_write_byte(SPI_BUS_NORFLASH, data);
}

void nor_w25q256jv_dev_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* ---- MSP begin: CS clock + pin ---- */
    __HAL_RCC_GPIOF_CLK_ENABLE();

    gpio_init.Pin   = NOR_W25Q256JV_CS_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(NOR_W25Q256JV_CS_PORT, &gpio_init);
    /* ---- MSP end ---- */

    NOR_CS_HIGH();

    spi_init(SPI_BUS_NORFLASH);
    spi_set_speed(SPI_BUS_NORFLASH, SPI_BAUDRATEPRESCALER_4);
}

uint16_t nor_w25q256jv_probe(void)
{
    uint16_t deviceid;

    NOR_CS_LOW();
    (void)nor_w25q256jv_spi_rw(NOR_MANUFACT_DEVICE_ID);
    (void)nor_w25q256jv_spi_rw(0x00U);
    (void)nor_w25q256jv_spi_rw(0x00U);
    (void)nor_w25q256jv_spi_rw(0x00U);
    deviceid  = (uint16_t)(nor_w25q256jv_spi_rw(0xFFU) << 8);
    deviceid |= nor_w25q256jv_spi_rw(0xFFU);
    NOR_CS_HIGH();

    return deviceid;
}

uint8_t nor_w25q256jv_addr_bytes(void)
{
    return NOR_W25Q256JV_ADDR_BYTES;
}