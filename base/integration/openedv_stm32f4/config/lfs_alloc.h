#ifndef LFS_ALLOC_H
#define LFS_ALLOC_H

#include <stddef.h>

/* Implemented by the FreeRTOS heap (heap_4.c).  Declared here so the
 * littlefs core does not need to include FreeRTOS headers. */
void *pvPortMalloc(size_t size);
void  vPortFree(void *ptr);

#define LFS_MALLOC(sz) pvPortMalloc(sz)
#define LFS_FREE(p)    vPortFree(p)

#endif /* LFS_ALLOC_H */
