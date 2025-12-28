#pragma once
#include <pthread.h>

/* ---- Type & initializer ------------------------------------------------- */
typedef pthread_mutex_t __libc_lock_define_t;

#ifndef _LIBC_LOCK_INITIALIZER
# define _LIBC_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif

#ifndef __libc_lock_define
/* glibc style: CLASS is typically empty or 'static' */
# define __libc_lock_define(CLASS, NAME) CLASS __libc_lock_define_t NAME
#endif

#ifndef __libc_lock_define_initialized
# define __libc_lock_define_initialized(CLASS, NAME) \
  CLASS __libc_lock_define_t NAME = _LIBC_LOCK_INITIALIZER
#endif

/* ---- Pointer-taking inline impls (centralized) -------------------------- */
static inline void __libc_lock_init_impl(__libc_lock_define_t *l){
  pthread_mutexattr_t a;
  pthread_mutexattr_init(&a);
  pthread_mutexattr_settype(&a, PTHREAD_MUTEX_NORMAL);
  pthread_mutex_init(l, &a);
  pthread_mutexattr_destroy(&a);
}
static inline void __libc_lock_lock_impl   (__libc_lock_define_t *l){ (void)pthread_mutex_lock(l); }
static inline void __libc_lock_unlock_impl (__libc_lock_define_t *l){ (void)pthread_mutex_unlock(l); }
static inline int  __libc_lock_trylock_impl(__libc_lock_define_t *l){ return pthread_mutex_trylock(l); }

/* ---- Object-style macros (glibc passes the object, not pointer) --------- */
#ifdef __libc_lock_init
# undef __libc_lock_init
#endif
#ifdef __libc_lock_lock
# undef __libc_lock_lock
#endif
#ifdef __libc_lock_unlock
# undef __libc_lock_unlock
#endif
#ifdef __libc_lock_trylock
# undef __libc_lock_trylock
#endif

#define __libc_lock_init(l)         __libc_lock_init_impl(&(l))
#define __libc_lock_lock(l)         __libc_lock_lock_impl(&(l))
#define __libc_lock_unlock(l)       __libc_lock_unlock_impl(&(l))
#define __libc_lock_trylock(l)      __libc_lock_trylock_impl(&(l))

/* ---- Optional pointer-form wrappers (some code passes &lock directly) --- */
#ifndef __libc_lock_init_ptr
# define __libc_lock_init_ptr(lp)   __libc_lock_init_impl((lp))
#endif
#ifndef __libc_lock_lock_ptr
# define __libc_lock_lock_ptr(lp)   __libc_lock_lock_impl((lp))
#endif
#ifndef __libc_lock_unlock_ptr
# define __libc_lock_unlock_ptr(lp) __libc_lock_unlock_impl((lp))
#endif
#ifndef __libc_lock_trylock_ptr
# define __libc_lock_trylock_ptr(lp) __libc_lock_trylock_impl((lp))
#endif

/* ---- Recursive aliases (ptmalloc typically treats them the same here) --- */
#ifndef __libc_lock_init_recursive
# define __libc_lock_init_recursive(NAME)   __libc_lock_init(NAME)
#endif
#ifndef __libc_lock_lock_recursive
# define __libc_lock_lock_recursive(NAME)   __libc_lock_lock(NAME)
#endif
#ifndef __libc_lock_unlock_recursive
# define __libc_lock_unlock_recursive(NAME) __libc_lock_unlock(NAME)
#endif
