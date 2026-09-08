#!/usr/bin/env python3
# Copyright (c) 2026 Kilian Chung
# SPDX-License-Identifier: NLPL

import os
import re
import sys

FILES = "abcdefgh"
RANKS = "87654321"

BOARD_LINES = 21
PROMPT_LINES = 2
PROMPT = "toggle square or reset bitboard: "

def parse_square(s):
    s = s.strip().lower()
    m = re.fullmatch(r'([a-h])([1-8])', s)
    if not m:
        return None
    file, rank = m.group(1), m.group(2)
    return (int(rank) - 1) * 8 + FILES.index(file)

def print_bitboard(bb):
    print("   ┌───┬───┬───┬───┬───┬───┬───┬───┐")
    for i in range(8):
        print(f" {8 - i} │", end="")
        for j in range(8):
            mask = 1 << (((7 - i) * 8) + j)
            print(f" {'X' if bb & mask else ' '} │", end="")
        print()
        if i < 7:
            print("   ├───┼───┼───┼───┼───┼───┼───┼───┤")
    print("   └───┴───┴───┴───┴───┴───┴───┴───┘")
    print("     " + "   ".join(FILES))
    print()
    print(f"dec: {bb}")
    print(f"hex: 0x{bb:016X}")

def render(bb, first=False):
    if not first:
        sys.stdout.write(f"\033[{BOARD_LINES + PROMPT_LINES}A\033[J")
    print_bitboard(bb)
    print()
    print(PROMPT)
    sys.stdout.write("\033[1A")
    sys.stdout.write(PROMPT)
    sys.stdout.flush()

def parse_value(arg):
    if arg.startswith("0x") or arg.startswith("0X"):
        return int(arg, 16)
    return int(arg)

def usage():
    print(f"Usage: {os.path.basename(sys.argv[0])} [value]")
    print("  value is a decimal number or hex with a 0x prefix")
    sys.exit(1)

def main():
    print("Bitboard Viewer\n")

    if len(sys.argv) > 2:
        usage()
    bb = 0
    if len(sys.argv) > 1:
        try:
            bb = parse_value(sys.argv[1])
        except ValueError:
            usage()

    render(bb, first=True)

    while True:
        try:
            line = input().strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not line:
            break
        sq = parse_square(line)
        if sq is not None:
            bb ^= 1 << sq
        else:
            try:
                bb = parse_value(line)
            except ValueError:
                pass
        render(bb)
        continue

if __name__ == "__main__":
    main()
