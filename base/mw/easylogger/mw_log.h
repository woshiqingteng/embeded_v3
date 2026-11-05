#ifndef MW_LOG_H
#define MW_LOG_H

#include <stdint.h>

int  mw_log_init(void);
void mw_log_start(void);
int  mw_log_fill(uint32_t bytes);
void mw_log_set_progress_cb(void (*cb)(void));

#endif /* MW_LOG_H */
