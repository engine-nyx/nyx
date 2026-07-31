#!/usr/bin/env python3
import re
import subprocess
import sys
import os

MOVE_RE = re.compile(r'^([a-h][1-8][a-h][1-8][nbrq]?):\s*(\d+)$', re.IGNORECASE)

def run_perft(engine_path, fen, depth, moves):
    pos_cmd = f"position fen {fen}" if fen else "position startpos"
    if moves:
        pos_cmd += " moves " + " ".join(moves)

    proc = subprocess.Popen(
        [engine_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    out, _ = proc.communicate(f"uci\nisready\n{pos_cmd}\ngo perft {depth}\nquit\n")

    result = {}
    for line in out.splitlines():
        m = MOVE_RE.match(line.strip())
        if m:
            result[m.group(1).lower()] = int(m.group(2))

    if not result:
        print(f"Warning: no perft output from {engine_path} for {pos_cmd} depth {depth}", file=sys.stderr)
        print(out, file=sys.stderr)

    return result


def main():
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} <engine> <oracle> <depth> [fen]")
        sys.exit(1)

    engine_path = sys.argv[1]
    oracle_path = sys.argv[2]
    depth = int(sys.argv[3])
    fen = sys.argv[4] if len(sys.argv) > 4 else None

    for p, label in [(engine_path, "engine"), (oracle_path, "oracle")]:
        if not os.path.isfile(p):
            print(f"Error: {label} not found: {p}")
            sys.exit(1)

    display_fen = fen or "startpos"
    print(f"engine: {engine_path}")
    print(f"oracle:  {oracle_path}")
    print(f"depth:   {depth}")
    print(f"fen:     {display_fen}")
    print()

    movelist = []

    while depth > 0:
        sys.stdout.write(f"\rdepth {depth} | moves: {' '.join(movelist) or '(none)'}")
        sys.stdout.flush()

        eng = run_perft(engine_path, fen, depth, movelist)
        ora = run_perft(oracle_path, fen, depth, movelist)

        eng_only = sorted(set(eng) - set(ora))
        ora_only = sorted(set(ora) - set(eng))

        if eng_only:
            print(f"\n\nMove(s) only in engine (not in oracle): {', '.join(eng_only)}")
            print(f"After: {' '.join(movelist) or '(initial position)'}")
            sys.exit(1)
        if ora_only:
            print(f"\n\nMove(s) only in oracle (not in engine): {', '.join(ora_only)}")
            print(f"After: {' '.join(movelist) or '(initial position)'}")
            sys.exit(1)

        diffs = [(m, eng[m], ora[m]) for m in sorted(eng) if eng[m] != ora[m]]

        if not diffs:
            print(f"\n\nAll perft results match. No divergence found.")
            sys.exit(0)

        move, ec, oc = diffs[0]
        print(f"\n  {move}: engine={ec}, oracle={oc}")
        movelist.append(move)
        depth -= 1

    print(f"\nDiverging line: {' '.join(movelist)}")


if __name__ == "__main__":
    main()
