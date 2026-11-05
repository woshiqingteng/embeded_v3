#include "lfs.h"
#include "lfs_config.h"
#include "nor.h"

static uint8_t g_read_buf[LFS_CACHE_SIZE];
static uint8_t g_prog_buf[LFS_CACHE_SIZE];
static uint8_t g_lookahead_buf[LFS_LOOKAHEAD_SIZE];

static int lfs_bd_read(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint32_t addr = LFS_NOR_BASE_ADDR + (uint32_t)block * LFS_BLOCK_SIZE + (uint32_t)off;
    (void)c;
    if ((uint32_t)off + (uint32_t)size > LFS_BLOCK_SIZE)
    {
        return LFS_ERR_INVAL;
    }
    return (nor_read((uint8_t *)buffer, addr, (uint16_t)size) == 0)
               ? LFS_ERR_OK : LFS_ERR_IO;
}

static int lfs_bd_prog(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, const void *buffer, lfs_size_t size)
{
    uint32_t addr = LFS_NOR_BASE_ADDR + (uint32_t)block * LFS_BLOCK_SIZE + (uint32_t)off;
    (void)c;
    if ((uint32_t)off + (uint32_t)size > LFS_BLOCK_SIZE)
    {
        return LFS_ERR_INVAL;
    }
    return (nor_write_erased((uint8_t *)buffer, addr, (uint16_t)size) == 0)
               ? LFS_ERR_OK : LFS_ERR_IO;
}

static int lfs_bd_erase(const struct lfs_config *c, lfs_block_t block)
{
    uint32_t addr = LFS_NOR_BASE_ADDR + (uint32_t)block * LFS_BLOCK_SIZE;
    (void)c;
    if (block >= LFS_BLOCK_COUNT)
    {
        return LFS_ERR_INVAL;
    }
    return (nor_erase_sector(addr / NOR_SECTOR_SIZE_BYTE) == 0)
               ? LFS_ERR_OK : LFS_ERR_IO;
}

static int lfs_bd_sync(const struct lfs_config *c)
{
    (void)c;
    return LFS_ERR_OK;
}

static const struct lfs_config g_cfg =
{
    .context         = 0,
    .read            = lfs_bd_read,
    .prog            = lfs_bd_prog,
    .erase           = lfs_bd_erase,
    .sync            = lfs_bd_sync,

    .read_size       = LFS_READ_SIZE,
    .prog_size       = LFS_PROG_SIZE,
    .block_size      = LFS_BLOCK_SIZE,
    .block_count     = LFS_BLOCK_COUNT,
    .block_cycles    = LFS_BLOCK_CYCLES,
    .cache_size      = LFS_CACHE_SIZE,
    .lookahead_size  = LFS_LOOKAHEAD_SIZE,
    .compact_thresh  = 0,
    .read_buffer     = g_read_buf,
    .prog_buffer     = g_prog_buf,
    .lookahead_buffer = g_lookahead_buf,
    .name_max        = 0,
    .file_max        = 0,
    .attr_max        = 0,
    .metadata_max    = 0,
};

const struct lfs_config *lfs_port_config(void)
{
    return &g_cfg;
}

