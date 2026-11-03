#ifndef LFS_CONFIG_H
#define LFS_CONFIG_H

/* LittleFS volume lives in the first 4 MiB of the 32 MiB W25Q256. */
#define LFS_NOR_BASE_ADDR      0x00000000U
#define LFS_NOR_VOLUME_SIZE    (4U * 1024U * 1024U)

#define LFS_READ_SIZE          256U
#define LFS_PROG_SIZE          256U
#define LFS_BLOCK_SIZE         4096U
#define LFS_BLOCK_COUNT        (LFS_NOR_VOLUME_SIZE / LFS_BLOCK_SIZE)
#define LFS_CACHE_SIZE         256U
#define LFS_LOOKAHEAD_SIZE     16U
#define LFS_BLOCK_CYCLES       500

#define LFS_LOG_PATH           "/log/app.log"

#endif /* LFS_CONFIG_H */
