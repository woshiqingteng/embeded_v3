option(GEN_ALL_ARTIFACT "Generate all artifacts (bin/hex/lst)" OFF)

function(mcu_generate target base)
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME "${base}"
        SUFFIX ".elf"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
                $<TARGET_FILE:${target}>
                $<TARGET_FILE_DIR:${target}>/${base}.bin
        COMMENT "Generating ${base}.bin")

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target}>
        COMMENT "Size of ${base}")

    if(GEN_ALL_ARTIFACT)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O ihex
                    $<TARGET_FILE:${target}>
                    $<TARGET_FILE_DIR:${target}>/${base}.hex
            COMMENT "Generating ${base}.hex")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_OBJDUMP} -h -S $<TARGET_FILE:${target}>
                    > $<TARGET_FILE_DIR:${target}>/${base}.lst
            COMMENT "Generating ${base}.lst")
    endif()
endfunction()

function(mcu_flash target base)
    if(NOT OPENOCD)
        find_program(OPENOCD openocd REQUIRED)
    endif()
    if(NOT OPENOCD_INTERFACE OR NOT OPENOCD_TARGET)
        message(FATAL_ERROR "OPENOCD_INTERFACE / OPENOCD_TARGET not set")
    endif()

    set(_addr "${MCU_FLASH_ADDR}")
    if(NOT _addr)
        set(_addr "${MCU_FLASH_BASE}")
    endif()

    if(MCU_BOOT_MODE STREQUAL "flash")
        add_custom_target(${target}_flash
            COMMAND ${OPENOCD}
                    -f interface/${OPENOCD_INTERFACE}
                    -f target/${OPENOCD_TARGET}
                    -c "program $<TARGET_FILE_DIR:${target}>/${base}.bin ${_addr} verify reset exit"
            DEPENDS ${target}
            USES_TERMINAL
            COMMENT "Flashing ${base}.bin to FLASH @ ${_addr}")
    elseif(MCU_BOOT_MODE STREQUAL "ram")
        add_custom_target(${target}_flash
            COMMAND ${OPENOCD}
                    -f interface/${OPENOCD_INTERFACE}
                    -f target/${OPENOCD_TARGET}
                    -c "halt"
                    -c "load_image $<TARGET_FILE_DIR:${target}>/${base}.bin ${_addr} bin"
                    -c "reg pc ${_addr}"
                    -c "resume"
                    -c "shutdown"
            DEPENDS ${target}
            USES_TERMINAL
            COMMENT "Loading ${base}.bin to RAM @ ${_addr}")
    else()
        message(FATAL_ERROR "Invalid MCU_BOOT_MODE: ${MCU_BOOT_MODE}")
    endif()
endfunction()
