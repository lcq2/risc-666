#include <stdint.h>
#include <errno.h>
#include "rv_av_api.h"

static inline long
__syscall_error(long a0)
{
  errno = -a0;
  return -1;
}

static inline long
__internal_syscall(long n, long _a0, long _a1, long _a2, long _a3, long _a4, long _a5)
{
  register long a0 asm("a0") = _a0;
  register long a1 asm("a1") = _a1;
  register long a2 asm("a2") = _a2;
  register long a3 asm("a3") = _a3;
  register long a4 asm("a4") = _a4;
  register long a5 asm("a5") = _a5;

#ifdef __riscv_32e
  register long syscall_id asm("t0") = n;
#else
  register long syscall_id asm("a7") = n;
#endif

  asm volatile ("scall"
		: "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(syscall_id));

  if (a0 < 0)
    return __syscall_error (a0);
  else
    return a0;
}

#define syscall_errno(n, a, b, c, d, e, f) \
        __internal_syscall(n, (long)(a), (long)(b), (long)(c), (long)(d), (long)(e), (long)(f))

int av_init(int width, int height)
{
	return syscall_errno(SYS_av_init, width, height, 0, 0, 0, 0);
}

void av_delay(uint32_t ms)
{
	syscall_errno(SYS_av_delay, ms, 0, 0, 0, 0, 0);
}

int av_update(uint8_t *screen)
{
	return syscall_errno(SYS_av_update, screen, 0, 0, 0, 0, 0);
}

int av_set_palette(uint32_t *palette, int ncolors)
{
	return syscall_errno(SYS_av_set_palette, palette, ncolors, 0, 0, 0, 0);
}

int av_poll_event()
{
  return syscall_errno(SYS_av_poll_event, 0, 0, 0, 0, 0, 0);
}

uint32_t av_get_ticks()
{
	return syscall_errno(SYS_av_get_ticks, 0, 0, 0, 0, 0, 0);
}

void av_shutdown()
{
	syscall_errno(SYS_av_shutdown, 0, 0, 0, 0, 0, 0);
}

