add_subdirectory(cherryusb)
add_subdirectory(fatfs)
add_subdirectory(littlefs)
add_subdirectory(easyflash)
add_subdirectory(fs)
add_subdirectory(easylogger)
add_subdirectory(lettershell)

add_library(mw INTERFACE)
target_link_libraries(mw INTERFACE
    mw_cherryusb
    mw_fs
    mw_easylogger
    mw_lettershell
)
