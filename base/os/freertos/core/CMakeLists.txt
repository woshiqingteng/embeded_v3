set(TP_FREERTOS ${THIRD_PARTY}/freertos/11.3.3)

add_library(freertos_core OBJECT
    ${TP_FREERTOS}/tasks.c
    ${TP_FREERTOS}/queue.c
    ${TP_FREERTOS}/list.c
    ${TP_FREERTOS}/timers.c
    ${TP_FREERTOS}/event_groups.c
    ${TP_FREERTOS}/stream_buffer.c
)

target_include_directories(freertos_core PUBLIC
    ${TP_FREERTOS}/include
    ${TP_FREERTOS}/portable/GCC/ARM_CM4F
)

target_link_libraries(freertos_core PUBLIC ${MCU_ARCH_INTERFACE} freertos_config)
target_compile_options(freertos_core PRIVATE -Wno-unused-parameter -Wno-sign-compare)
