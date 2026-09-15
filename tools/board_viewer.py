#!/usr/bin/env python3
# Copyright (c) 2026 Kilian Chung
# SPDX-License-Identifier: NLPL

import os
import re
import select
import sys
import termios
import time
import tty

FILES = "abcdefgh"

EMPTY = 0
WHITE, BLACK = 0, 1
WP, WN, WB, WR, WQ, WK = 1, 2, 3, 4, 5, 6
BP, BN, BB, BR, BQ, BK = 7, 8, 9, 10, 11, 12

START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

SYM = {
    WP: "\u2659", WN: "\u2658", WB: "\u2657", WR: "\u2656", WQ: "\u2655", WK: "\u2654",
    BP: "\u265f", BN: "\u265e", BB: "\u265d", BR: "\u265c", BQ: "\u265b", BK: "\u265a",
}

FEN_MAP = {
    "P": WP, "N": WN, "B": WB, "R": WR, "Q": WQ, "K": WK,
    "p": BP, "n": BN, "b": BB, "r": BR, "q": BQ, "k": BK,
}

PROMO_CODE = {"q": 5, "r": 4, "b": 3, "n": 2}

KNIGHT = ((1, 2), (2, 1), (2, -1), (1, -2), (-1, -2), (-2, -1), (-2, 1), (-1, 2))
ORTHO = ((1, 0), (0, 1), (-1, 0), (0, -1))
DIAG = ((1, 1), (1, -1), (-1, 1), (-1, -1))
ALL8 = ORTHO + DIAG


def pcode(t, c):
    return t + 6 * c

def type_of(p):
    return ((p - 1) % 6) + 1

def color_of(p):
    return BLACK if p > 6 else WHITE

def other(c):
    return BLACK if c == WHITE else WHITE

def in_bounds(f, r):
    return 0 <= f < 8 and 0 <= r < 8

def sqidx(name):
    return (int(name[1]) - 1) * 8 + FILES.index(name[0].lower())

def sqname(i):
    return FILES[i % 8] + str(i // 8 + 1)


class State:
    __slots__ = ("board", "turn", "castling", "ep", "halfmove", "fullmove")

    def __init__(self, board, turn, castling, ep, halfmove, fullmove):
        self.board = board
        self.turn = turn
        self.castling = castling
        self.ep = ep
        self.halfmove = halfmove
        self.fullmove = fullmove


def attacked(board, sq, by):
    f, r = sq % 8, sq // 8

    def get(ff, rr):
        return board[rr * 8 + ff] if in_bounds(ff, rr) else EMPTY

    if by == WHITE:
        if get(f - 1, r - 1) == WP or get(f + 1, r - 1) == WP:
            return True
    else:
        if get(f - 1, r + 1) == BP or get(f + 1, r + 1) == BP:
            return True

    kn = pcode(2, by)
    for df, dr in KNIGHT:
        if get(f + df, r + dr) == kn:
            return True

    for df, dr in ORTHO:
        ff, rr = f + df, r + dr
        while in_bounds(ff, rr):
            p = board[rr * 8 + ff]
            if p != EMPTY:
                if color_of(p) == by and type_of(p) in (WR, WQ):
                    return True
                break
            ff += df
            rr += dr

    for df, dr in DIAG:
        ff, rr = f + df, r + dr
        while in_bounds(ff, rr):
            p = board[rr * 8 + ff]
            if p != EMPTY:
                if color_of(p) == by and type_of(p) in (WB, WQ):
                    return True
                break
            ff += df
            rr += dr

    kg = pcode(6, by)
    for df, dr in ALL8:
        if get(f + df, r + dr) == kg:
            return True

    return False


def find_king(board, c):
    kc = pcode(6, c)
    for i, p in enumerate(board):
        if p == kc:
            return i
    return None


def pseudo_moves(st):
    board, c = st.board, st.turn
    moves = []

    def add(src, idx, promo=None):
        tg = board[idx]
        if tg == EMPTY or color_of(tg) != c:
            moves.append((src, idx, promo))

    def slide(src, df, dr):
        ff, rr = src % 8 + df, src // 8 + dr
        while in_bounds(ff, rr):
            idx = rr * 8 + ff
            tg = board[idx]
            if tg == EMPTY:
                moves.append((src, idx, None))
            else:
                if color_of(tg) != c:
                    moves.append((src, idx, None))
                break
            ff += df
            rr += dr

    for sq, p in enumerate(board):
        if p == EMPTY or color_of(p) != c:
            continue
        t = type_of(p)
        f, r = sq % 8, sq // 8

        if t == WP:
            d = 1 if c == WHITE else -1
            start_r, promo_r = (1, 7) if c == WHITE else (6, 0)
            rr = r + d
            if in_bounds(f, rr) and board[rr * 8 + f] == EMPTY:
                if rr == promo_r:
                    for pr in ("q", "r", "b", "n"):
                        moves.append((sq, rr * 8 + f, pr))
                else:
                    moves.append((sq, rr * 8 + f, None))
                    if r == start_r:
                        rr2 = r + 2 * d
                        if board[rr2 * 8 + f] == EMPTY:
                            moves.append((sq, rr2 * 8 + f, None))
            for df in (-1, 1):
                ff, rr = f + df, r + d
                if in_bounds(ff, rr):
                    idx = rr * 8 + ff
                    tg = board[idx]
                    if tg != EMPTY and color_of(tg) != c:
                        if rr == promo_r:
                            for pr in ("q", "r", "b", "n"):
                                moves.append((sq, idx, pr))
                        else:
                            moves.append((sq, idx, None))
                    elif st.ep is not None and idx == st.ep:
                        moves.append((sq, idx, None))

        elif t == WN:
            for df, dr in KNIGHT:
                if in_bounds(f + df, r + dr):
                    add(sq, (r + dr) * 8 + f + df)

        elif t in (WB, WR, WQ):
            dirs = DIAG if t == WB else ORTHO if t == WR else ALL8
            for df, dr in dirs:
                slide(sq, df, dr)

        else:
            for df, dr in ALL8:
                if in_bounds(f + df, r + dr):
                    add(sq, (r + dr) * 8 + f + df)
            if c == WHITE:
                if (st.castling[0] and board[7] == WR and board[5] == EMPTY
                        and board[6] == EMPTY and not attacked(board, 4, BLACK)
                        and not attacked(board, 5, BLACK) and not attacked(board, 6, BLACK)):
                    moves.append((4, 6, None))
                if (st.castling[1] and board[0] == WR and board[1] == EMPTY
                        and board[2] == EMPTY and board[3] == EMPTY and not attacked(board, 4, BLACK)
                        and not attacked(board, 3, BLACK) and not attacked(board, 2, BLACK)):
                    moves.append((4, 2, None))
            else:
                if (st.castling[2] and board[63] == BR and board[61] == EMPTY
                        and board[62] == EMPTY and not attacked(board, 60, WHITE)
                        and not attacked(board, 61, WHITE) and not attacked(board, 62, WHITE)):
                    moves.append((60, 62, None))
                if (st.castling[3] and board[56] == BR and board[57] == EMPTY
                        and board[58] == EMPTY and board[59] == EMPTY and not attacked(board, 60, WHITE)
                        and not attacked(board, 59, WHITE) and not attacked(board, 58, WHITE)):
                    moves.append((60, 58, None))

    return moves


def apply(move, st):
    f, t, promo = move
    brd = st.board[:]
    cast = list(st.castling)
    ep = st.ep
    hm = st.halfmove
    fm = st.fullmove
    p = brd[f]
    c = color_of(p)
    tp = type_of(p)

    is_capture = brd[t] != EMPTY
    brd[f] = EMPTY

    if tp == WK and f in (4, 60):
        if c == WHITE:
            if t == 6:
                brd[7], brd[5] = EMPTY, WR
            elif t == 2:
                brd[0], brd[3] = EMPTY, WR
        else:
            if t == 62:
                brd[63], brd[61] = EMPTY, BR
            elif t == 58:
                brd[56], brd[59] = EMPTY, BR

    if tp == WP and t == ep:
        brd[t - 8 if c == WHITE else t + 8] = EMPTY
        is_capture = True

    if promo:
        brd[t] = pcode(PROMO_CODE[promo], c)
    else:
        brd[t] = p

    if tp == WK and f in (4, 60):
        if c == WHITE:
            cast[0] = cast[1] = False
        else:
            cast[2] = cast[3] = False
    if f == 0 or t == 0:
        cast[1] = False
    if f == 7 or t == 7:
        cast[0] = False
    if f == 56 or t == 56:
        cast[3] = False
    if f == 63 or t == 63:
        cast[2] = False

    ep2 = (f + t) // 2 if tp == WP and abs(t - f) == 16 else None
    hm2 = 0 if (tp == WP or is_capture) else hm + 1
    fm2 = fm + (1 if c == BLACK else 0)

    return State(brd, other(c), (cast[0], cast[1], cast[2], cast[3]), ep2, hm2, fm2)


def legal_moves(st):
    res = []
    c = st.turn
    for mv in pseudo_moves(st):
        s2 = apply(mv, st)
        ks = find_king(s2.board, c)
        if ks is not None and not attacked(s2.board, ks, other(c)):
            res.append(mv)
    return res


def parse_fen(fen):
    parts = fen.split()
    board = [EMPTY] * 64
    ranks = parts[0].split("/")
    for r, row in enumerate(ranks):
        f = 0
        for ch in row:
            if ch.isdigit():
                f += int(ch)
            else:
                board[(7 - r) * 8 + f] = FEN_MAP[ch]
                f += 1
    turn = WHITE if parts[1] == "w" else BLACK
    cr = parts[2] if len(parts) > 2 else "-"
    castling = ("K" in cr, "Q" in cr, "k" in cr, "q" in cr)
    ep = sqidx(parts[3]) if len(parts) > 3 and parts[3] != "-" else None
    halfmove = int(parts[4]) if len(parts) > 4 else 0
    fullmove = int(parts[5]) if len(parts) > 5 else 1
    return State(board, turn, castling, ep, halfmove, fullmove)


def parse_uci(s, st):
    m = re.fullmatch(r"([a-h][1-8])([a-h][1-8])([QRBNqrbn]?)", s)
    if not m:
        return None
    f, t = sqidx(m.group(1)), sqidx(m.group(2))
    promo = m.group(3).lower() or None
    for mv in legal_moves(st):
        if (f, t, promo) == mv:
            return mv
    return None


def uci_string(mv):
    return sqname(mv[0]) + sqname(mv[1]) + (mv[2] or "")


WM_W = 6


def move_lines(moves, cursor):
    hl = cursor - 1
    out = []
    for i in range(0, len(moves), 2):
        wm = moves[i]
        bm = moves[i + 1] if i + 1 < len(moves) else ""
        wp = wm.ljust(WM_W)
        if i == hl:
            wp = "\033[7m" + wp + "\033[0m"
        if i + 1 == hl:
            bp = "\033[7m" + bm + "\033[0m"
        else:
            bp = bm
        out.append(f"{i // 2 + 1}. {wp} {bp}".rstrip())
    return out


def print_board(st):
    print("   \u250c\u2500\u2500\u2500\u252c\u2500\u2500\u2500\u252c\u2500\u2500\u2500\u252c\u2500\u2500\u2500\u252c\u2500\u2500\u2500\u252c\u2500\u2500\u2500\u252c\u2500\u2500\u2500\u252c\u2500\u2500\u2500\u2510")
    for rr in range(7, -1, -1):
        print(f" {rr + 1} \u2502", end="")
        for ff in range(8):
            print(f" {SYM.get(st.board[rr * 8 + ff], ' ')} \u2502", end="")
        print()
        if rr:
            print("   \u251c\u2500\u2500\u2500\u253c\u2500\u2500\u2500\u253c\u2500\u2500\u2500\u253c\u2500\u2500\u2500\u253c\u2500\u2500\u2500\u253c\u2500\u2500\u2500\u253c\u2500\u2500\u2500\u253c\u2500\u2500\u2500\u2524")
    print("   \u2514\u2500\u2500\u2500\u2534\u2500\u2500\u2500\u2534\u2500\u2500\u2500\u2534\u2500\u2500\u2500\u2534\u2500\u2500\u2500\u2534\u2500\u2500\u2500\u2534\u2500\u2500\u2500\u2534\u2500\u2500\u2500\u2518")
    print("     " + "   ".join(FILES))


def render(st, moves, cursor, msg=None, up=0):
    if up:
        sys.stdout.write(f"\033[{up}A\r\033[J")
    else:
        sys.stdout.write("\r\033[J")
    print("Board Viewer")
    print()
    print_board(st)
    print()
    if moves:
        mlines = move_lines(moves, cursor)
        print("Moves:")
        for ln in mlines:
            print(" " + ln)
        nlines = 2 + 18 + 1 + 1 + len(mlines)
    else:
        print("Moves: (none)")
        nlines = 2 + 18 + 1 + 1
    lm = legal_moves(st)
    if not lm:
        ks = find_king(st.board, st.turn)
        outcome = "Checkmate" if ks is not None and attacked(st.board, ks, other(st.turn)) else "Stalemate"
    else:
        outcome = ""
    turn = "White" if st.turn == WHITE else "Black"
    cast = "".join(x for x, f in (("K", st.castling[0]), ("Q", st.castling[1]), ("k", st.castling[2]), ("q", st.castling[3])) if f) or "-"
    ep = sqname(st.ep) if st.ep is not None else "-"
    print(f"{turn} to move | ply {cursor}/{len(moves)} | fullmove {st.fullmove} | halfmove {st.halfmove} | castling {cast} | ep {ep} {outcome}")
    nlines += 1
    if msg:
        print(msg)
        nlines += 1
    print("arrow-keys navigation | r reset | q quit | UCI move (e.g. e2e4, e7e8q)")
    nlines += 1
    if sys.stdout.isatty():
        sys.stdout.write("\n\033[1A> ")
    else:
        sys.stdout.write("> ")
    sys.stdout.flush()
    return nlines + 1


def usage():
    print(f"Usage: {sys.argv[0]} [fen]")
    print("  fen defaults to the start position if omitted")
    sys.exit(1)


def get_command():
    if not sys.stdin.isatty():
        try:
            line = input()
        except (EOFError, KeyboardInterrupt):
            return ("quit", "")
        return ("text", line.strip())
    buf = ""
    while True:
        try:
            data = os.read(sys.stdin.fileno(), 1)
        except KeyboardInterrupt:
            return ("quit", "")
        if not data:
            return ("quit", "")
        ch = data.decode(errors="replace")
        if ch == "\x04":
            return ("quit", "")
        if ch == "\x1b":
            seq = ""
            dl = time.time() + 0.1
            while len(seq) < 2 and time.time() < dl:
                r, _, _ = select.select([sys.stdin.fileno()], [], [], 0.03)
                if not r:
                    break
                try:
                    d = os.read(sys.stdin.fileno(), 1)
                except OSError:
                    break
                if not d:
                    break
                seq += d.decode(errors="replace")
            if len(seq) == 2 and seq[0] == "[" and seq[1] in "ABCD":
                return ("key", {"A": "up", "B": "down", "C": "right", "D": "left"}[seq[1]])
            continue
        if ch in ("\n", "\r"):
            if ch == "\r" and select.select([sys.stdin.fileno()], [], [], 0)[0]:
                os.read(sys.stdin.fileno(), 1)
            break
        if ch in ("\x7f", "\x08"):
            if buf:
                buf = buf[:-1]
                sys.stdout.write("\b \b")
                sys.stdout.flush()
            continue
        buf += ch
        sys.stdout.write(ch)
        sys.stdout.flush()
        if len(buf) == 1 and buf in ("j", "k", "q", "r"):
            return ("key", buf)
    if len(buf) == 1 and buf in ("j", "k", "q", "r"):
        return ("key", buf)
    return ("text", buf.strip())


def main():
    if len(sys.argv) > 2:
        usage()
    fen = sys.argv[1] if len(sys.argv) > 1 else START_FEN
    try:
        states = [parse_fen(fen)]
    except Exception:
        print(f"Board Viewer\n\nError: invalid FEN: {fen}\n")
        usage()

    moves = []
    cursor = 0
    n = render(states[cursor], moves, cursor)

    ttyold = None
    if sys.stdin.isatty():
        fd = sys.stdin.fileno()
        ttyold = termios.tcgetattr(fd)
        tty.setcbreak(fd)

    try:
        while True:
            cmd, line = get_command()
            up = n - 1 if sys.stdin.isatty() else (n - 1 if cmd == "key" else n)
            if cmd == "quit":
                print()
                break
            if not line:
                n = render(states[cursor], moves, cursor, up=up)
            elif line in ("q", "quit"):
                print()
                break
            elif line in ("j", "f", "fwd", "forward", ">", "right", "down", "end", "last"):
                cursor = len(states) - 1 if line in ("down", "end", "last") else min(cursor + 1, len(states) - 1)
                n = render(states[cursor], moves, cursor, up=up)
            elif line in ("k", "b", "back", "<", "left", "up", "start", "first", "home"):
                cursor = 0 if line in ("up", "start", "first", "home") else max(cursor - 1, 0)
                n = render(states[cursor], moves, cursor, up=up)
            elif line in ("r", "reset"):
                states = states[:1]
                moves = []
                cursor = 0
                n = render(states[cursor], moves, cursor, up=up)
            else:
                mv = parse_uci(line, states[cursor])
                if mv is None:
                    n = render(states[cursor], moves, cursor, msg=f"illegal: {line}", up=up)
                    continue
                states = states[: cursor + 1]
                states.append(apply(mv, states[cursor]))
                moves = moves[:cursor] + [uci_string(mv)]
                cursor += 1
                n = render(states[cursor], moves, cursor, up=up)
    finally:
        if ttyold is not None:
            termios.tcsetattr(sys.stdin.fileno(), termios.TCSADRAIN, ttyold)


if __name__ == "__main__":
    main()
