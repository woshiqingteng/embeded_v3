#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"

#include "app_ipc.h"
#include "app_state.h"
#include "bsp_adc.h"
#include "oled.h"
#include "bsp_pwm.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_uart.h"
#include "bsp_sd.h"
#include "bsp_rtc.h"
#include "bsp_log.h"
#include "nor.h"
#include "fault_info.h"
#include <errno.h>
#include "fs.h"
#include "storage_manager.h"
#include "elog.h"
#include "mw_log.h"

static int cmd_stats(int argc, char *argv[]);
static int cmd_led(int argc, char *argv[]);
static int cmd_adc(int argc, char *argv[]);
static int cmd_uart(int argc, char *argv[]);
static int cmd_clock(int argc, char *argv[]);
static int cmd_uptime(int argc, char *argv[]);
static int cmd_heap(int argc, char *argv[]);
static int cmd_tasks(int argc, char *argv[]);
static int cmd_fault(int argc, char *argv[]);
static int cmd_nor(int argc, char *argv[]);
static int cmd_pwm(int argc, char *argv[]);
static int cmd_key(int argc, char *argv[]);
static int cmd_sd(int argc, char *argv[]);
static int cmd_fs(int argc, char *argv[]);
static int cmd_lfs(int argc, char *argv[]);
static int cmd_log(int argc, char *argv[]);
static int cmd_usb(int argc, char *argv[]);
static int cmd_wdg(int argc, char *argv[]);
static int cmd_oled(int argc, char *argv[]);
static int cmd_rtc(int argc, char *argv[]);

#define APP_CMD(_name, _func, _desc) \
    SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | \
                         SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | \
                         SHELL_CMD_DISABLE_RETURN, \
                     _name, _func, _desc)

APP_CMD(stats, cmd_stats, show run stats);
APP_CMD(uptime, cmd_uptime, show uptime);
APP_CMD(heap, cmd_heap, show heap usage);
APP_CMD(tasks, cmd_tasks, list tasks);
APP_CMD(clock, cmd_clock, show clocks);
APP_CMD(fault, cmd_fault, show last fault);
APP_CMD(led, cmd_led, led on/off/toggle/status);
APP_CMD(adc, cmd_adc, show adc mv);
APP_CMD(uart, cmd_uart, uart status);
APP_CMD(pwm, cmd_pwm, set/get pwm permille);
APP_CMD(key, cmd_key, key status/inject);
APP_CMD(nor, cmd_nor, nor id/info/read);
APP_CMD(fs, cmd_fs, unified fs mount/umount/ls/cat/stat/df/mkdir/rm/mv);
APP_CMD(sd, cmd_sd, fatfs mount/umount/ls/info/cat/dma);
APP_CMD(lfs, cmd_lfs, littlefs mount/umount/ls/info/df/cat);
APP_CMD(log, cmd_log, log test/fill/level/tag/flush/status);
APP_CMD(usb, cmd_usb, usb status/readonly);
APP_CMD(wdg, cmd_wdg, watchdog status);
APP_CMD(rtc, cmd_rtc, rtc show/set);
APP_CMD(oled, cmd_oled, oled on/off/clear/text/status);

SHELL_EXPORT_CMD_AGENCY(
    SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
        SHELL_CMD_DISABLE_RETURN,
    sh, shellRun, run command directly,
    shellGetCurrent(), (const char *)p1);

static const char s_nl[] = { (char)13, (char)10, '\0' };

static int cmd_stats(int argc, char *argv[])
{
    app_state_t st;
    Shell *sh = shellGetCurrent();
    (void)argc;
    (void)argv;
    app_state_get(&st);
    shellPrint(sh, "heap=%lu B tasks=%lu pwm=%u key=%lu usb=%u",
               (unsigned long)xPortGetFreeHeapSize(),
               (unsigned long)uxTaskGetNumberOfTasks(),
               st.pwm_permille, (unsigned long)st.last_key,
               st.usb_connected ? 1U : 0U);
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_adc(int argc, char *argv[])
{
    uint16_t mv[6];
    Shell *sh = shellGetCurrent();
    (void)argc;
    (void)argv;
    (void)bsp_adc_get_mv(mv);
    shellPrint(sh, "adc %u %u %u %u %u %u mV",
               mv[0], mv[1], mv[2], mv[3], mv[4], mv[5]);
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_pwm(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    if (argc > 1)
    {
        int v = atoi(argv[1]);
        if (v < 0)
        {
            v = 0;
        }
        if (v > 1000)
        {
            v = 1000;
        }
        bsp_pwm_set_duty((uint16_t)v);
        app_state_set_pwm((uint16_t)v);
    }
    shellPrint(sh, "pwm=%u permille", bsp_pwm_get_duty());
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_key(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();

    if ((argc > 1) && (strcmp(argv[1], "status") == 0))
    {
        shellPrint(sh, "key0=%u key1=%u key2=%u wkup=%u",
                   bsp_key_is_pressed(BSP_KEY0) ? 1U : 0U,
                   bsp_key_is_pressed(BSP_KEY1) ? 1U : 0U,
                   bsp_key_is_pressed(BSP_KEY2) ? 1U : 0U,
                   bsp_key_is_pressed(BSP_KEY_WKUP) ? 1U : 0U);
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    if (argc > 1)
    {
        app_key_notify((uint32_t)atoi(argv[1]));
    }
    shellPrint(sh, "key injected 0..3");
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_led(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    int id;
    const char *op;
    bsp_led_t led;

    if (argc < 3)
    {
        shellPrint(sh, "led <0|1> on|off|toggle|status");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    id = atoi(argv[1]);
    if ((id < 0) || (id >= (int)BSP_LED_NUM))
    {
        shellPrint(sh, "invalid led");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }
    led = (bsp_led_t)id;
    op = argv[2];

    if ((id == 0) && (strcmp(op, "status") != 0))
    {
        shellPrint(sh, "led0 is system heartbeat; use led1 for control");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    if (strcmp(op, "on") == 0)
    {
        bsp_led_on(led);
    }
    else if (strcmp(op, "off") == 0)
    {
        bsp_led_off(led);
    }
    else if (strcmp(op, "toggle") == 0)
    {
        bsp_led_toggle(led);
    }

    shellPrint(sh, "led%d=%s", id, bsp_led_is_on(led) ? "on" : "off");
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_uart(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    int id;
    bsp_uart_t uart;

    if (argc < 2)
    {
        shellPrint(sh, "uart <1|2> status");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    id = atoi(argv[1]);
    if ((id < 1) || (id > 2))
    {
        shellPrint(sh, "invalid uart");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }
    uart = (id == 1) ? BSP_UART1 : BSP_UART2;
    shellPrint(sh, "uart%d baud=%lu busy=%u",
               id,
               (unsigned long)bsp_uart_get_baud(uart),
               bsp_uart_tx_busy(uart) ? 1U : 0U);
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_clock(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    (void)argc;
    (void)argv;
    shellPrint(sh, "sysclk=%lu MHz hclk=%lu MHz pclk1=%lu MHz pclk2=%lu MHz",
               (unsigned long)(HAL_RCC_GetSysClockFreq() / 1000000U),
               (unsigned long)(HAL_RCC_GetHCLKFreq() / 1000000U),
               (unsigned long)(HAL_RCC_GetPCLK1Freq() / 1000000U),
               (unsigned long)(HAL_RCC_GetPCLK2Freq() / 1000000U));
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_uptime(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    (void)argc;
    (void)argv;
    shellPrint(sh, "tick=%lu ms", (unsigned long)HAL_GetTick());
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_heap(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    (void)argc;
    (void)argv;
    shellPrint(sh, "heap_free=%lu B heap_min=%lu B tasks=%lu",
               (unsigned long)xPortGetFreeHeapSize(),
               (unsigned long)xPortGetMinimumEverFreeHeapSize(),
               (unsigned long)uxTaskGetNumberOfTasks());
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_tasks(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    char buf[512];

    (void)argc;
    (void)argv;
    vTaskList(buf);
    (void)shellWriteString(sh, buf);
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_fault(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    (void)argc;
    (void)argv;

    if (g_fault_info.magic != APP_FAULT_MAGIC)
    {
        (void)shellWriteString(sh, "no fault record");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    shellPrint(sh, "type=%lu cfsr=%08lx hfsr=%08lx pc=%08lx lr=%08lx file=%s:%lu task=%s",
               (unsigned long)g_fault_info.type,
               (unsigned long)g_fault_info.cfsr,
               (unsigned long)g_fault_info.hfsr,
               (unsigned long)g_fault_info.pc,
               (unsigned long)g_fault_info.lr,
               g_fault_info.file,
               (unsigned long)g_fault_info.line,
               g_fault_info.task);
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_nor(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    uint8_t buf[64];
    uint32_t addr;
    uint32_t len;

    if (argc < 2)
    {
        shellPrint(sh, "nor id|info|read <addr> <len>");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    if (strcmp(argv[1], "id") == 0)
    {
        shellPrint(sh, "nor jedec=0x%04x", (unsigned)nor_read_id());
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    if (strcmp(argv[1], "info") == 0)
    {
        shellPrint(sh, "nor size=%lu MiB sector=%lu B page=%lu B",
                   (unsigned long)(NOR_DEVICE_SIZE_BYTE / (1024U * 1024U)),
                   (unsigned long)NOR_SECTOR_SIZE_BYTE,
                   (unsigned long)NOR_PAGE_SIZE_BYTE);
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    if ((strcmp(argv[1], "read") != 0) || (argc < 4))
    {
        shellPrint(sh, "nor read <addr> <len>; max %u", (unsigned)sizeof(buf));
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    addr = (uint32_t)strtoul(argv[2], 0, 0);
    len = (uint32_t)strtoul(argv[3], 0, 0);
    if (len > sizeof(buf))
    {
        len = sizeof(buf);
    }
    if ((addr >= NOR_DEVICE_SIZE_BYTE) || (len == 0U))
    {
        shellPrint(sh, "nor range invalid");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }
    if ((addr + len) > NOR_DEVICE_SIZE_BYTE)
    {
        len = NOR_DEVICE_SIZE_BYTE - addr;
    }

    if (nor_read(buf, addr, (uint16_t)len) != 0)
    {
        shellPrint(sh, "nor read failed/timeout");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }
    for (uint32_t i = 0U; i < len; i++)
    {
        if ((i % 16U) == 0U)
        {
            shellPrint(sh, "%08lx:", (unsigned long)(addr + i));
        }
        shellPrint(sh, " %02x", buf[i]);
        if (((i % 16U) == 15U) || ((i + 1U) == len))
        {
            (void)shellWriteString(sh, s_nl);
        }
    }
    return 0;
}

static void shell_newline(Shell *sh)
{
    (void)shellWriteString(sh, s_nl);
}

static void fs_join(const char *mount, const char *path, char *out, size_t size)
{
    if ((path == 0) || (path[0] == '\0'))
    {
        (void)snprintf(out, size, "%s/", mount);
    }
    else if (path[0] == '/')
    {
        (void)snprintf(out, size, "%s%s", mount, path);
    }
    else
    {
        (void)snprintf(out, size, "%s/%s", mount, path);
    }
}

static int fs_do_ls(Shell *sh, const char *path)
{
    fs_dir_t    dir;
    fs_dirent_t entry;
    int         ret;

    ret = fs_opendir(&dir, path);
    if (ret != 0)
    {
        shellPrint(sh, "fs open dir failed: %d", ret);
        shell_newline(sh);
        return ret;
    }
    for (;;)
    {
        ret = fs_readdir(&dir, &entry);
        if (ret != 0)
        {
            break;
        }
        shellPrint(sh, "%c %8lu %s",
                   (entry.type == FS_DIR_ENTRY_DIR) ? 'd' : '-',
                   (unsigned long)entry.size,
                   entry.name);
        shell_newline(sh);
    }
    (void)fs_closedir(&dir);
    return (ret == 1) ? 0 : ret;
}

static int fs_do_cat(Shell *sh, const char *path)
{
    fs_file_t fp;
    char      buf[129];
    ssize_t   n;
    int       ret;

    ret = fs_open(&fp, path, FS_O_READ);
    if (ret != 0)
    {
        shellPrint(sh, "fs open failed: %d", ret);
        shell_newline(sh);
        return ret;
    }
    while ((n = fs_read(&fp, buf, sizeof(buf) - 1U)) > 0)
    {
        buf[n] = '\0';
        (void)shellWriteString(sh, buf);
    }
    (void)fs_close(&fp);
    if (n < 0)
    {
        shellPrint(sh, "fs read failed: %d", (int)n);
        shell_newline(sh);
        return (int)n;
    }
    return 0;
}

static int fs_do_stat(Shell *sh, const char *path)
{
    fs_dirent_t entry;
    int         ret = fs_stat(path, &entry);

    if (ret != 0)
    {
        shellPrint(sh, "fs stat failed: %d", ret);
        shell_newline(sh);
        return ret;
    }
    shellPrint(sh, "%c %8lu %s",
               (entry.type == FS_DIR_ENTRY_DIR) ? 'd' : '-',
               (unsigned long)entry.size,
               entry.name);
    shell_newline(sh);
    return 0;
}

static int fs_do_df(Shell *sh, const char *path)
{
    fs_statvfs_t st;
    int          ret = fs_statvfs(path, &st);

    if (ret != 0)
    {
        shellPrint(sh, "fs df failed: %d", ret);
        shell_newline(sh);
        return ret;
    }
    shellPrint(sh, "block=%lu total=%lu free=%lu bytes=%lu free_bytes=%lu",
               st.block_size, st.total_blocks, st.free_blocks,
               st.total_bytes, st.free_bytes);
    shell_newline(sh);
    return 0;
}

static int cmd_fs(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    const char *op;
    const char *path;
    int ret = 0;

    if (argc < 2)
    {
        shellPrint(sh, "fs mount|umount|state|ls|cat|stat|df|mkdir|rm|mv <path> [path]");
        shell_newline(sh);
        return 0;
    }
    op = argv[1];

    if (strcmp(op, "state") == 0)
    {
        shellPrint(sh, "storage=%s sd=%u flash=%u nor=%u",
                   storage_state_name(),
                   fs_is_mounted(FS_MOUNT_SD) ? 1U : 0U,
                   fs_is_mounted(FS_MOUNT_FLASH) ? 1U : 0U,
                   fs_is_mounted(FS_MOUNT_NOR) ? 1U : 0U);
        shell_newline(sh);
        return 0;
    }

    if (strcmp(op, "mount") == 0)
    {
        path = (argc > 2) ? argv[2] : FS_MOUNT_SD;
        if (strcmp(path, FS_MOUNT_SD) == 0)
        {
            ret = storage_fatfs_mount();
        }
        else if (strcmp(path, FS_MOUNT_FLASH) == 0)
        {
            ret = fs_is_mounted(FS_MOUNT_FLASH) ? 0 : fs_mount(fs_logfs_mount_obj());
        }
        else if (strcmp(path, FS_MOUNT_NOR) == 0)
        {
            ret = fs_is_mounted(FS_MOUNT_NOR) ? 0 : fs_mount(fs_littlefs_mount_obj());
        }
        else
        {
            ret = -EINVAL;
        }
        shellPrint(sh, "fs mount %s ret=%d", path, ret);
        shell_newline(sh);
        return ret;
    }

    if (strcmp(op, "umount") == 0)
    {
        path = (argc > 2) ? argv[2] : FS_MOUNT_SD;
        if (strcmp(path, FS_MOUNT_SD) == 0)
        {
            ret = storage_fatfs_umount();
        }
        else if (strcmp(path, FS_MOUNT_FLASH) == 0)
        {
            ret = fs_unmount(fs_logfs_mount_obj());
        }
        else if (strcmp(path, FS_MOUNT_NOR) == 0)
        {
            ret = fs_unmount(fs_littlefs_mount_obj());
        }
        else
        {
            ret = -EINVAL;
        }
        shellPrint(sh, "fs umount %s ret=%d", path, ret);
        shell_newline(sh);
        return ret;
    }

    path = (argc > 2) ? argv[2] : FS_MOUNT_FLASH;

    if (strcmp(op, "ls") == 0)
    {
        (void)fs_do_ls(sh, path);
    }
    else if (strcmp(op, "cat") == 0)
    {
        (void)fs_do_cat(sh, path);
    }
    else if (strcmp(op, "stat") == 0)
    {
        (void)fs_do_stat(sh, path);
    }
    else if (strcmp(op, "df") == 0)
    {
        (void)fs_do_df(sh, path);
    }
    else if (strcmp(op, "mkdir") == 0)
    {
        ret = fs_mkdir(path);
        shellPrint(sh, "fs mkdir ret=%d", ret);
        shell_newline(sh);
    }
    else if (strcmp(op, "rm") == 0)
    {
        ret = fs_unlink(path);
        shellPrint(sh, "fs rm ret=%d", ret);
        shell_newline(sh);
    }
    else if (strcmp(op, "mv") == 0)
    {
        if (argc > 3)
        {
            ret = fs_rename(path, argv[3]);
        }
        else
        {
            ret = -EINVAL;
        }
        shellPrint(sh, "fs mv ret=%d", ret);
        shell_newline(sh);
    }
    else
    {
        shellPrint(sh, "fs: unknown op");
        shell_newline(sh);
    }
    return ret;
}

static int cmd_sd(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    char path[160];

    if (argc < 2)
    {
        shellPrint(sh, "sd mount|umount|ls|info|cat|test|stats|dma on/off/status");
        shell_newline(sh);
        return 0;
    }

    if (strcmp(argv[1], "dma") == 0)
    {
        const char *op = (argc > 2) ? argv[2] : "status";

        if (strcmp(op, "on") == 0)
        {
            shellPrint(sh, "sd dma %s", (bsp_sd_set_dma_enabled(true) == 0) ? "on" : "failed; blocking mode kept");
        }
        else if (strcmp(op, "off") == 0)
        {
            (void)bsp_sd_set_dma_enabled(false);
            shellPrint(sh, "sd dma off");
        }
        else
        {
            shellPrint(sh, "sd dma=%s", bsp_sd_dma_enabled() ? "on" : "off");
        }
        shell_newline(sh);
        return 0;
    }

    if (strcmp(argv[1], "info") == 0)
    {
        sd_card_info_t info;

        if (bsp_sd_get_info(&info) != 0)
        {
            shellPrint(sh, "sd not ready");
        }
        else
        {
            shellPrint(sh, "sd size=%lu MiB blocks=%lu block_size=%lu B readonly=%u owner=%u dma=%u state=%s",
                       (unsigned long)info.total_size_mb,
                       (unsigned long)info.block_count,
                       (unsigned long)info.block_size,
                       bsp_sd_is_writable() ? 0U : 1U,
                       (unsigned)bsp_sd_owner(),
                       bsp_sd_dma_enabled() ? 1U : 0U,
                       storage_state_name());
        }
        shell_newline(sh);
        return 0;
    }

    if (strcmp(argv[1], "stats") == 0)
    {
        bsp_sd_diag_t d;

        bsp_sd_get_diag(&d);
        shellPrint(sh, "dma selftest=%lu sdio_irq=%lu dma_irq=%lu error=0x%08lx start=%lu",
                   (unsigned long)d.selftest,
                   (unsigned long)d.sdio_irq,
                   (unsigned long)d.dma_irq,
                   (unsigned long)d.error,
                   (unsigned long)d.start_status);
        shell_newline(sh);
        return 0;
    }

    if (strcmp(argv[1], "test") == 0)
    {
        static uint8_t blk[4096];
        uint32_t lba = (argc > 2) ? (uint32_t)atoi(argv[2]) : 0U;
        uint32_t count = (argc > 3) ? (uint32_t)atoi(argv[3]) : 1U;
        uint32_t last = (count > 1U) ? count - 1U : 0U;
        int ret;

        if (count == 0U)
        {
            count = 1U;
        }
        if (count > 8U)
        {
            count = 8U;
            last = 7U;
        }
        ret = bsp_sd_read(blk, lba, count);
        if (ret == 0)
        {
            shellPrint(sh, "sd read lba=%lu cnt=%lu %02x %02x %02x %02x sig=%02x%02x",
                       (unsigned long)lba, (unsigned long)count,
                       blk[0], blk[1], blk[2], blk[3],
                       blk[last * 512U + 510U], blk[last * 512U + 511U]);
        }
        else
        {
            shellPrint(sh, "sd read lba=%lu cnt=%lu failed ret=%d",
                       (unsigned long)lba, (unsigned long)count, ret);
        }
        shell_newline(sh);
        return 0;
    }

    if (strcmp(argv[1], "mount") == 0)
    {
        shellPrint(sh, "sd mount ret=%d", storage_fatfs_mount());
        shell_newline(sh);
    }
    else if (strcmp(argv[1], "umount") == 0)
    {
        shellPrint(sh, "sd umount ret=%d", storage_fatfs_umount());
        shell_newline(sh);
    }
    else if (strcmp(argv[1], "cat") == 0)
    {
        if (argc < 3)
        {
            shellPrint(sh, "sd cat <file>");
            shell_newline(sh);
            return 0;
        }
        fs_join(FS_MOUNT_SD, argv[2], path, sizeof(path));
        (void)fs_do_cat(sh, path);
    }
    else if (strcmp(argv[1], "ls") == 0)
    {
        fs_join(FS_MOUNT_SD, (argc > 2) ? argv[2] : "/", path, sizeof(path));
        (void)fs_do_ls(sh, path);
    }
    return 0;
}

static int cmd_lfs(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    char path[160];

    if (argc < 2)
    {
        shellPrint(sh, "lfs mount|umount|ls|info|df|cat <file>");
        shell_newline(sh);
        return 0;
    }

    if (strcmp(argv[1], "mount") == 0)
    {
        shellPrint(sh, "lfs mount ret=%d",
                   fs_is_mounted(FS_MOUNT_NOR) ? 0 : fs_mount(fs_littlefs_mount_obj()));
        shell_newline(sh);
    }
    else if (strcmp(argv[1], "umount") == 0)
    {
        shellPrint(sh, "lfs umount ret=%d", fs_unmount(fs_littlefs_mount_obj()));
        shell_newline(sh);
    }
    else if ((strcmp(argv[1], "info") == 0) || (strcmp(argv[1], "df") == 0))
    {
        (void)fs_do_df(sh, FS_MOUNT_NOR);
    }
    else if (strcmp(argv[1], "cat") == 0)
    {
        if (argc < 3)
        {
            shellPrint(sh, "lfs cat <file>");
            shell_newline(sh);
            return 0;
        }
        fs_join(FS_MOUNT_NOR, argv[2], path, sizeof(path));
        (void)fs_do_cat(sh, path);
    }
    else if (strcmp(argv[1], "ls") == 0)
    {
        fs_join(FS_MOUNT_NOR, (argc > 2) ? argv[2] : "/", path, sizeof(path));
        (void)fs_do_ls(sh, path);
    }
    return 0;
}

static int cmd_rtc(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    bsp_rtc_datetime_t dt;
    char buf[24];

    if ((argc >= 2) && (strcmp(argv[1], "set") == 0))
    {
        unsigned y, mo, d, h, mi, s;

        if ((argc < 4) ||
            (sscanf(argv[2], "%u-%u-%u", &y, &mo, &d) != 3) ||
            (sscanf(argv[3], "%u:%u:%u", &h, &mi, &s) != 3))
        {
            shellPrint(sh, "rtc set YYYY-MM-DD HH:MM:SS");
            shell_newline(sh);
            return 0;
        }
        dt.year = (uint16_t)y;
        dt.month = (uint8_t)mo;
        dt.date = (uint8_t)d;
        dt.hour = (uint8_t)h;
        dt.minute = (uint8_t)mi;
        dt.second = (uint8_t)s;
        dt.weekday = 0U;
        shellPrint(sh, "rtc set ret=%d", bsp_rtc_set_datetime(&dt));
        shell_newline(sh);
        return 0;
    }

    bsp_rtc_format(buf, sizeof(buf));
    shellPrint(sh, "%s", buf);
    shell_newline(sh);
    return 0;
}

static int cmd_log(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();

    if ((argc < 2) || (strcmp(argv[1], "test") == 0))
    {
        elog_info("shell", "log test tick=%lu", (unsigned long)HAL_GetTick());
        return 0;
    }

    if (strcmp(argv[1], "fill") == 0)
    {
        uint32_t kb = (argc > 2) ? (uint32_t)atoi(argv[2]) : 300U;

        shellPrint(sh, "log fill ret=%d", mw_log_fill(kb * 1024U));
        shell_newline(sh);
        return 0;
    }

    if (strcmp(argv[1], "level") == 0)
    {
        int level = (argc > 2) ? atoi(argv[2]) : (int)ELOG_LVL_INFO;

        if (level < (int)ELOG_LVL_ASSERT)
        {
            level = (int)ELOG_LVL_ASSERT;
        }
        if (level > (int)ELOG_LVL_VERBOSE)
        {
            level = (int)ELOG_LVL_VERBOSE;
        }
        elog_set_filter_lvl((uint8_t)level);
        shellPrint(sh, "log level=%d", level);
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    if (strcmp(argv[1], "tag") == 0)
    {
        elog_set_filter_tag((argc > 2) ? argv[2] : "");
        shellPrint(sh, "log tag=%s", (argc > 2) ? argv[2] : "");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    if (strcmp(argv[1], "flush") == 0)
    {
        (void)shellWriteString(sh, "log is drained by task_log");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    if (strcmp(argv[1], "status") == 0)
    {
        shellPrint(sh, "log dropped=%lu B tick=%lu ms",
                   (unsigned long)bsp_log_dropped(),
                   (unsigned long)HAL_GetTick());
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    shellPrint(sh, "log test|level <0..5>|tag <tag>|flush|status");
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_usb(int argc, char *argv[])
{
    app_state_t st;
    Shell *sh = shellGetCurrent();
    (void)argc;
    (void)argv;
    app_state_get(&st);
    shellPrint(sh, "usb configured=%u msc_readonly=%u storage=%s",
               st.usb_connected ? 1U : 0U,
               bsp_sd_is_writable() ? 0U : 1U,
               storage_state_name());
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_wdg(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    (void)argc;
    (void)argv;
    shellPrint(sh, "iwdg ~4s heartbeats=0x%02lx", (unsigned long)WDG_BIT_ALL);
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_oled(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();
    const char *op = (argc > 1) ? argv[1] : "status";

    if (strcmp(op, "on") == 0)
    {
        oled_display_on();
    }
    else if (strcmp(op, "off") == 0)
    {
        oled_display_off();
    }
    else if (strcmp(op, "clear") == 0)
    {
        oled_lock();
        oled_clear();
        oled_unlock();
    }
    else if (strcmp(op, "text") == 0)
    {
        char text[64];
        size_t pos = 0U;
        int x;
        int y;

        if (argc < 5)
        {
            shellPrint(sh, "oled text <x> <y> <text>");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        x = atoi(argv[2]);
        y = atoi(argv[3]);
        for (int i = 4; (i < argc) && (pos < (sizeof(text) - 1U)); i++)
        {
            int n = snprintf(text + pos, sizeof(text) - pos,
                             "%s%s", (pos != 0U) ? " " : "", argv[i]);

            if (n > 0)
            {
                pos += (size_t)n;
            }
        }
        text[sizeof(text) - 1U] = 0;
        oled_lock();
        oled_show_string((uint8_t)x, (uint8_t)y, text, OLED_FONT_6X8);
        oled_refresh();
        oled_unlock();
    }
    else if (strcmp(op, "status") != 0)
    {
        shellPrint(sh, "oled on|off|clear|text <x> <y> <text>|status");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    shellPrint(sh, "oled %s", op);
    (void)shellWriteString(sh, s_nl);
    return 0;
}


