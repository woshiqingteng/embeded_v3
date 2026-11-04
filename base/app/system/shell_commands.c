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
#include "mw_fatfs.h"
#include "mw_littlefs.h"
#include "mw_log.h"
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
static int cmd_adc(int argc, char *argv[]);
static int cmd_pwm(int argc, char *argv[]);
static int cmd_key(int argc, char *argv[]);
static int cmd_sd(int argc, char *argv[]);
static int cmd_lfs(int argc, char *argv[]);
static int cmd_log(int argc, char *argv[]);
static int cmd_usb(int argc, char *argv[]);
static int cmd_wdg(int argc, char *argv[]);
static int cmd_oled(int argc, char *argv[]);
static int cmd_reboot(int argc, char *argv[]);

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
    CMD(adc, cmd_adc, show adc mv),
    CMD(pwm, cmd_pwm, set/get pwm permille),
    CMD(key, cmd_key, inject key event),
    CMD(sd, cmd_sd, fatfs mount/umount/ls),
    CMD(lfs, cmd_lfs, littlefs mount/umount/format/ls),
    CMD(log, cmd_log, log test),
    CMD(usb, cmd_usb, usb status),
    CMD(wdg, cmd_wdg, watchdog status),
    CMD(oled, cmd_oled, oled status),
    CMD(reboot, cmd_reboot, reboot),
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
    shellPrint(sh, "heap=%lu tasks=%lu pwm=%u key=%lu usb=%u",
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
    if (argc > 1)
    {
        app_key_notify((uint32_t)atoi(argv[1]));
    }
    shellPrint(sh, "key injected 0..3");
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_sd(int argc, char *argv[])
{
    Shell *sh = shellGetCurrent();

    if (argc < 2)
    {
        shellPrint(sh, "sd mount|umount|ls");
        (void)shellWriteString(sh, s_nl);
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
        shellPrint(sh, "lfs mount|umount|format|ls");
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
    else if (strcmp(argv[1], "format") == 0)
    {
        mw_log_shutdown();
        (void)mw_littlefs_umount();
        shellPrint(sh, "lfs format %s", (mw_littlefs_format() == 0) ? "ok" : "fail");
        (void)shellWriteString(sh, s_nl);
        (void)mw_littlefs_mount();
        (void)mw_log_init();
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
    (void)argc;
    (void)argv;
    elog_info("shell", "log test tick=%lu", (unsigned long)HAL_GetTick());
    return 0;
}

static int cmd_usb(int argc, char *argv[])
{
    app_state_t st;
    Shell *sh = shellGetCurrent();
    (void)argc;
    (void)argv;
    app_state_get(&st);
    shellPrint(sh, "usb configured=%u", st.usb_connected ? 1U : 0U);
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
    (void)argc;
    (void)argv;
    oled_lock();
    oled_show_string(0, 0, "SHELL OK", OLED_FONT_6X8);
    oled_refresh();
    oled_unlock();
    shellPrint(sh, "oled ok");
    (void)shellWriteString(sh, s_nl);
    return 0;
}

static int cmd_reboot(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    NVIC_SystemReset();
    return 0;
}

#endif
