#!/usr/bin/env python3
"""
autoinclude.py

Repeatedly builds the project and fixes missing include errors by:
- Parsing compiler errors for missing headers.
- Copying required glibc headers into include/glibc-src/, preserving the original
  path relative to the glibc source tree.
- Creating optional “flat” header aliases under include/glibc-src/include/ for
  includes like #include <foo.h>.
- Treating some glibc-internal headers as “blocked” and requiring manual shims
  under include/special_shims/.

Important design choice:
- include/auto_stubs.h is manually maintained; we do NOT auto-append random
  #defines anymore.
"""

import os
import re
import sys
import shutil
import subprocess
import platform
import argparse
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

SCRIPT_DIR   = Path(__file__).resolve().parent
PROJECT_ROOT = (SCRIPT_DIR / "..").resolve()

BUILD_DIR       = PROJECT_ROOT / "build"
ERRFILE         = BUILD_DIR / "compile.err"
CPPFLAGS_FILE   = BUILD_DIR / "CPPFLAGS"

LOCAL_INC       = PROJECT_ROOT / "include"
MIRROR_ROOT     = LOCAL_INC / "glibc-src"
SPECIAL_SHIMS_DIR = LOCAL_INC / "special_shims"
STUBS_FILE      = LOCAL_INC / "auto_stubs.h"
GLIBC_COMPAT    = LOCAL_INC / "glibc_compat.h"

SRC_DIR         = PROJECT_ROOT / "src"

GLIBC_SRC: Path | None = None
MAX_ITERS = int(os.environ.get("MAX_ITERS", "50"))

# ---------------------------------------------------------------------------
# Arch detection
# ---------------------------------------------------------------------------

def _detect_arch() -> str:
    m = platform.machine()
    if not m:
        return "x86_64"
    m = m.lower()
    if m in ("x86_64", "amd64"):
        return "x86_64"
    if m in ("i386", "i486", "i586", "i686"):
        return "i386"
    if m in ("aarch64", "arm64"):
        return "aarch64"
    return m

ARCH = os.environ.get("GLIBC_ARCH", _detect_arch())

# ---------------------------------------------------------------------------
# Policy knobs
# ---------------------------------------------------------------------------

# Headers that should NOT be mirrored from glibc; they must be provided by a
# manual shim under include/special_shims/ (same relative path).
BLOCKED_HEADERS = {
    "ldsodefs.h",
    "shlib-compat.h",
    "libc-symbols.h",
    "glibc-symbols.h",
    "libc-internal.h",
    "random-bits.h",
    "dl-tunables.h",          # can appear as dl-tunables.h or elf/dl-tunables.h
    "libc-lock.h",            # can appear as libc-lock.h or bits/libc-lock.h
    "setvmaname.h",
}

# You can add full relative paths here too if your build reports them that way.
# (The script uses basename matching for “blocked” decisions.)
# ---------------------------------------------------------------------------

def log(msg: str):
    print(f"[autoinclude] {msg}", flush=True)

def sh(cmd, cwd=None, capture=False, env=None):
    if capture:
        return subprocess.run(
            cmd,
            cwd=cwd,
            env=env,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    subprocess.run(cmd, cwd=cwd, env=env, check=True)

def ensure_dirs():
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    LOCAL_INC.mkdir(parents=True, exist_ok=True)
    MIRROR_ROOT.mkdir(parents=True, exist_ok=True)
    SPECIAL_SHIMS_DIR.mkdir(parents=True, exist_ok=True)
    SRC_DIR.mkdir(parents=True, exist_ok=True)

    if not STUBS_FILE.exists():
        STUBS_FILE.write_text(
            "#ifndef AUTO_STUBS_H\n#define AUTO_STUBS_H\n"
            "/* Manually maintained small shims for standalone malloc. */\n"
            "#endif\n"
        )

def ensure_required_repo_headers():
    # Keep this minimal: the repo owns these files; script should not “recreate” them.
    if not GLIBC_COMPAT.exists():
        log(f"WARNING: {GLIBC_COMPAT} is missing. (Expected to be tracked in repo.)")
    if not (LOCAL_INC / "config.h").exists():
        log("WARNING: include/config.h is missing. (Expected to be tracked in repo.)")

# ---------------------------------------------------------------------------
# Copying selected glibc sources into src/
# ---------------------------------------------------------------------------

def ensure_glibc_sources():
    """
    Copy selected glibc source files into src/ as 1:1 imports.
    Your Makefile decides which of these are compiled as separate units.

    Note: malloc.c may #include "arena.c" / "morecore.c" in some snapshots.
    Keep Makefile consistent with your chosen model.
    """
    if GLIBC_SRC is None:
        return

    glibc_malloc_dir = GLIBC_SRC / "malloc"
    needed = ["arena.c", "malloc.c", "morecore.c"]

    for name in needed:
        src_file = glibc_malloc_dir / name
        dst_file = SRC_DIR / name

        if not src_file.exists():
            log(f"[sources] WARNING: expected {src_file} not found in glibc tree")
            continue

        dst_file.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src_file, dst_file)
        log(f"[sources] copied {src_file} -> {dst_file}")

    # malloc-hugepages.c lives elsewhere in glibc
    hugepages_src = GLIBC_SRC / "sysdeps" / "generic" / "malloc-hugepages.c"
    hugepages_dst = SRC_DIR / "malloc-hugepages.c"
    if hugepages_src.exists():
        hugepages_dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(hugepages_src, hugepages_dst)
        log(f"[sources] copied {hugepages_src} -> {hugepages_dst}")
    else:
        log(f"[sources] WARNING: expected {hugepages_src} not found in glibc tree")

# ---------------------------------------------------------------------------
# Dynamic CPPFLAGS
# ---------------------------------------------------------------------------

def collect_include_dirs_under(base: Path) -> list[Path]:
    dirs: set[Path] = set()
    if base.exists():
        for root, _, _ in os.walk(base):
            dirs.add(Path(root))
    return sorted(dirs, key=lambda p: (len(p.parts), str(p)))

def compute_cppflags():
    """
    Build CPPFLAGS dynamically.

    CRITICAL ORDERING:
      -Iinclude/special_shims MUST come BEFORE include/glibc-src so manual shims
      always win even if a mirrored header with the same basename exists.
    """
    parts: list[str] = []

    # Force include “globals”
    cfg    = LOCAL_INC / "config.h"
    stubs  = LOCAL_INC / "auto_stubs.h"
    compat = LOCAL_INC / "glibc_compat.h"

    if cfg.exists():
        parts.append("-include include/config.h")
    if stubs.exists():
        parts.append("-include include/auto_stubs.h")
    if compat.exists():
        parts.append("-include include/glibc_compat.h")

    # ---- (1) Explicit include ordering ----
    # 1) special shims first
    if SPECIAL_SHIMS_DIR.exists():
        parts.append("-Iinclude/special_shims")

    # 2) then the main include dir (for trace.h, config, etc.)
    parts.append("-Iinclude")

    # 3) then compat_next (if present)
    compat_next = LOCAL_INC / "compat_next"
    if compat_next.exists():
        parts.append("-Iinclude/compat_next")

    # 4) then mirrored glibc tree LAST (so it can’t override shims)
    if MIRROR_ROOT.exists():
        parts.append("-Iinclude/glibc-src")

        # Optional: also add common subdirs (harmless; helps with path includes)
        for sub in ("include", "sysdeps", "elf"):
            p = MIRROR_ROOT / sub
            if p.exists():
                parts.append(f"-Iinclude/glibc-src/{sub}")

    # 5) add any other include subdirectories (but keep them after the explicit ones)
    #    This is mainly for nested dirs you might create later.
    for d in collect_include_dirs_under(LOCAL_INC):
        rel = d.relative_to(PROJECT_ROOT)
        flag = f"-I{rel}"
        if flag in parts:
            continue
        # Avoid re-adding the main ones redundantly before/after
        if rel.as_posix() in ("include", "include/special_shims", "include/compat_next", "include/glibc-src"):
            continue
        parts.append(flag)

    return " ".join(parts)

def write_cppflags_file(cppflags: str):
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    CPPFLAGS_FILE.write_text(cppflags + "\n")
    log(f"[flags] wrote CPPFLAGS to {CPPFLAGS_FILE}")

# ---------------------------------------------------------------------------
# Error parsing
# ---------------------------------------------------------------------------

MISSING_PATTERNS = [
    re.compile(r"fatal error:\s+([^:\s]+): No such file or directory"),
    re.compile(r"error:\s+'([^']+)' file not found"),
]

def parse_missing_headers(err_text: str) -> list[str]:
    missing: list[str] = []
    for line in err_text.splitlines():
        for pat in MISSING_PATTERNS:
            m = pat.search(line)
            if m:
                hdr = m.group(1).strip()
                if hdr.endswith(".h"):
                    missing.append(hdr)

    out: list[str] = []
    seen = set()
    for h in missing:
        if h not in seen:
            seen.add(h)
            out.append(h)
    return out

# ---------------------------------------------------------------------------
# Glibc header search / mirroring
# ---------------------------------------------------------------------------

def find_candidates_in_glibc(src_path: Path, hdr: str) -> list[Path]:
    base = os.path.basename(hdr)
    try:
        out = subprocess.check_output(["find", str(src_path), "-name", base], text=True)
    except subprocess.CalledProcessError:
        return []
    cands = [Path(line.strip()) for line in out.splitlines() if line.strip()]
    return [p for p in cands if p.name == base]

def choose_best(cands: list[Path]) -> Path | None:
    if not cands:
        return None

    non_x32 = [p for p in cands if "/x32/" not in str(p)]
    if non_x32:
        cands = non_x32

    for p in cands:
        if "/sysdeps/generic/" in str(p):
            return p
    for p in cands:
        if f"/sysdeps/{ARCH}/" in str(p):
            return p
    for p in cands:
        if "/include/" in str(p):
            return p
    return cands[0]

def mirror_preserving_tree(src_abs: Path) -> tuple[bool, str]:
    try:
        rel = src_abs.relative_to(GLIBC_SRC)
    except ValueError:
        return False, ""
    dst = MIRROR_ROOT / rel
    if not dst.exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src_abs, dst)
    return True, str(rel)

def mirror_flat_alias(hdr: str, src_abs: Path) -> tuple[bool, str]:
    if "/" in hdr:
        return False, ""
    alias_dir = MIRROR_ROOT / "include"
    alias_dir.mkdir(parents=True, exist_ok=True)
    alias_dst = alias_dir / os.path.basename(hdr)
    if not alias_dst.exists():
        shutil.copy2(src_abs, alias_dst)
        return True, str(alias_dst.relative_to(MIRROR_ROOT))
    return False, str(alias_dst.relative_to(MIRROR_ROOT))

# ---------------------------------------------------------------------------
# Special shims resolution
# ---------------------------------------------------------------------------

def require_special_shim(hdr: str):
    """
    Ensure a manual shim exists under include/special_shims/ for blocked headers.

    (2) Mapping:
      dl-tunables.h might be reported either as:
        - dl-tunables.h
        - elf/dl-tunables.h
      We normalize to the shim path you actually want to store.
    """
    # Normalize common ambiguous include spellings -> canonical shim path
    if hdr == "dl-tunables.h":
        hdr = "elf/dl-tunables.h"

    shim_path = SPECIAL_SHIMS_DIR / hdr
    if not shim_path.exists():
        raise FileNotFoundError(
            f"Blocked header {hdr} requires manual shim at: {shim_path}"
        )
    return shim_path

# ---------------------------------------------------------------------------
# Main loop
# ---------------------------------------------------------------------------

def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument("--glibc-src", required=True, help="Path to your glibc checkout")
    return p.parse_args()

def main():
    global GLIBC_SRC

    ensure_dirs()
    ensure_required_repo_headers()

    args = parse_args()
    GLIBC_SRC = Path(args.glibc_src).resolve()

    log(f"GLIBC_SRC is: {GLIBC_SRC}")
    log(f"PROJECT_ROOT={PROJECT_ROOT}")
    log(f"ERRFILE={ERRFILE} MAX_ITERS={MAX_ITERS}")
    log(f"ARCH={ARCH}")

    ensure_glibc_sources()

    for it in range(1, MAX_ITERS + 1):
        log(f"[loop] Iteration {it}")

        if ERRFILE.exists():
            ERRFILE.unlink()
        ERRFILE.parent.mkdir(parents=True, exist_ok=True)
        ERRFILE.touch()

        cppflags = compute_cppflags()
        log(f"[flags] CPPFLAGS={cppflags}")
        env = os.environ.copy()
        env["CPPFLAGS"] = cppflags

        res = sh(["make", "-f", "generate.mk", "-s", "malloc-standalone"], cwd=PROJECT_ROOT, capture=True, env=env)

        # If Makefile didn't redirect stderr, save it
        if ERRFILE.stat().st_size == 0 and res.stderr:
            ERRFILE.write_text(res.stderr or "")

        if res.returncode == 0:
            write_cppflags_file(cppflags)
            log("[loop] Build succeeded.")
            return 0

        err_text = ERRFILE.read_text(errors="ignore")
        missing_headers = parse_missing_headers(err_text)

        if not missing_headers:
            log("[loop] No missing-header diagnostics found.")
            log("[loop] --- tail compile.err ---")
            print("\n".join(err_text.splitlines()[-80:]))
            return 2

        any_added = False
        log("[loop] Missing headers found; mirroring from glibc or requiring special shims …")

        for hdr in missing_headers:
            base = os.path.basename(hdr)

            if base in BLOCKED_HEADERS:
                # Require manual shim under include/special_shims/
                log(f"blocked header {hdr}; requiring manual shim")
                require_special_shim(hdr)
                continue

            best: Path | None = None

            # If hdr includes a path, try direct
            if "/" in hdr:
                direct = GLIBC_SRC / Path(hdr)
                if direct.exists():
                    best = direct
                    log(f"using direct path for {hdr}: {best}")
                else:
                    log(f"direct path {direct} does not exist, will search by basename")

            if best is None:
                cands = find_candidates_in_glibc(GLIBC_SRC, hdr)
                if not cands:
                    log(f"WARNING: no candidate found for {hdr} in {GLIBC_SRC}")
                    continue
                best = choose_best(cands)
                if not best:
                    log(f"WARNING: failed choosing best for {hdr}")
                    continue

            ok, rel = mirror_preserving_tree(best)
            if ok:
                log(f"mirrored: {rel}")
                any_added = True

            alias_added, alias_rel = mirror_flat_alias(hdr, best)
            if alias_added:
                log(f"alias: {alias_rel}")
                any_added = True

        if not any_added:
            log("[loop] Could not add any new headers this round. Stopping.")
            log("[loop] --- tail compile.err ---")
            print("\n".join(err_text.splitlines()[-80:]))
            return 3

        log("[loop] Re-trying build after adding headers…")

    log(f"[loop] Reached MAX_ITERS={MAX_ITERS} without resolving all missing headers.")
    return 4

if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as e:
        print(f"[autoinclude] ERROR running {e.cmd}: rc={e.returncode}")
        if e.stderr:
            print(e.stderr)
        sys.exit(1)
    except FileNotFoundError as e:
        print(f"[autoinclude] ERROR: {e}")
        sys.exit(1)
