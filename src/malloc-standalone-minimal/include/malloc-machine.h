#pragma once

/* Keep the lock glue if you still use the glibc-style locking macros. */
#include <compat_next/libc-lock.h>

/* Do NOT include the removed sysdeps header anymore. */
/* #include <sysdeps/generic/malloc-machine.h> */

/* Atomics are now provided by include/atomic.h.
   As a safety net, only provide fences if atomics weren't seen yet. */
#ifndef atomic_full_barrier
#  define atomic_full_barrier() __asm__ __volatile__("" ::: "memory")
#endif
#ifndef atomic_read_barrier
#  define atomic_read_barrier()  atomic_full_barrier()
#endif
#ifndef atomic_write_barrier
#  define atomic_write_barrier() atomic_full_barrier()
#endif

/* Nothing else is needed here for your current sources. */
