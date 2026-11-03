set(TP_SHELL ${THIRD_PARTY}/letter_shell/3.1.2)

add_library(lettershell_core OBJECT
    ${TP_SHELL}/src/shell.c
    ${TP_SHELL}/src/shell_ext.c
)
target_include_directories(lettershell_core PUBLIC ${TP_SHELL}/src)
target_link_libraries(lettershell_core PUBLIC lettershell_config)

add_library(lettershell_port OBJECT
    ${INTEGRATION_BSP}/shell_port.c
)
target_include_directories(lettershell_port PUBLIC
    ${TP_SHELL}/src
    ${INTEGRATION_BSP}
    ${CMAKE_SOURCE_DIR}/mw/lettershell
)
target_link_libraries(lettershell_port PUBLIC
    bsp_uart
    freertos
    lettershell_config
)

add_library(mw_lettershell INTERFACE)
target_sources(mw_lettershell INTERFACE
    $<TARGET_OBJECTS:lettershell_core>
    $<TARGET_OBJECTS:lettershell_port>
)
target_link_libraries(mw_lettershell INTERFACE
    lettershell_core
    lettershell_port
    lettershell_config
)
