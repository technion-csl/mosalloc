## `automated/include/` — Compatibility Layer & Local Headers

This directory contains the **compatibility headers** that let glibc’s `malloc.c` compile and run as a standalone `LD_PRELOAD` allocator.

In a normal glibc build, many headers/macros come from internal build system wiring. Here we replace only what’s needed, and keep the “real” allocator code (in `src/`) as close to upstream as possible.

---

### Files in this directory

#### `config.h`

Project-wide configuration knobs used during compilation (forced-included via `CPPFLAGS`).

* Used to define/disable features in a consistent way across the build.
* Helps keep build behavior stable across autoinclude runs.

#### `glibc_compat.h`

A small “glue” header that provides **missing glibc internal macros/attributes** in a safe way.

* Examples: likely/unlikely helpers, symbol visibility/alias macros, no-op probes, etc.
* The goal is **syntactic compatibility** without pulling in full glibc internals.

#### `auto_stubs.h`

A “last mile” header for **small, manually maintained stubs** that are easier to provide locally than to import from glibc.

* This is intentionally kept **small and explicit** (despite the name).
* Typical contents:

  * `SINGLE_THREAD_P` policy (standalone decision)
  * `__set_errno` declaration (implemented in `src/errno_shim.c`)
  * tiny fallback helpers for optional subsystems (e.g., tcache init no-op, libio helpers)
  * minimal error reporting stubs (`__libc_message`, `malloc_printerr_tail`)

If something lands here, it’s usually because importing the “real” dependency would drag in too much glibc machinery.

#### `trace.h`

Small tracing interface used by the exported wrappers (`exports.c`).

* Enables lightweight debug logs controlled by an environment variable (e.g. `MALLOC_FORK_TRACE`).
* Kept separate so you can turn tracing on/off without touching allocator internals.

---

## `special_shims/` — Headers We Do *Not* Want to Import

These are **high-risk internal headers** that exist in glibc, but are tightly coupled to the loader (`ld.so`), symbol versioning, or libc build internals. Importing them directly tends to cause:

* circular includes
* missing private symbols
* loader-only dependencies
* ABI/versioning macro explosions

So instead, we provide **minimal replacement shims** with the same header names, containing just enough to compile and behave correctly for a standalone allocator.

### Files in `special_shims/`

#### `ldsodefs.h`

Shim for loader-internal definitions normally provided by `ld.so` / dynamic loader headers.

* glibc’s malloc code sometimes includes headers that *assume* loader state exists.
* In standalone mode, we avoid depending on any loader-private structs/symbols and provide only the minimal definitions/macros that the allocator code expects.

#### `libc-internal.h`

Shim for internal libc-only helpers that are not part of the public ABI.

* Used to satisfy internal macros/attributes/function annotations that appear in malloc sources.
* The shim avoids pulling in unrelated libc internals.

#### `shlib-compat.h`

Shim for symbol versioning / compatibility glue.

* glibc uses this to map symbols across versions (`compat_symbol`, `versioned_symbol`, etc.).
* In a standalone allocator, we do not provide symbol-versioned libc ABI — so this file typically turns those into safe no-ops or minimal placeholders.

#### `random-bits.h`

Shim for internal random / entropy helpers used by malloc hardening paths.

* Standalone allocators don’t want to depend on libc’s internal randomness plumbing.
* The shim provides the minimal API expected by malloc code (either simplified behavior or a safe fallback).

---

### Design intent

* **Keep upstream allocator code upstream-like** (`src/malloc.c` etc.), and concentrate hacks here.
* Prefer **small, readable shims** over importing deep glibc internals.
* If a dependency is “loader-ish” or “versioning-ish”, it belongs in `special_shims/`, not in the auto-copied include tree.