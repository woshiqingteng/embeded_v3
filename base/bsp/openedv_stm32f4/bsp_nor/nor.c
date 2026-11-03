/**
 * @file    nor.c
 * @brief   SPI NOR flash storage: device-independent command algorithms
 *          (read / page program / sector erase) over the chip driver
 *          (nor_w25q256jv, which owns the SPI bus and chip select).
 */

#include "stm32f4xx_hal.h"
#include "nor.h"
#include "nor_w25q256jv.h"

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

static uint8_t g_nor_buf[NOR_SECTOR_SIZE_BYTE];

#define NOR_CS_LOW()   nor_w25q256jv_cs_low()
#define NOR_CS_HIGH()  nor_w25q256jv_cs_high()

static uint8_t nor_spi_rw(uint8_t data)
{
    return nor_w25q256jv_spi_rw(data);
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

static void nor_wait_busy(void)
{
    while ((nor_read_sr(1) & FLASH_SR1_BUSY_BIT) == FLASH_SR1_BUSY_BIT)
    {
    }
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

void nor_init(void)
{
    nor_w25q256jv_dev_init();
    (void)nor_w25q256jv_probe();

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

uint16_t nor_read_id(void)
{
    return nor_w25q256jv_probe();
}

void nor_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t i;

    if (addr >= NOR_DEVICE_SIZE_BYTE)
    {
        return;
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
}

static void nor_write_page(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t i;

    nor_write_enable();

    NOR_CS_LOW();
    (void)nor_spi_rw(FLASH_PageProgram);
    nor_send_address(addr);

    for (i = 0U; i < datalen; i++)
    {
        (void)nor_spi_rw(pbuf[i]);
    }

    NOR_CS_HIGH();
    nor_wait_busy();
}

static void nor_write_nocheck(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t pageremain = (uint16_t)(NOR_PAGE_SIZE_BYTE - (addr % NOR_PAGE_SIZE_BYTE));

    if (datalen <= pageremain)
    {
        pageremain = datalen;
    }

    for (;;)
    {
        nor_write_page(pbuf, addr, pageremain);

        if (datalen == pageremain)
        {
            break;
        }

        pbuf      += pageremain;
        addr      += pageremain;
        datalen   -= pageremain;
        pageremain = (datalen > NOR_PAGE_SIZE_BYTE) ? NOR_PAGE_SIZE_BYTE : datalen;
    }
}

void nor_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint32_t secpos;
    uint16_t secoff;
    uint16_t secremain;
    uint16_t i;

    if (addr >= NOR_DEVICE_SIZE_BYTE)
    {
        return;
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
        nor_read(g_nor_buf, secpos * NOR_SECTOR_SIZE_BYTE, NOR_SECTOR_SIZE_BYTE);

        for (i = 0U; i < secremain; i++)
        {
            if (g_nor_buf[secoff + i] != 0xFFU)
            {
                break;
            }
        }

        if (i < secremain) /* sector is not blank: erase and merge */
        {
            nor_erase_sector(secpos);

            for (i = 0U; i < secremain; i++)
            {
                g_nor_buf[i + secoff] = pbuf[i];
            }

            nor_write_nocheck(g_nor_buf, secpos * NOR_SECTOR_SIZE_BYTE, NOR_SECTOR_SIZE_BYTE);
        }
        else
        {
            nor_write_nocheck(pbuf, addr, secremain);
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
}

void nor_write_erased(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    if (addr >= NOR_DEVICE_SIZE_BYTE)
    {
        return;
    }

    if ((addr + datalen) > NOR_DEVICE_SIZE_BYTE)
    {
        datalen = (uint16_t)(NOR_DEVICE_SIZE_BYTE - addr);
    }

    nor_write_nocheck(pbuf, addr, datalen);
}

void nor_erase_sector(uint32_t saddr)
{
    saddr *= NOR_SECTOR_SIZE_BYTE;

    if (saddr >= NOR_DEVICE_SIZE_BYTE)
    {
        return;
    }

    nor_write_enable();
    nor_wait_busy();

    NOR_CS_LOW();
    (void)nor_spi_rw(FLASH_SectorErase);
    nor_send_address(saddr);
    NOR_CS_HIGH();
    nor_wait_busy();
}

void nor_erase_block64(uint32_t addr)
{
    if (addr >= NOR_DEVICE_SIZE_BYTE)
    {
        return;
    }

    nor_write_enable();
    nor_wait_busy();

    NOR_CS_LOW();
    (void)nor_spi_rw(FLASH_BlockErase64k);
    nor_send_address(addr);
    NOR_CS_HIGH();
    nor_wait_busy();
}
