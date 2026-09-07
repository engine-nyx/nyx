#!/usr/bin/env python3
import os
import re
import sys

FILES = "abcdefgh"
RANKS = "87654321"

BOARD_LINES = 20

def parse_square(s):
    s = s.strip().lower()
    m = re.fullmatch(r'([a-h])([1-8])', s)
    if not m:
        return None
    file, rank = m.group(1), m.group(2)
    return (int(rank) - 1) * 8 + FILES.index(file)

def print_bitboard(bb):
    print("  ┌───┬───┬───┬───┬───┬───┬───┬───┐")
    for i in range(8):
        print(f"{8 - i} │", end="")
        for j in range(8):
            mask = 1 << (((7 - i) * 8) + j)
            print(f" {'X' if bb & mask else ' '} │", end="")
        print()
        if i < 7:
            print("  ├───┼───┼───┼───┼───┼───┼───┼───┤")
    print("  └───┴───┴───┴───┴───┴───┴───┴───┘")
    print("    " + "   ".join(FILES))
    print()
    print(f"dec: {bb}")
    print(f"hex: 0x{bb:016X}")

def redraw(bb):
    sys.stdout.write(f"\033[{BOARD_LINES + 3}A\033[J")
    print_bitboard(bb)
    sys.stdout.write("\n")
    sys.stdout.flush()

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
        arg = sys.argv[1]
        try:
            if arg.startswith("0x") or arg.startswith("0X"):
                bb = int(arg, 16)
            else:
                bb = int(arg)
        except ValueError:
            usage()

    print_bitboard(bb)
    sys.stdout.write("\n")
    sys.stdout.flush()

    while True:
        try:
            line = input("toggle square (e.g. e1, A8): ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not line:
            break
        sq = parse_square(line)
        if sq is None:
            redraw(bb)
            continue
        bb ^= 1 << sq
        redraw(bb)

if __name__ == "__main__":
    main()
