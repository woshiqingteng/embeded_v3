#include "ff.h"
#include "diskio.h"
#include "bsp_sd.h"

#define SD_PDRV 0

DSTATUS disk_status(BYTE pdrv)
{
    sd_card_info_t info;

    if (pdrv != SD_PDRV)
    {
        return STA_NOINIT;
    }
    return (bsp_sd_get_info(&info) == 0) ? 0 : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != SD_PDRV)
    {
        return STA_NOINIT;
    }
    return (bsp_sd_init() == 0) ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != SD_PDRV)
    {
        return RES_PARERR;
    }
    return (bsp_sd_read(buff, (uint32_t)sector, (uint32_t)count) == 0)
               ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != SD_PDRV)
    {
        return RES_PARERR;
    }
    return (bsp_sd_write(buff, (uint32_t)sector, (uint32_t)count) == 0)
               ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    sd_card_info_t info;

    if (pdrv != SD_PDRV)
    {
        return RES_PARERR;
    }

    switch (cmd)
    {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_COUNT:
            if (bsp_sd_get_info(&info) != 0)
            {
                return RES_ERROR;
            }
            *(LBA_t *)buff = info.block_count;
            return RES_OK;

        case GET_SECTOR_SIZE:
            if (bsp_sd_get_info(&info) != 0)
            {
                return RES_ERROR;
            }
            *(WORD *)buff = (WORD)info.block_size;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1U;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}
