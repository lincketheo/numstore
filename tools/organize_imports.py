#!/usr/bin/env python3
"""
reorder_includes.py
Run from repo root:
    python3 reorder_includes.py
"""

import pathlib, re, sys

MODULE_ORDER = ["core", "compiler", "client", "server", "numstore"]

RE_PRAGMA  = re.compile(r'^\s*#\s*pragma\s+once\b')
RE_SYSINC  = re.compile(r'^\s*#\s*include\s*<[^>]+>')
RE_QUOTE   = re.compile(r'^(\s*#\s*include\s*)"([^"]+)"(.*)$')

# ---------------------------------------------------------------------- #
def ensure_pragma_once(lines):
    """Put '#pragma once' on line 0 of a header; remove duplicates."""
    changed = False
    idx = next((i for i,l in enumerate(lines[:10]) if RE_PRAGMA.match(l)), None)
    if idx is None:
        lines.insert(0, "#pragma once\n"); changed = True
    elif idx != 0:
        del lines[idx]; lines.insert(0, "#pragma once\n"); changed = True
    i = 1
    while i < len(lines):
        if RE_PRAGMA.match(lines[i]): del lines[i]; changed = True
        else: i += 1
    return changed
# ---------------------------------------------------------------------- #
def collect_include_block(lines, start):
    """Return (end_idx, quoted_includes, system_includes)."""
    q_inc, s_inc, i = [], [], start
    while i < len(lines):
        ln = lines[i]
        if RE_QUOTE.match(ln):   q_inc.append(ln)
        elif RE_SYSINC.match(ln): s_inc.append(ln)
        elif ln.strip() == "" or ln.lstrip().startswith("//"):
            pass
        else: break
        i += 1
    return i, q_inc, s_inc
# ---------------------------------------------------------------------- #
def ensure_comment(ln):
    return ln if "//" in ln else ln.rstrip("\n") + " // TODO\n"

def regroup_project_includes(q_inc):
    """Return list of includes regrouped by MODULE_ORDER with TODO comments."""
    buckets = {m: [] for m in MODULE_ORDER}
    other   = []
    for ln in q_inc:
        module = ln.split('"')[1].split('/',1)[0]
        (buckets.get(module, other)).append(ln)

    out = []
    for m in MODULE_ORDER:
        if buckets[m]:
            out.extend(map(ensure_comment, buckets[m]))
            out.append("\n")
    if other:
        out.extend(map(ensure_comment, other))
        out.append("\n")
    return out
# ---------------------------------------------------------------------- #
def process_one(path: pathlib.Path) -> bool:
    txt = path.read_text().splitlines(keepends=True)
    changed = False

    # 1. Headers: guarantee #pragma once
    if path.suffix == ".h":
        changed |= ensure_pragma_once(txt)

    # 2. Locate first non-pragma / non-comment / non-blank
    i = 0
    while i < len(txt) and (RE_PRAGMA.match(txt[i]) or
                            txt[i].strip() == "" or
                            txt[i].lstrip().startswith("//")):
        i += 1

    end, quoted, system = collect_include_block(txt, i)

    # 3. Self-header detection (only for .c)
    self_header_line = None
    if path.suffix == ".c":
        want = path.stem + ".h"
        for idx, ln in enumerate(quoted):
            hdrfile = ln.split('"')[1].rsplit('/',1)[-1]
            if hdrfile == want:
                self_header_line = quoted.pop(idx)
                break

    if self_header_line or quoted or system:
        block = []

        # a. self-header first (verbatim, no comment tweak)
        if self_header_line:
            if not self_header_line.endswith("\n"): self_header_line += "\n"
            block.append(self_header_line)
            block.append("\n")

        # b. system includes (original order)
        if system:
            block.extend(map(ensure_comment, system))
            block.append("\n")

        # c. project includes grouped
        if quoted:
            block.extend(regroup_project_includes(quoted))

        # d. exactly one blank line after final include section
        if block and block[-1].strip(): block.append("\n")
        while len(block) > 1 and block[-2].strip() == "" and block[-1].strip() == "":
            block.pop()

        # e. splice into file
        txt[i:end] = block
        changed = True

    if changed:
        path.write_text("".join(txt))
    return changed
# ---------------------------------------------------------------------- #
def main() -> int:
    root = pathlib.Path(".")
    mods = sum(process_one(p) for p in root.rglob("*.[ch]") if p.is_file())
    print(f"Updated {mods} file(s).")
    return 0

if __name__ == "__main__":
    sys.exit(main())

