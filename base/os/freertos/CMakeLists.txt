# FreeRTOS is exposed upward as a single target. The kernel sources and the
# Cortex-M4 port are intentionally kept together here, mirroring the mw targets;
# no separate core/port targets are exposed to upper layers.
add_library(os_freertos STATIC
    ${TP_FREERTOS}/tasks.c
    ${TP_FREERTOS}/queue.c
    ${TP_FREERTOS}/list.c
    ${TP_FREERTOS}/event_groups.c
    ${TP_FREERTOS}/stream_buffer.c
    ${TP_FREERTOS}/portable/GCC/ARM_CM4F/port.c
    ${TP_FREERTOS}/portable/MemMang/heap_4.c
)

target_include_directories(os_freertos PUBLIC
    ${TP_FREERTOS}/include
    ${TP_FREERTOS}/portable/GCC/ARM_CM4F
    ${BOARD_FREERTOS_CONFIG_DIR}
)

target_link_libraries(os_freertos PUBLIC cmsis_device ${MCU_ARCH_INTERFACE})
target_compile_options(os_freertos PRIVATE -Wno-unused-parameter -Wno-sign-compare)
