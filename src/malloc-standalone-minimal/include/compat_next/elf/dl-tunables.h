#pragma once
#include <stddef.h>
#include <stdint.h>

/* Match arena.c expectations */
typedef struct {
  long        numval;   /* arena.c reads this */
  const char *strval;   /* unused here */
} tunable_val_t;

#ifndef TUNABLES_ENABLED
# define TUNABLES_ENABLED 0
#endif

#ifndef TUNABLE_CALLBACK
# define TUNABLE_CALLBACK(name) name##_callback
#endif

#ifndef TUNABLE_GET
# define TUNABLE_GET(sym, type, cb) \
    do { type sym = (type)0; (void)(sym); (void)(cb); } while (0)
#endif

#ifndef TUNABLE_IS_INITIALIZED
# define TUNABLE_IS_INITIALIZED(x) 1
#endif

#ifndef TUNABLE_BOOL
# define TUNABLE_BOOL(ns, name, var, defval)   /* no-op */
#endif
#ifndef TUNABLE_INT
# define TUNABLE_INT(ns, name, var, defval)    /* no-op */
#endif
#ifndef TUNABLE_SIZE_T
# define TUNABLE_SIZE_T(ns, name, var, defval) /* no-op */
#endif
#ifndef TUNABLE_ALIAS
# define TUNABLE_ALIAS(ns, oldname, newname)   /* no-op */
#endif
