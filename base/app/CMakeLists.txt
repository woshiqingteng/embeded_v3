add_executable(firmware
    main.c
    app_init.c
    app_state.c
    system/system_init.c
    system/app_ipc.c
    system/app_tick.c
    system/fault_handler.c
    system/freertos_hooks.c
    system/syscalls.c
    system/shell_commands.c
    system/storage_manager.c
    system/usb_bridge.c
    task/task_app_ctrl.c
    task/task_app_adc.c
    task/task_app_oled.c
    task/task_sys_wdg.c
    task/task_sys_log.c
    task/task_sys_shell.c
    task/task_sys_usb.c
)

target_include_directories(firmware PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/system
    ${CMAKE_CURRENT_SOURCE_DIR}/task
)

target_link_libraries(firmware PRIVATE
    platform
    common
    os
    bsp
    mw
)

mcu_generate(firmware firmware)
mcu_flash(firmware firmware)
