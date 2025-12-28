# Minimal Standalone `malloc` (glibc-based)

This directory contains the **minimal, hand-curated standalone build** of glibc’s `malloc`.
It is designed to be **small, explicit, and stable**, and serves as a baseline implementation for understanding and experimenting with glibc’s allocator outside the full glibc build system.

---

## Goals

The minimal build aims to:

* Build glibc’s `malloc` as a **standalone shared object** (`.so`)
* Avoid automation and header-copying scripts
* Keep **full control** over:

  * which headers are used
  * which features are enabled
  * how `morecore` is wired
* Provide a **debug-friendly baseline** before moving to the automated build

This version is intentionally conservative and explicit.

---

## High-Level Architecture

```
Application
   |
   |  (LD_PRELOAD)
   v
exports.c
   |
   v
glibc malloc internals
(malloc.c, arena.c)
   |
   v
morecore.c
```

### Key Ideas

* **Interposition**
  `exports.c` exposes the standard allocation API (`malloc`, `free`, `realloc`, …) and forwards calls into glibc-internal entry points.

* **Allocator Core**
  `malloc.c` and `arena.c` are glibc sources with small, targeted edits to reduce dependencies and keep the build minimal.

* **Explicit `morecore`**
  The allocator’s interaction with the system heap is handled by a custom `morecore.c`, making heap growth/shrink behavior:

  * visible
  * debuggable
  * optionally exportable for testing

* **Manual Compatibility Layer**
  Required glibc internals are provided via a carefully curated set of headers and shims under `include/`, instead of auto-generated stubs.

---

## Building

From the `minimal/` directory:

```bash
make
```

This builds:

```
build/libmalloc_min.so
```
