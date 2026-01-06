/* include/config.h
 *
 * Tiny manual config for the standalone malloc build.
 * This should contain only environment / feature macros that are
 * genuinely about your target system, not glibc internals.
 */

#pragma once
#ifndef MALLOC_STANDALONE_CONFIG_H
#define MALLOC_STANDALONE_CONFIG_H

/* Enable GNU extensions in system headers (needed by glibc malloc code). */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif



/* glibc atomic.h expects this on some configs. Make it derive from GCC. */
#ifndef HAVE_64B_ATOMICS
# define HAVE_64B_ATOMICS (__atomic_always_lock_free(8, 0))
#endif

/* Add real environment facts here if you ever need them, e.g.:
 *
 *   #define MALLOC_STANDALONE_64BIT 1
 *
 * For now we deliberately keep this file minimal. Most glibc-internal
 * config (HAVE_*, USE_*) will be pulled automatically into auto_stubs.h
 * by autoinclude_loop.py, or defaulted conservatively.
 */

#endif /* MALLOC_STANDALONE_CONFIG_H */
