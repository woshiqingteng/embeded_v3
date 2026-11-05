#include <string.h>

#include "easyflash.h"
#include "bsp_internal_flash.h"

static const ef_env s_default_env[] = { { 0, 0, 0 } };

EfErrCode ef_port_init(ef_env const **default_env, size_t *default_env_size)
{
    *default_env = s_default_env;
    *default_env_size = 0U;
    return EF_NO_ERR;
}

EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, size_t size)
{
    return (bsp_internal_flash_read(addr, buf, (uint32_t)size) == 0)
               ? EF_NO_ERR : EF_READ_ERR;
}

EfErrCode ef_port_erase(uint32_t addr, size_t size)
{
    return (bsp_internal_flash_erase_range(addr, (uint32_t)size) == 0)
               ? EF_NO_ERR : EF_ERASE_ERR;
}

EfErrCode ef_port_write(uint32_t addr, const uint32_t *buf, size_t size)
{
    return (bsp_internal_flash_prog(addr, buf, (uint32_t)size) == 0)
               ? EF_NO_ERR : EF_WRITE_ERR;
}

void ef_port_env_lock(void)
{
}

void ef_port_env_unlock(void)
{
}

void ef_log_debug(const char *file, const long line, const char *format, ...)
{
    (void)file;
    (void)line;
    (void)format;
}

void ef_log_info(const char *format, ...)
{
    (void)format;
}

void ef_print(const char *format, ...)
{
    (void)format;
}
