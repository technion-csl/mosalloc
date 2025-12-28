/* include/glibc_compat.h */
#ifndef GLIBC_COMPAT_H
#define GLIBC_COMPAT_H

/* --- attributes/visibility used in glibc headers --- */
#ifndef attribute_hidden
# define attribute_hidden /* nothing */
#endif

#ifndef internal_function
# define internal_function /* nothing */
#endif

#ifndef __attribute_noinline__
# define __attribute_noinline__ /* nothing */
#endif

#ifndef __always_inline
# define __always_inline inline
#endif

#ifndef __thread
# define __thread /* nothing */
#endif

/* glibc uses IS_IN(libc), IS_IN(rtld), etc. in many internal headers */
#ifndef IS_IN
# define IS_IN(x) 0
#endif

/* “libc_hidden_*” family are glibc’s symbol versioning helpers — no-ops here */
#ifndef libc_hidden_proto
# define libc_hidden_proto(sym, ...)
#endif
#ifndef libc_hidden_def
# define libc_hidden_def(sym)
#endif
#ifndef libc_hidden_weak
# define libc_hidden_weak(sym)
#endif
#ifndef libc_hidden_data_def
# define libc_hidden_data_def(sym)
#endif
#ifndef libc_hidden_ver
# define libc_hidden_ver(local, public)
#endif

/* Likewise for rtld_hidden_* */
#ifndef rtld_hidden_def
# define rtld_hidden_def(sym)
#endif

#endif /* GLIBC_COMPAT_H */
