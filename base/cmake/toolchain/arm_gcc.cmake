set(TOOLCHAIN_PREFIX arm-none-eabi-)

find_program(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++ REQUIRED)
find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc REQUIRED)
find_program(CMAKE_AR           ${TOOLCHAIN_PREFIX}gcc-ar REQUIRED)
find_program(CMAKE_RANLIB       ${TOOLCHAIN_PREFIX}gcc-ranlib REQUIRED)
find_program(CMAKE_NM           ${TOOLCHAIN_PREFIX}gcc-nm REQUIRED)
find_program(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy REQUIRED)
find_program(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size REQUIRED)
find_program(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}objdump REQUIRED)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# share
set(OPT_C_STD       "-std=gnu11")
set(OPT_CXX_STD     "-std=gnu++17")
set(OPT_COMMON  "-Wall -Wextra -Wshadow -Wundef \
                 -fno-common -ffunction-sections -fdata-sections")
set(OPT_DEBUG       "-Og -g3 -DDEBUG")

option(ENABLE_LTO "Enable Link-Time Optimization (release only)" OFF)
set(OPT_RELEASE_C   "-Os -DNDEBUG \
                     -fno-unwind-tables -fno-asynchronous-unwind-tables")
set(OPT_RELEASE_CXX "${OPT_RELEASE_C} -fno-exceptions -fno-rtti -fno-threadsafe-statics")
set(OPT_RELEASE_LINK "-Wl,--no-undefined")
if(ENABLE_LTO)
    set(OPT_RELEASE_C   "${OPT_RELEASE_C}   -flto=auto -ffat-lto-objects")
    set(OPT_RELEASE_CXX "${OPT_RELEASE_CXX} -flto=auto -ffat-lto-objects")
    set(OPT_RELEASE_LINK "${OPT_RELEASE_LINK} -flto=auto")
    message(STATUS "LTO : ENABLED (release)")
else()
    message(STATUS "LTO : disabled")
endif()

# compile
set(CMAKE_C_FLAGS_INIT   "${OPT_C_STD}   ${OPT_COMMON}")
set(CMAKE_CXX_FLAGS_INIT "${OPT_CXX_STD} ${OPT_COMMON}")
set(CMAKE_ASM_FLAGS_INIT "-x assembler-with-cpp")

set(CMAKE_C_FLAGS_DEBUG_INIT   ${OPT_DEBUG})
set(CMAKE_CXX_FLAGS_DEBUG_INIT ${OPT_DEBUG})

set(CMAKE_C_FLAGS_RELEASE_INIT   ${OPT_RELEASE_C})
set(CMAKE_CXX_FLAGS_RELEASE_INIT ${OPT_RELEASE_CXX})

# link
set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nano.specs -Wl,--gc-sections -Wl,--print-memory-usage")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT ${OPT_RELEASE_LINK})
