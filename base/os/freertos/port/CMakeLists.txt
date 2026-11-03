set(TP_FREERTOS ${THIRD_PARTY}/freertos/11.3.3)

add_library(freertos_port OBJECT
    ${TP_FREERTOS}/portable/GCC/ARM_CM4F/port.c
    ${TP_FREERTOS}/portable/MemMang/heap_4.c
)

target_include_directories(freertos_port PUBLIC
    ${TP_FREERTOS}/include
    ${TP_FREERTOS}/portable/GCC/ARM_CM4F
)

target_link_libraries(freertos_port PUBLIC cmsis_core freertos_config)
target_compile_options(freertos_port PRIVATE -Wno-unused-parameter -Wno-sign-compare)

add_library(freertos INTERFACE)
target_link_libraries(freertos INTERFACE freertos_core freertos_port freertos_config)

target_sources(freertos INTERFACE $<TARGET_OBJECTS:freertos_core> $<TARGET_OBJECTS:freertos_port>)
