add_executable(firmware
    main.c
    system/app_init.c
    system/app_ipc.c
    system/app_state.c
    system/app_tick.c
    system/shell_commands.c
    task/task_ctrl.c
    task/task_adc.c
    task/task_wdg.c
    task/task_log.c
    task/task_oled.c
    task/task_shell.c
    task/task_usb.c
)

target_include_directories(firmware PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/system
    ${CMAKE_CURRENT_SOURCE_DIR}/task
)

target_link_libraries(firmware PRIVATE
    cmsis_device
    common
    freertos
    integration_config
    integration_os

    bsp_core
    bsp_uart
    bsp_log
    bsp_led
    bsp_key
    bsp_pwm
    bsp_adc
    bsp_oled
    bsp_sd
    bsp_nor

    mw_easylogger
    mw_lettershell
    mw_fatfs
    mw_littlefs
    mw_cherryusb
    mw_usb_bridge
    mw_lvgl
)

mcu_generate(firmware firmware)
mcu_flash(firmware firmware)
