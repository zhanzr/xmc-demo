/**
 * @file    syscalls.c
 * @brief   Minimal newlib retarget layer.
 *
 * Only the pieces the demo apps need are implemented: stdout/stderr go to the
 * USIC1 channel 0 console. There is no filesystem and no heap by default, so the
 * remaining stubs just fail cleanly instead of pulling in errno machinery.
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include "console.h"

/* Heap. Off by default: failing every allocation is safer than guessing a _sbrk.
 * Projects that need malloc -- newlib's stdio allocates a buffer per stream on
 * first use, and Dhrystone allocates its work records -- set XMC_HEAP_SIZE (see
 * cmake/xmc1100_board.cmake) to serve _sbrk from a static arena. A bump allocator
 * is enough: these runs never free.
 */
#ifndef XMC_HEAP_SIZE
#define XMC_HEAP_SIZE 0
#endif

/*
 * -nostartfiles removes crt0, so newlib's __libc_init_array loses _init/_fini.
 * There is nothing to run before/after static constructors in C.
 */
void _init(void)
{
}

void _fini(void)
{
}

#if XMC_HEAP_SIZE > 0
static uint8_t heap_arena[XMC_HEAP_SIZE] __attribute__((aligned(8)));
static size_t heap_used;
#endif

void *_sbrk(ptrdiff_t incr)
{
#if XMC_HEAP_SIZE > 0
    const size_t prev = heap_used;

    if ((incr < 0) || ((size_t)incr > (XMC_HEAP_SIZE - prev)))
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_used = prev + (size_t)incr;
    return &heap_arena[prev];
#else
    (void)incr;
    errno = ENOMEM;
    return (void *)-1;
#endif
}

#define CONSOLE_OUT_FD (1)
#define CONSOLE_ERR_FD (2)

int _write(int file, const char *data, int len)
{
    if ((file != CONSOLE_OUT_FD) && (file != CONSOLE_ERR_FD))
    {
        errno = EBADF;
        return -1;
    }

    if (len > 0)
    {
        console_write(data, (size_t)len);
    }

    return len;
}

int _read(int file, char *data, int len)
{
    if (file != STDIN_FILENO)
    {
        errno = EBADF;
        return -1;
    }

    if (len <= 0)
    {
        return 0;
    }

    /* Line oriented, non-blocking after the 1 s idle timeout. No local echo. */
    return (int)console_readline(data, (size_t)len, 1000U);
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _lseek(int file, int offset, int whence)
{
    (void)file;
    (void)offset;
    (void)whence;
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

void _exit(int status)
{
    (void)status;
    for (;;)
    {
        __asm volatile("wfi");
    }
}
