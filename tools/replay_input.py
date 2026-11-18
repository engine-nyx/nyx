#!/usr/bin/env python3
"""Feed a UCI command file to the engine, waiting for `bestmove` after each `go`.

By default this replays the `input` file in the repository root through
`make run`.  Unlike `make run < input`, the next `position`/`go` command is
only sent once the engine has finished searching and printed `bestmove`.

Usage:
    tools/replay_input.py [FILE] [--timeout SECONDS] [-- COMMAND...]

Examples:
    tools/replay_input.py
    tools/replay_input.py input -- make run
    tools/replay_input.py input --timeout 120 -- ./build/release/nyx
"""

from __future__ import annotations

import argparse
import queue
import subprocess
import sys
import threading
import time


def reader(stream, out: queue.Queue[str | None]) -> None:
    """Forward engine output to a queue, using None as an EOF marker."""
    try:
        for line in stream:
            out.put(line.rstrip("\n"))
    finally:
        out.put(None)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Replay UCI commands, waiting for bestmove after each go."
    )
    parser.add_argument("input", nargs="?", default="input", help="command file")
    parser.add_argument(
        "--timeout",
        type=float,
        default=None,
        metavar="SECONDS",
        help="max time to wait for a bestmove (default: wait forever)",
    )
    parser.add_argument(
        "command",
        nargs="*",
        help="command used to start the engine (default: make run)",
    )
    args, command = parser.parse_known_args()

    command = [part for part in command if part != "--"]
    if not command:
        command = ["make", "run"]

    try:
        with open(args.input, encoding="utf-8") as file:
            commands = [line.strip() for line in file if line.strip()]
    except OSError as error:
        print(f"replay_input: {error}", file=sys.stderr)
        return 1

    proc = subprocess.Popen(
        command,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    assert proc.stdin is not None and proc.stdout is not None

    output: queue.Queue[str | None] = queue.Queue()
    output_thread = threading.Thread(target=reader, args=(proc.stdout, output), daemon=True)
    output_thread.start()

    def wait_for_bestmove(deadline: float | None) -> None:
        while True:
            timeout = None if deadline is None else max(0.0, deadline - time.monotonic())
            try:
                line = output.get(timeout=timeout)
            except queue.Empty:
                raise TimeoutError("timed out waiting for bestmove")
            if line is None:
                raise EOFError("engine exited before sending bestmove")
            print(line)
            if line.strip().startswith("bestmove"):
                return

    try:
        for command_line in commands:
            print(f"> {command_line}")
            proc.stdin.write(command_line + "\n")
            proc.stdin.flush()

            if command_line.split()[0] == "go":
                deadline = (
                    None
                    if args.timeout is None
                    else time.monotonic() + args.timeout
                )
                wait_for_bestmove(deadline)
    except (BrokenPipeError, TimeoutError, EOFError) as error:
        print(f"replay_input: {error}", file=sys.stderr)
        proc.kill()
        proc.wait()
        return 1
    finally:
        try:
            proc.stdin.close()
        except BrokenPipeError:
            pass

    try:
        proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()

    # Drain any output the engine produced after the last bestmove.
    output_thread.join(timeout=1)
    while True:
        try:
            line = output.get_nowait()
        except queue.Empty:
            break
        if line is None:
            break
        print(line)

    return proc.returncode or 0


if __name__ == "__main__":
    raise SystemExit(main())
