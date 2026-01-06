#ifndef MALLOC_FORK_LIBC_INTERNAL_H_SHIM
#define MALLOC_FORK_LIBC_INTERNAL_H_SHIM

#include <stddef.h>

/* In the full glibc build, libc-internal.h pulls in a bunch of
   libc_hidden_proto / profiling / init machinery.

   For the standalone malloc build we only need enough declarations so
   that references compile; we don't actually call these from malloc.c /
   arena.c in any meaningful way. */

#ifndef attribute_hidden
# define attribute_hidden
#endif

/* Profiling hooks (normally provided when glibc is built with
   -finstrument-functions).  We don't provide real definitions here;
   they are only declared so the prototypes in glibc headers compile.  */
extern void __cyg_profile_func_enter (void *this_fn, void *call_site);
extern void __cyg_profile_func_exit  (void *this_fn, void *call_site);

/* glibc's "free resources at exit" helper.  Not used by our
   standalone malloc, but some headers expect it to exist. */
extern void __libc_freeres (void);

/* Miscellaneous libc initialization; again, we only need a
   declaration so that libc-internal.h users compile cleanly.  */
extern void __init_misc (int argc, char **argv, char **envp) attribute_hidden;

#endif /* MALLOC_FORK_LIBC_INTERNAL_H_SHIM */
