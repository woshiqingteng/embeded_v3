#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "stm32f4xx_hal.h"
#include "bsp_internal_flash.h"
#include "bsp_log.h"

static SemaphoreHandle_t g_flash_lock;
static bool g_flash_ready;

static int int_flash_lock(void)
{
    if (g_flash_lock == 0)
    {
        return -1;
    }
    return (xSemaphoreTake(g_flash_lock, portMAX_DELAY) == pdTRUE) ? 0 : -1;
}

static void int_flash_unlock(void)
{
    if (g_flash_lock != 0)
    {
        (void)xSemaphoreGive(g_flash_lock);
    }
}

static void int_flash_clear_flags(void)
{
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
}

int bsp_internal_flash_init(void)
{
    if (g_flash_lock == 0)
    {
        g_flash_lock = xSemaphoreCreateMutex();
    }
    if (g_flash_lock == 0)
    {
        return -1;
    }

    g_flash_ready = true;
    BSP_LOGI("iflash", "log area 0x%08lx size=%lu KiB sectors=%lu-%lu",
             (unsigned long)BSP_INT_FLASH_BASE,
             (unsigned long)(BSP_INT_FLASH_SIZE / 1024U),
             (unsigned long)BSP_INT_FLASH_SECTOR_FIRST,
             (unsigned long)(BSP_INT_FLASH_SECTOR_FIRST + BSP_INT_FLASH_SECTOR_COUNT - 1U));
    return 0;
}

int bsp_internal_flash_read(uint32_t addr, void *buf, uint32_t len)
{
    if ((buf == 0) || (addr < BSP_INT_FLASH_BASE) ||
        ((addr + len) > (BSP_INT_FLASH_BASE + BSP_INT_FLASH_SIZE)))
    {
        return -1;
    }
    if (!g_flash_ready)
    {
        return -1;
    }
    if (int_flash_lock() != 0)
    {
        return -1;
    }
    memcpy(buf, (const void *)addr, len);
    int_flash_unlock();
    return 0;
}

int bsp_internal_flash_prog(uint32_t addr, const void *buf, uint32_t len)
{
    const uint8_t *src = (const uint8_t *)buf;
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t i = 0U;

    if ((buf == 0) || (addr < BSP_INT_FLASH_BASE) ||
        ((addr + len) > (BSP_INT_FLASH_BASE + BSP_INT_FLASH_SIZE)) ||
        ((addr & 0x3U) != 0U))
    {
        return -1;
    }
    if (!g_flash_ready || (int_flash_lock() != 0))
    {
        return -1;
    }

    HAL_FLASH_Unlock();
    int_flash_clear_flags();

    while ((i + 4U) <= len)
    {
        uint32_t word;
        memcpy(&word, src + i, sizeof(word));
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word);
        if (status != HAL_OK)
        {
            break;
        }
        i += 4U;
    }

    while ((status == HAL_OK) && (i < len))
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr + i, src[i]);
        i++;
    }

    HAL_FLASH_Lock();
    int_flash_unlock();
    return (status == HAL_OK) ? 0 : -1;
}

int bsp_internal_flash_erase_range(uint32_t addr, uint32_t len)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t sector_error = 0U;
    uint32_t first_sector;
    uint32_t sector_count;
    HAL_StatusTypeDef status;

    if ((addr < BSP_INT_FLASH_BASE) ||
        ((addr + len) > (BSP_INT_FLASH_BASE + BSP_INT_FLASH_SIZE)) ||
        ((addr % BSP_INT_FLASH_SECTOR_SIZE) != 0U) ||
        ((len % BSP_INT_FLASH_SECTOR_SIZE) != 0U))
    {
        return -1;
    }
    if (!g_flash_ready || (int_flash_lock() != 0))
    {
        return -1;
    }

    first_sector = BSP_INT_FLASH_SECTOR_FIRST +
                   ((addr - BSP_INT_FLASH_BASE) / BSP_INT_FLASH_SECTOR_SIZE);
    sector_count = len / BSP_INT_FLASH_SECTOR_SIZE;

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.Sector       = first_sector;
    erase.NbSectors    = sector_count;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    HAL_FLASH_Unlock();
    int_flash_clear_flags();
    status = HAL_FLASHEx_Erase(&erase, &sector_error);
    HAL_FLASH_Lock();
    int_flash_unlock();
    return (status == HAL_OK) ? 0 : -1;
}
