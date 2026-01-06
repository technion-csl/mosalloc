# Automated Standalone malloc (glibc-derived)

## Overview

The `automated/` directory contains a **fully automated pipeline** for extracting, adapting, and building a **standalone `malloc` implementation derived from glibc**, without requiring a full glibc build or dynamic loader environment.

The result is a shared object (`.so`) that:

* Implements glibc’s `malloc` / `free` / `realloc` / `calloc`
* Can be injected via `LD_PRELOAD`
* Preserves glibc allocator behavior as closely as possible
* Exposes selected internal hooks (e.g. `morecore`) for experimentation

This directory represents the **scalable, repeatable version** of the project, designed to survive glibc version changes and incremental allocator research.

---

## Design Goals

1. **Automation-first**

   * No hand-copying of headers or source files
   * No fragile, manually maintained include lists
   * Build adapts automatically to the glibc version in use

2. **Minimal surface area**

   * Only the allocator subsystem is built
   * Loader, pthread, TLS, and versioning machinery are stubbed or shimmed
   * Internal glibc APIs are reduced to the minimum required set

3. **Faithful allocator semantics**

   * The allocator logic remains as close as possible to upstream glibc
   * Changes are isolated to compatibility boundaries
   * Core algorithms (`_int_malloc`, `_int_free`, arenas, bins, tcache) are untouched

4. **Safe experimentation**

   * Internal hooks like `morecore` can be re-exposed
   * Instrumentation and tracing can be enabled without affecting correctness
   * Failures are reproducible and debuggable

---

## High-Level Architecture

```
automated/
├── scripts/        # Automation & dependency discovery
├── src/            # Standalone allocator glue + exports
├── include/        # Compatibility layer & shims
├── build/          # Generated build artifacts (ignored by git)
└── Makefile        # Entry point for the automated build
```

### 1. Automation Layer (`scripts/`)

The automation script is responsible for:

* Driving repeated build attempts
* Parsing compiler errors
* Discovering missing headers, macros, and symbols
* Pulling required glibc headers into a local mirror
* Emitting a stable `CPPFLAGS` configuration once a build succeeds

This allows the allocator to be rebuilt against **different glibc versions** with minimal manual intervention.

---

### 2. Compatibility Layer (`include/`)

Because glibc’s allocator is not designed to be built standalone, this directory provides a **controlled compatibility boundary**:

* `glibc_compat.h`
  Replaces glibc’s internal visibility, versioning, and annotation macros.

* `auto_stubs.h`
  Contains minimal macro and function stubs discovered during compilation.

* `special_shims/`
  Hand-written replacements for deeply internal headers that should *never* be copied verbatim (e.g. loader state, symbol versioning).

* `trace.h`
  Optional tracing hooks used by the exported wrappers.

This layer is where **portability and isolation** are enforced.

---

### 3. Allocator Core & Exports (`src/`)

This directory contains:

* Thin wrappers that expose the allocator as a public API
* Minimal glue code needed to connect glibc internals to `LD_PRELOAD`
* Small, well-defined shims (e.g. `errno` handling)

The actual allocator logic (`malloc.c`, `arena.c`, etc.) is **copied verbatim from glibc** during the automated build and is not permanently stored in the repository.

This ensures:

* No silent drift from upstream glibc
* Clear separation between **upstream code** and **project-owned glue**

---

### 4. Build Model

The automated build:

1. Uses the system compiler (no glibc build system)
2. Builds a shared object (`libmalloc_try.so`)
3. Hides all internal glibc symbols by default
4. Explicitly exports only the allocator entry points
5. Supports optional tracing and debugging flags

The build is repeatable and deterministic once the automation converges.

---

## 5. Build Configuration & Key Variables

The automated build is driven by a **Makefile + Python automation loop**. Most configuration happens through a small set of variables that control *where glibc comes from*, *how the allocator is built*, and *how debuggable the result is*.

### Core Variables

| Variable     | Purpose                                                                                         |
| ------------ | ----------------------------------------------------------------------------------------------- |
| `GLIBC_SRC`  | Path to an existing glibc source tree. If missing, the Makefile can clone glibc automatically.  |
| `GLIBC_REPO` | Upstream glibc Git repository used for cloning when `GLIBC_SRC` does not exist.                 |
| `MAX_ITERS`  | Maximum number of build iterations the automation script is allowed to perform before stopping. |
| `DEBUG`      | Controls optimization vs. debuggability of the allocator build (`DEBUG=1` enables `-O0 -g3`).   |

Example usage:

```bash
make DEBUG=1 GLIBC_SRC=/path/to/glibc
```

This produces a fully debuggable allocator suitable for `gdb`, even with heavy macro expansion and inlined glibc internals.

---

## Compiler & Linker Flags (Why They Exist)

The allocator is built **outside glibc’s build system**, so the Makefile must explicitly recreate a safe subset of glibc’s assumptions.

### Compilation Flags (`CFLAGS`)

* `-fPIC`
  Required for building a shared object that will be injected via `LD_PRELOAD`.

* `-std=gnu11`
  glibc allocator code relies on GNU C extensions.

* `-pthread`
  Required even when thread usage is stubbed, because allocator code references pthread-related constructs.

* `-Wall -Wextra`
  Keeps compatibility issues visible during automation.

* `-Wimplicit-function-declaration`
  Intentionally enabled to surface missing internal declarations early, so they can be shimmed explicitly.

* `-fvisibility=hidden` (for allocator objects)
  Ensures **only explicitly exported symbols** are visible, preventing collisions with the system glibc.

---

### Linker Flags (`LDFLAGS`)

* `-shared`
  Produces a `.so` suitable for `LD_PRELOAD`.

* `-Wl,-z,defs`
  Forces the link to fail if *any symbol is unresolved*.
  This is critical: it prevents silent runtime failures caused by missing internal glibc symbols.

* `-ldl`
  Required for runtime symbol lookup and preload behavior.

---

## Dependency Tracking (`-MMD -MP`)

Although the **automation script** does *not* rely on dependency files, the Makefile keeps them enabled for **manual and debugging workflows**.

### Why dependency files are still useful

After the automation converges, common workflows include:

* Editing a shim header
* Modifying `auto_stubs.h`
* Adjusting compatibility macros
* Debugging allocator behavior incrementally

In these cases, re-running the full automation loop would be unnecessary overhead. Dependency tracking allows:

```bash
make malloc-standalone
```

to correctly rebuild only what changed.

---

### What the flags do

* `-MMD`
  Generates a `.d` file for each object file, listing **non-system headers only**.
  This keeps dependency files portable and small.

* `-MP`
  Adds **phony targets** for each header listed in the `.d` file.

  This prevents the classic failure:

```
make: *** No rule to make target 'include/special_shims/foo.h', needed by build/malloc.o
```

when a header is renamed or removed during experimentation.

#### Concrete example

1. `build/malloc.o` was compiled while including `special_shims/ldsodefs.h`
2. You later remove or rename that shim
3. Without `-MP`: `make` aborts due to a stale dependency
4. With `-MP`: the missing header becomes a harmless phony target, and `make` rebuilds cleanly

This is especially important in this project because **headers are generated, removed, and replaced frequently** during development.

---

## Relationship to the Automation Script

It is important to understand the separation of responsibilities:

* **The Makefile**

  * Knows how to compile and link the allocator
  * Exposes a stable target: `malloc-standalone`
  * Supports incremental rebuilds and debugging

* **The automation script**

  * Drives repeated calls to `make malloc-standalone`
  * Parses compiler errors
  * Copies headers and sources
  * Generates the final `CPPFLAGS`

Dependency files are **ignored by the script**, but remain valuable once the build converges and human iteration begins.

---

## Summary

* The automated build intentionally mirrors glibc behavior while remaining isolated
* Compiler and linker flags enforce correctness, debuggability, and symbol hygiene
* Dependency tracking is kept as an **opt-in debugging aid**, not a core automation mechanism
* The result is a reproducible, inspectable, and extensible standalone allocator build

---

## Intended Use Cases

* Re-exposing allocator internals (e.g. `morecore`)
* Instrumenting glibc malloc without rebuilding glibc
* Studying allocator behavior under real workloads
* Prototyping allocator modifications safely
* Comparing glibc allocator behavior across versions
