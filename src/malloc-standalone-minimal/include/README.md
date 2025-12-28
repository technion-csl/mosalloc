# `minimal/include/` — Header Layout & Compatibility Model

This directory contains **all headers required to build a minimal standalone version of glibc’s `malloc`**, without using the full glibc build system.

Unlike the automated build, headers here are **manually curated** and divided into clear categories based on their origin and role.

---

## 1. Verbatim glibc Headers (Unmodified)

These files are copied **exactly** from the glibc source tree and are **not shims**.

They define core allocator data structures, constants, and low-level helpers that the allocator relies on.

### Atomic & Low-Level Helpers

* `atomic-machine.h`
* `atomic.h`

### Allocation Semantics & Layout

* `calloc-clear-memory.h`
* `malloc-alignment.h`
* `malloc-size.h`
* `malloc-machine.h`
* `malloc-hugepages.h`
* `malloc-sysdep.h`

### Internal Allocator Interfaces

* `malloc/malloc-internal.h`
* `_itoa.h`

These headers are included by `malloc.c` / `arena.c` and must remain **bit-for-bit compatible** with upstream glibc.

---

## 2. glibc Source Mirror Headers

These headers come from glibc’s `include/` directory and are mirrored locally to satisfy internal includes.

* `glibc-src/include/malloc.h`

They exist to avoid pulling in the full glibc include hierarchy while preserving expected include paths.

---

## 3. Combined Compatibility Shim: `glibc_shim.h`

`glibc_shim.h` is a **centralized compatibility header** that replaces several deeply internal glibc headers with minimal, safe stand-ins.

It consolidates shims for:

* `libc-internal.h`
* `libc-pointer-arith.h`
* `not-cancel.h`
* `random-bits.h`
* `libc-mtag.h`
* `setvmaname.h`
* `shlib-compat.h`

Instead of scattering stubs across many files, this header provides a **single, explicit compatibility boundary** between the allocator and the rest of glibc.

---

## 4. Manual Compatibility Headers (`compat_next/`)

The `compat_next/` directory contains **hand-written shims** for headers that are:

* Too loader-dependent
* Too pthread-dependent
* Too entangled with glibc versioning

These headers are intentionally **minimal** and only define what the allocator actually uses.

### Examples

* `compat_next/libc-lock.h`
* `compat_next/bits/libc-lock.h`
* `compat_next/elf/dl-tunables.h`

They allow the allocator to compile without:

* the glibc dynamic loader
* tunables infrastructure
* internal locking frameworks

---

## 5. Project-Specific Headers

These headers are **not part of glibc**, but support the standalone build.

* `config.h`
  Build-time configuration and feature toggles.

* `glibc_compat.h`
  Defines visibility macros, alias helpers, and attribute stubs expected by glibc code.

* `trace.h`
  Optional tracing hooks used by exported allocator wrappers.

---

## Design Philosophy

The minimal build follows these rules:

* **Allocator logic is untouched** (copied directly from glibc)
* **Compatibility is explicit**, not implicit
* **Shims are isolated and documented**
* **No dynamic loader assumptions**
* **No full libc dependency**