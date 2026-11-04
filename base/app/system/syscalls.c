/**
 * @file    syscalls.c
 * @brief   newlib syscall stubs: weak _write -> __io_putchar, plus the
 *          remaining stubs required once --specs=nosys.specs is dropped.
 */

#include <sys/stat.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

/* Stack kept free below _estack so the heap cannot collide with the MSP. */
#define SYS_MIN_STACK_SIZE 0x800U

extern uint8_t _end;
extern uint8_t _estack;

extern int __io_putchar(int ch) __attribute__((weak));

__attribute__((weak)) int _write(int file, char *ptr, int len)
{
    (void)file;

    for (int i = 0; i < len; i++)
    {
        if (__io_putchar)
        {
            (void)__io_putchar((int)ptr[i]);
        }
    }

    return len;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

void *_sbrk(ptrdiff_t incr)
{
    static uint8_t *heap_end = NULL;
    uint8_t       *prev;

    if (heap_end == NULL)
    {
        heap_end = &_end;
    }

    if ((uintptr_t)(heap_end + incr) > ((uintptr_t)&_estack - SYS_MIN_STACK_SIZE))
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    prev = heap_end;
    heap_end += (ptrdiff_t)incr;

    return prev;
}

void _exit(int status)
{
    (void)status;

    for (;;)
    {
    }
}
