#!/usr/bin/env python3
"""Convert PGN game(s) to UCI "position ... moves ..." commands.

Pure standard library, no dependencies. Comments ({...}, ;...), variations
(...), NAGs ($1), move numbers and the result token are ignored.

Usage:
    python3 pgn2uci.py game.pgn
    python3 pgn2uci.py game.pgn -n 40        # only the first 40 plies
    python3 pgn2uci.py game.pgn --final-fen  # also print the final FEN
    cat game.pgn | python3 pgn2uci.py -
"""
import argparse
import re
import sys

START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
KNIGHT = [(1, 2), (2, 1), (2, -1), (1, -2), (-1, -2), (-2, -1), (-2, 1), (-1, 2)]
KING = [(1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1)]
DIAG = [(1, 1), (1, -1), (-1, 1), (-1, -1)]
ORTH = [(1, 0), (-1, 0), (0, 1), (0, -1)]
SAN_RE = re.compile(r"^([NBRQK])?([a-h])?([1-8])?x?([a-h][1-8])(?:=?([NBRQ]))?$")


def sq_name(sq):
    return "abcdefgh"[sq % 8] + str(sq // 8 + 1)


class Position:
    """Board with squares 0..63 (a1=0, h1=7, a8=56)."""

    def __init__(self, fen=START_FEN):
        parts = fen.split()
        if len(parts) < 2:
            raise ValueError(f"bad FEN: {fen!r}")
        self.b = ["."] * 64
        rows = parts[0].split("/")
        if len(rows) != 8:
            raise ValueError(f"bad FEN: {fen!r}")
        for i, row in enumerate(rows):
            rank, file = 7 - i, 0
            for ch in row:
                if ch.isdigit():
                    file += int(ch)
                else:
                    self.b[rank * 8 + file] = ch
                    file += 1
        self.white = parts[1] == "w"
        self.castling = set(parts[2]) - {"-"} if len(parts) > 2 else set()
        ep = parts[3] if len(parts) > 3 else "-"
        self.ep = -1 if ep == "-" else (int(ep[1]) - 1) * 8 + "abcdefgh".index(ep[0])
        self.halfmove = int(parts[4]) if len(parts) > 4 else 0
        self.fullmove = int(parts[5]) if len(parts) > 5 else 1

    def copy(self):
        p = Position.__new__(Position)
        p.b = self.b[:]
        p.white, p.castling, p.ep = self.white, set(self.castling), self.ep
        p.halfmove, p.fullmove = self.halfmove, self.fullmove
        return p

    def fen(self):
        rows = []
        for rank in range(7, -1, -1):
            row, empty = "", 0
            for file in range(8):
                ch = self.b[rank * 8 + file]
                if ch == ".":
                    empty += 1
                else:
                    row += (str(empty) if empty else "") + ch
                    empty = 0
            rows.append(row + (str(empty) if empty else ""))
        castling = "".join(c for c in "KQkq" if c in self.castling) or "-"
        ep = sq_name(self.ep) if self.ep >= 0 else "-"
        return (f"{'/'.join(rows)} {'w' if self.white else 'b'} {castling} "
                f"{ep} {self.halfmove} {self.fullmove}")

    # ---- attack detection -------------------------------------------------
    def attacked(self, sq, by_white):
        b = self.b
        f, r = sq % 8, sq // 8
        up = lambda c: c.upper() if by_white else c.lower()

        pr = r - 1 if by_white else r + 1
        if 0 <= pr < 8:
            for pf in (f - 1, f + 1):
                if 0 <= pf < 8 and b[pr * 8 + pf] == up("p"):
                    return True
        for df, dr in KNIGHT:
            nf, nr = f + df, r + dr
            if 0 <= nf < 8 and 0 <= nr < 8 and b[nr * 8 + nf] == up("n"):
                return True
        for df, dr in KING:
            nf, nr = f + df, r + dr
            if 0 <= nf < 8 and 0 <= nr < 8 and b[nr * 8 + nf] == up("k"):
                return True
        for dirs, kinds in ((DIAG, (up("b"), up("q"))), (ORTH, (up("r"), up("q")))):
            for df, dr in dirs:
                nf, nr = f + df, r + dr
                while 0 <= nf < 8 and 0 <= nr < 8:
                    ch = b[nr * 8 + nf]
                    if ch != ".":
                        if ch in kinds:
                            return True
                        break
                    nf, nr = nf + df, nr + dr
        return False

    def king_in_check(self, white):
        k = self.b.index("K" if white else "k")
        return self.attacked(k, not white)

    # ---- move making ------------------------------------------------------
    def push(self, fr, to, promo=""):
        b = self.b
        p = b[fr]
        white = p.isupper()
        pt = p.upper()
        capture = b[to] != "."
        b[to], b[fr] = p, "."
        if pt == "P":
            if to == self.ep and not capture and fr % 8 != to % 8:
                b[to - 8 if white else to + 8] = "."  # en passant capture
                capture = True
            if promo:
                b[to] = promo.upper() if white else promo.lower()
        if pt == "K" and abs(to - fr) == 2:  # castling: move the rook too
            if to > fr:
                b[fr + 1], b[fr + 3] = b[fr + 3], "."
            else:
                b[fr - 1], b[fr - 4] = b[fr - 4], "."
        self.ep = (fr + to) // 2 if pt == "P" and abs(to - fr) == 16 else -1
        if pt == "K":
            self.castling -= set("KQ" if white else "kq")
        for sq, ch in ((0, "Q"), (7, "K"), (56, "q"), (63, "k")):
            if fr == sq or to == sq:
                self.castling.discard(ch)
        self.halfmove = 0 if (pt == "P" or capture) else self.halfmove + 1
        if not white:
            self.fullmove += 1
        self.white = not white

    # ---- SAN parsing ------------------------------------------------------
    def _can_reach(self, fr, to, kind):
        b = self.b
        df, dr = to % 8 - fr % 8, to // 8 - fr // 8
        target = b[to]
        if target != "." and target.isupper() == self.white:
            return False  # own piece on destination
        if kind == "P":
            d = 1 if self.white else -1
            if df == 0:
                if target != ".":
                    return False
                if dr == d:
                    return True
                start = 1 if self.white else 6
                return dr == 2 * d and fr // 8 == start and b[fr + 8 * d] == "."
            return abs(df) == 1 and dr == d and (target != "." or to == self.ep)
        if kind == "N":
            return (abs(df), abs(dr)) in ((1, 2), (2, 1))
        if kind == "K":
            return max(abs(df), abs(dr)) == 1
        diag = abs(df) == abs(dr) != 0
        orth = (df == 0) != (dr == 0)
        if not ((kind in "BQ" and diag) or (kind in "RQ" and orth)):
            return False
        sf, sr = (df > 0) - (df < 0), (dr > 0) - (dr < 0)
        f, r = fr % 8 + sf, fr // 8 + sr
        while (f, r) != (to % 8, to // 8):
            if b[r * 8 + f] != ".":
                return False
            f, r = f + sf, r + sr
        return True

    def parse_san(self, san):
        """Return (from_sq, to_sq, promo_letter) for a SAN move."""
        s = san.replace("e.p.", "").rstrip("+#!?")
        if s.replace("0", "O") in ("O-O", "O-O-O"):
            base = 0 if self.white else 56
            to = base + (6 if s.replace("0", "O") == "O-O" else 2)
            if self.b[base + 4] != ("K" if self.white else "k"):
                raise ValueError(f"illegal castling {san!r}")
            return base + 4, to, ""
        m = SAN_RE.match(s)
        if not m:
            raise ValueError(f"cannot parse move {san!r}")
        kind = m.group(1) or "P"
        d_file, d_rank = m.group(2), m.group(3)
        to = (int(m.group(4)[1]) - 1) * 8 + "abcdefgh".index(m.group(4)[0])
        promo = m.group(5) or ""
        mine = kind if self.white else kind.lower()
        cands = []
        for fr in range(64):
            if self.b[fr] != mine:
                continue
            if d_file and "abcdefgh"[fr % 8] != d_file:
                continue
            if d_rank and str(fr // 8 + 1) != d_rank:
                continue
            if not self._can_reach(fr, to, kind):
                continue
            trial = self.copy()
            trial.push(fr, to, promo)
            if trial.king_in_check(self.white):
                continue  # leaves own king in check
            cands.append(fr)
        if not cands:
            raise ValueError(f"illegal move {san!r}")
        if len(cands) > 1:
            raise ValueError(f"ambiguous move {san!r}")
        return cands[0], to, promo


# ---- PGN scanning ---------------------------------------------------------
TOKEN_RE = re.compile(
    r'\[\s*(?P<tag>\w+)\s+"(?P<val>(?:[^"\\]|\\.)*)"\s*\]'
    r"|(?P<brace>\{[^}]*\})"
    r"|(?P<semi>;[^\n]*)"
    r"|(?P<paren>[()])"
    r"|(?P<tok>[^\s{}\[\]();]+)"
)
RESULTS = {"1-0", "0-1", "1/2-1/2", "*"}


def read_games(text):
    """Yield (headers, [san, ...]) for each game in the text."""
    headers, moves, depth = {}, [], 0
    for m in TOKEN_RE.finditer(text):
        if m.group("tag"):
            if moves:  # a new game starts without the previous one having a result
                yield headers, moves
                headers, moves = {}, []
            headers[m.group("tag")] = m.group("val")
        elif m.group("paren"):
            depth += 1 if m.group("paren") == "(" else -1
        elif m.group("tok") and depth == 0:
            tok = m.group("tok")
            if tok in RESULTS:
                yield headers, moves
                headers, moves = {}, []
                continue
            tok = re.sub(r"^\d+\.+", "", tok)  # "12." / "12..." / "12.Nf3"
            if tok and not tok.startswith("$") and tok not in ("--", "Z0"):
                moves.append(tok)
    if moves or headers:
        yield headers, moves


def convert(headers, sans, max_plies=None):
    """Return (uci_line, final_fen, error_or_None)."""
    fen = headers.get("FEN", START_FEN)
    pos = Position(fen)
    is_start = " ".join(fen.split()[:4]) == " ".join(START_FEN.split()[:4])
    uci, err = [], None
    for i, san in enumerate(sans):
        if max_plies is not None and i >= max_plies:
            break
        try:
            fr, to, promo = pos.parse_san(san)
        except ValueError as e:
            err = f"ply {i + 1}: {e}"
            break
        uci.append(sq_name(fr) + sq_name(to) + promo.lower())
        pos.push(fr, to, promo)
    head = "position startpos" if is_start else f"position fen {fen}"
    return head + (" moves " + " ".join(uci) if uci else ""), pos.fen(), err


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("pgn", help="PGN file, or - for stdin")
    ap.add_argument("-n", "--plies", type=int, default=None, help="limit number of plies")
    ap.add_argument("--final-fen", action="store_true",
                    help="also print the final position's FEN as a comment line")
    args = ap.parse_args()

    text = sys.stdin.read() if args.pgn == "-" else open(args.pgn, encoding="utf-8").read()
    n = 0
    for headers, sans in read_games(text):
        n += 1
        line, fen, err = convert(headers, sans, args.plies)
        if err:
            print(f"# game {n}: stopped at {err}", file=sys.stderr)
        print(line)
        if args.final_fen:
            print(f"# final fen: {fen}")
    if n == 0:
        sys.exit("No games found in input.")


if __name__ == "__main__":
    main()
