#ifndef MALLOC_FORK_SHLIB_COMPAT_H_SHIM
#define MALLOC_FORK_SHLIB_COMPAT_H_SHIM

/* Make all of glibc's shared-library compatibility helpers no-ops.
   We don't do symbol versioning games in the standalone malloc.  */

#ifndef SHLIB_COMPAT
# define SHLIB_COMPAT(lib, introduced, obsoleted) 0
#endif

#ifndef versioned_symbol
# define versioned_symbol(lib, local, symbol, version)
#endif

#ifndef compat_symbol
# define compat_symbol(lib, local, symbol, version)
#endif

#ifndef default_symbol_version
# define default_symbol_version(local, symbol, version)
#endif

#ifndef strong_alias
# define strong_alias(sym, alias)
#endif

#ifndef weak_alias
# define weak_alias(sym, alias)
#endif

#endif /* MALLOC_FORK_SHLIB_COMPAT_H_SHIM */
