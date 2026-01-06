#ifndef MALLOC_STANDALONE_DL_TUNABLES_H
#define MALLOC_STANDALONE_DL_TUNABLES_H

#include <stdint.h>

/*
 * Stand-ins for the tunables machinery. malloc/arena touches these
 * but standalone builds don't need real tunables.
 */
typedef intmax_t tunable_num_t;

typedef struct
{
  tunable_num_t numval;
} tunable_val_t;

typedef void (*tunable_callback_t) (tunable_val_t *);

#ifndef TUNABLE_CALLBACK_FNDECL
# define TUNABLE_CALLBACK_FNDECL(__name, __type) \
    static int __name (tunable_val_t *valp)
#endif

#ifndef TUNABLE_CALLBACK
# define TUNABLE_CALLBACK(__name) __name
#endif

#ifndef TUNABLE_GET
# define TUNABLE_GET(__name, __type, __callback) \
    do { (void) (__callback); } while (0)
#endif

#ifndef TUNABLE_IS_INITIALIZED
# define TUNABLE_IS_INITIALIZED(__name) 0
#endif

static inline void __tunables_init (void) { }

static inline void
__libc_tunable_set_val (tunable_val_t *val, tunable_num_t newval)
{
  if (val)
    val->numval = newval;
}

#endif /* MALLOC_STANDALONE_DL_TUNABLES_H */
