#!/usr/bin/env python3
# Copyright (c) 2026 Kilian Chung
# SPDX-License-Identifier: NLPL

import os
import re
import sys
from datetime import date

AUTHOR = "Kilian Chung"
SPDX_ID = "NLPL"

COMMENT_STYLE = {
    ".c": "//", ".h": "//", ".cpp": "//", ".hpp": "//", ".cc": "//",
    ".java": "//", ".go": "//", ".rs": "//", ".js": "//", ".ts": "//",
    ".py": "#", ".sh": "#", ".bash": "#", ".yml": "#", ".yaml": "#",
    ".toml": "#", ".ini": "#", ".cfg": "#",
}

YEAR_RE = re.compile(r"(\d{4})-(\d{4})|(\d{4})")

def current_year():
    return date.today().year

HEADER_SCAN_LINES = 12

def comment_token(path):
    base = os.path.basename(path)
    if base == "Makefile" or base == "makefile":
        return "#"
    return COMMENT_STYLE.get(os.path.splitext(path)[1].lower())

def copyright_line(token, year):
    return f"{token} Copyright (c) {year} {AUTHOR}"

def spdx_line(token):
    return f"{token} SPDX-License-Identifier: {SPDX_ID}"

def is_comment_line(line, token):
    s = line.lstrip()
    return s.startswith(token), s[len(token):].lstrip() if s.startswith(token) else s

def spdx_index(lines, token):
    for i, ln in enumerate(lines[:HEADER_SCAN_LINES]):
        ok, s = is_comment_line(ln, token)
        if ok and "SPDX-License-Identifier" in s:
            return i
    return None

def copyright_index(lines, token, around):
    best, best_d = None, None
    for i, ln in enumerate(lines[:HEADER_SCAN_LINES]):
        ok, s = is_comment_line(ln, token)
        if ok and "Copyright" in s and "SPDX" not in s:
            d = abs(i - around)
            if best_d is None or d < best_d:
                best, best_d = i, d
    return best

def update_year(line, year):
    m = YEAR_RE.search(line)
    if not m:
        return line, False
    if m.group(1) and m.group(2):
        start, end = int(m.group(1)), int(m.group(2))
        if end == year:
            return line, False
        return line.replace(m.group(0), f"{start}-{year}"), True
    single = int(m.group(3))
    if single == year:
        return line, False
    return line.replace(m.group(0), f"{single}-{year}"), True

def process(path, dry_run=False, year=None):
    year = year or current_year()
    token = comment_token(path)
    if not token:
        print(f"skip (no comment style): {path}")
        return "skip"

    try:
        with open(path) as f:
            lines = f.readlines()
    except OSError as e:
        print(f"error: {path}: {e}")
        return "error"

    spdx_idx = spdx_index(lines, token)

    if spdx_idx is not None:
        c_idx = copyright_index(lines, token, spdx_idx)
        if c_idx is None:
            lines.insert(spdx_idx, copyright_line(token, year) + "\n")
            status = "added copyright"
        else:
            line, changed = update_year(lines[c_idx], year)
            if changed:
                lines[c_idx] = line
                status = "year updated"
            else:
                print(f"ok: {path}")
                return "ok"
    else:
        insert_at = 1 if lines and lines[0].startswith("#!") else 0
        header = [copyright_line(token, year) + "\n", spdx_line(token) + "\n", "\n"]
        lines[insert_at:insert_at] = header
        status = "prepended"

    if dry_run:
        print(f"would {status}: {path}")
        return "dry"

    with open(path, "w") as f:
        f.writelines(lines)
    print(f"{status}: {path}")
    return "changed"

def collect(paths):
    files = []
    for p in paths:
        if os.path.isdir(p):
            for root, dirs, names in os.walk(p):
                dirs[:] = [d for d in dirs if d not in (".git", "build")]
                for n in names:
                    files.append(os.path.join(root, n))
        else:
            files.append(p)
    return files

def main():
    print("SPDX Prepender\n")

    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} [-n] <file|dir>...")
        sys.exit(1)

    args = sys.argv[1:]
    dry_run = False
    if args and args[0] in ("-n", "--dry-run"):
        dry_run = True
        args = args[1:]

    counts = {}
    for path in collect(args):
        status = process(path, dry_run)
        counts[status] = counts.get(status, 0) + 1

    print()
    for status, n in sorted(counts.items()):
        print(f"{n:5d} {status}")

if __name__ == "__main__":
    main()