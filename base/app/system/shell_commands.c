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
#include "bsp_log.h"
#include "nor.h"
#include "fault_info.h"
#include "mw_fatfs.h"
#include "mw_littlefs.h"
#include "elog.h"

#if SHELL_USING_CMD_EXPORT != 1

extern int shellSetVar(char *name, int value);
extern void shellUp(Shell *shell);
extern void shellDown(Shell *shell);
extern void shellRight(Shell *shell);
extern void shellLeft(Shell *shell);
extern void shellTab(Shell *shell);
extern void shellBackspace(Shell *shell);
extern void shellDelete(Shell *shell);
extern void shellEnter(Shell *shell);
extern void shellHelp(int argc, char *argv[]);
extern void shellUsers(void);
extern void shellCmds(void);
extern void shellVars(void);
extern void shellKeys(void);
extern void shellClear(void);

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
static int cmd_lfs(int argc, char *argv[]);
static int cmd_log(int argc, char *argv[]);
static int cmd_usb(int argc, char *argv[]);
static int cmd_wdg(int argc, char *argv[]);
static int cmd_oled(int argc, char *argv[]);

SHELL_AGENCY_FUNC(shellRun, shellGetCurrent(), (const char *)p1);

#define CMD(_name, _func, _desc) \
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN, \
                   _name, _func, _desc)

const ShellCommand shellCommandList[] =
{
    {.attr.value=SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_USER),
     .data.user.name = SHELL_DEFAULT_USER,
     .data.user.password = SHELL_DEFAULT_USER_PASSWORD,
     .data.user.desc = "default user"},

    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
                   setVar, shellSetVar, set var),
    SHELL_KEY_ITEM(SHELL_CMD_PERMISSION(0), 0x1B5B4100, shellUp, up),
    SHELL_KEY_ITEM(SHELL_CMD_PERMISSION(0), 0x1B5B4200, shellDown, down),
    SHELL_KEY_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
                   0x1B5B4300, shellRight, right),
    SHELL_KEY_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
                   0x1B5B4400, shellLeft, left),
    SHELL_KEY_ITEM(SHELL_CMD_PERMISSION(0), 0x09000000, shellTab, tab),
    SHELL_KEY_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
                   0x08000000, shellBackspace, backspace),
    SHELL_KEY_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
                   0x7F000000, shellDelete, delete),
    SHELL_KEY_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
                   0x0A000000, shellEnter, enter),
    SHELL_KEY_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
                   0x0D000000, shellEnter, enter),

    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
                   help, shellHelp, show command info),
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
                   users, shellUsers, list all user),
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
                   cmds, shellCmds, list all cmd),
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
                   vars, shellVars, list all var),
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
                   keys, shellKeys, list all key),
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
                   clear, shellClear, clear console),
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
                   sh, SHELL_AGENCY_FUNC_NAME(shellRun), run command directly),

    CMD(stats, cmd_stats, show run stats),
    CMD(uptime, cmd_uptime, show uptime),
    CMD(heap, cmd_heap, show heap usage),
    CMD(tasks, cmd_tasks, list tasks),
    CMD(clock, cmd_clock, show clocks),
    CMD(fault, cmd_fault, show last fault),
    CMD(led, cmd_led, led on/off/toggle/status),
    CMD(adc, cmd_adc, show adc mv),
    CMD(uart, cmd_uart, uart status),
    CMD(pwm, cmd_pwm, set/get pwm permille),
    CMD(key, cmd_key, key status/inject),
    CMD(nor, cmd_nor, nor id/info/read),
    CMD(sd, cmd_sd, fatfs mount/umount/ls/info/cat/dma),
    CMD(lfs, cmd_lfs, littlefs mount/umount/ls/info/df/cat),
    CMD(log, cmd_log, log test/level/tag/flush/status),
    CMD(usb, cmd_usb, usb status/readonly),
    CMD(wdg, cmd_wdg, watchdog status),
    CMD(oled, cmd_oled, oled on/off/clear/text/status),
};

const unsigned short shellCommandCount =
    sizeof(shellCommandList) / sizeof(ShellCommand);

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

    shellPrint(sh, "cfsr=%08lx hfsr=%08lx pc=%08lx lr=%08lx",
               (unsigned long)g_fault_info.cfsr,
               (unsigned long)g_fault_info.hfsr,
               (unsigned long)g_fault_info.pc,
               (unsigned long)g_fault_info.lr);
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

    nor_read(buf, addr, (uint16_t)len);
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

static int cmd_sd(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();

    if (argc < 2)
    {
        shellPrint(sh, "sd mount|umount|ls|info|cat <file>|dma on/off/status");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    if (strcmp(argv[1], "dma") == 0)
    {
        const char *op = (argc > 2) ? argv[2] : "status";

        if (strcmp(op, "on") == 0)
        {
            if (bsp_sd_set_dma_enabled(true) == 0)
            {
                shellPrint(sh, "sd dma on");
            }
            else
            {
                shellPrint(sh, "sd dma failed; blocking mode kept");
            }
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
        (void)shellWriteString(sh, s_nl);
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
            shellPrint(sh, "sd size=%lu MiB blocks=%lu block_size=%lu B readonly=%u owner=%u dma=%u",
                       (unsigned long)info.total_size_mb,
                       (unsigned long)info.block_count,
                       (unsigned long)info.block_size,
                       bsp_sd_is_writable() ? 0U : 1U,
                       (unsigned)bsp_sd_owner(),
                       bsp_sd_dma_enabled() ? 1U : 0U);
        }
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    if (strcmp(argv[1], "cat") == 0)
    {
        FIL file;
        UINT br;
        char buf[129];

        if (argc < 3)
        {
            shellPrint(sh, "sd cat <file>");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        if (mw_fatfs_mount() != 0)
        {
            shellPrint(sh, "sd not mounted");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        if (mw_fatfs_lock(1000U) != 0)
        {
            shellPrint(sh, "sd busy");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        if (f_open(&file, argv[2], FA_READ) == FR_OK)
        {
            while ((f_read(&file, buf, sizeof(buf) - 1U, &br) == FR_OK) && (br > 0U))
            {
                buf[br] = '\0';
                (void)shellWriteString(sh, buf);
            }
            (void)f_close(&file);
        }
        else
        {
            shellPrint(sh, "sd open failed");
            (void)shellWriteString(sh, s_nl);
        }
        mw_fatfs_unlock();
        return 0;
    }

    if (strcmp(argv[1], "mount") == 0)
    {
        shellPrint(sh, "sd mount %s", (mw_fatfs_mount() == 0) ? "ok" : "fail");
        (void)shellWriteString(sh, s_nl);
    }
    else if (strcmp(argv[1], "umount") == 0)
    {
        shellPrint(sh, "sd umount %s", (mw_fatfs_umount() == 0) ? "ok" : "fail");
        (void)shellWriteString(sh, s_nl);
    }
    else if (strcmp(argv[1], "ls") == 0)
    {
        DIR dir;
        FILINFO fno;

        if (mw_fatfs_mount() != 0)
        {
            shellPrint(sh, "sd not mounted");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        if (mw_fatfs_lock(1000U) != 0)
        {
            shellPrint(sh, "sd busy");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        if (f_opendir(&dir, "") == FR_OK)
        {
            while ((f_readdir(&dir, &fno) == FR_OK) && (fno.fname[0] != 0))
            {
                (void)shellWriteString(sh, fno.fname);
                (void)shellWriteString(sh, s_nl);
            }
            (void)f_closedir(&dir);
        }
        mw_fatfs_unlock();
    }
    return 0;
}

static int cmd_lfs(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();

    if (argc < 2)
    {
        shellPrint(sh, "lfs mount|umount|ls|info|df|cat <file>");
        (void)shellWriteString(sh, s_nl);
        return 0;
    }

    if (strcmp(argv[1], "mount") == 0)
    {
        shellPrint(sh, "lfs mount %s", (mw_littlefs_mount() == 0) ? "ok" : "fail");
        (void)shellWriteString(sh, s_nl);
    }
    else if (strcmp(argv[1], "umount") == 0)
    {
        shellPrint(sh, "lfs umount %s", (mw_littlefs_umount() == 0) ? "ok" : "fail");
        (void)shellWriteString(sh, s_nl);
    }
    else if ((strcmp(argv[1], "info") == 0) || (strcmp(argv[1], "df") == 0))
    {
        lfs_t *lfs;
        lfs_ssize_t used;

        if (mw_littlefs_mount() != 0)
        {
            shellPrint(sh, "lfs not mounted");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        if (mw_littlefs_lock(1000U) != 0)
        {
            shellPrint(sh, "lfs busy");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        lfs = mw_littlefs_handle();
        used = lfs_fs_size(lfs);
        shellPrint(sh, "lfs mounted=1 used_blocks=%ld total_blocks=%lu",
                   (long)used,
                   (unsigned long)lfs->cfg->block_count);
        (void)shellWriteString(sh, s_nl);
        mw_littlefs_unlock();
    }
    else if (strcmp(argv[1], "cat") == 0)
    {
        lfs_t *lfs;
        lfs_file_t file;
        char buf[129];

        if (argc < 3)
        {
            shellPrint(sh, "lfs cat <file>");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        if (mw_littlefs_mount() != 0)
        {
            shellPrint(sh, "lfs not mounted");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        if (mw_littlefs_lock(1000U) != 0)
        {
            shellPrint(sh, "lfs busy");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        lfs = mw_littlefs_handle();
        if (lfs_file_open(lfs, &file, argv[2], LFS_O_RDONLY) == LFS_ERR_OK)
        {
            lfs_ssize_t n;
            while ((n = lfs_file_read(lfs, &file, buf, sizeof(buf) - 1U)) > 0)
            {
                buf[n] = 0;
                (void)shellWriteString(sh, buf);
            }
            (void)lfs_file_close(lfs, &file);
        }
        else
        {
            shellPrint(sh, "lfs open failed");
            (void)shellWriteString(sh, s_nl);
        }
        mw_littlefs_unlock();
    }
    else if (strcmp(argv[1], "ls") == 0)
    {
        lfs_t *lfs;
        lfs_dir_t dir;
        struct lfs_info info;

        if (mw_littlefs_mount() != 0)
        {
            shellPrint(sh, "lfs not mounted");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        if (mw_littlefs_lock(1000U) != 0)
        {
            shellPrint(sh, "lfs busy");
            (void)shellWriteString(sh, s_nl);
            return 0;
        }
        lfs = mw_littlefs_handle();
        if (lfs_dir_open(lfs, &dir, "/") == LFS_ERR_OK)
        {
            while (lfs_dir_read(lfs, &dir, &info) > 0)
            {
                (void)shellWriteString(sh, info.name);
                (void)shellWriteString(sh, s_nl);
            }
            (void)lfs_dir_close(lfs, &dir);
        }
        mw_littlefs_unlock();
    }
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
    shellPrint(sh, "usb configured=%u msc_readonly=%u",
               st.usb_connected ? 1U : 0U,
               bsp_sd_is_writable() ? 0U : 1U);
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


#endif
