add_library(mw_fs STATIC
    fs.c
    fs_fatfs.c
    fs_littlefs.c
    fs_logfs.c
)
target_include_directories(mw_fs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mw_fs PRIVATE
    mw_fatfs
    mw_littlefs
    mw_easyflash
    bsp_sd
    bsp_internal_flash
    os_freertos
)
