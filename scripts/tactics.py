#!/usr/bin/env python3
"""Score one or more builds on EPD test suites.

    scripts/tactics.py                            # working tree, the bundled suite
    scripts/tactics.py --ref HEAD~1 --ref worktree
    scripts/tactics.py --suite .tools/suites/bratko-kopec.epd --movetime 2000

Each position gets the same slice of clock and the engine either finds the move
or it does not, so this is a strength measurement under a game-like constraint
rather than a search-shape one. It is also a coarse one: a suite of 25 positions
resolves nothing smaller than a few percent, and a solved count that moved by
one or two is noise. Its value is that it is cheap and that a *drop* is
informative - an engine that used to see a tactic and now does not has usually
broken something, and the failing position tells you where to look.

A match is still the only thing that measures strength properly. Use
scripts/sprt.sh for the verdict and this for the fast, readable smoke test.

EPD lines look like:

    <fen> ; bm Nd5 a4 ; id BK.05

`bm` is the set of moves that count as solved, `am` the set that count as
failed. Moves may be written in algebraic notation (Nd5) or in the engine's own
UCI notation (b3d5); both are accepted, and the conversion between them is done
against the engine's own move generator so the suite can never disagree with it.
"""

import argparse
import concurrent.futures
import os
import re
import sys
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, ROOT)

# play.py already owns a careful UCI wrapper and the board helpers; sharing it
# means the runner cannot drift away from the engine the way a second copy would.
from play import Engine, EngineError, board_of  # noqa: E402

import subprocess  # noqa: E402

BUILD_REF = os.path.join(HERE, "build_ref.sh")
DEFAULT_SUITE = os.path.join(ROOT, "tests", "suites", "regression.epd")

FILES = "abcdefgh"


class SuiteError(RuntimeError):
    pass


# --------------------------------------------------------------------------
# EPD parsing
# --------------------------------------------------------------------------


def parse_epd(path):
    """Yield (fen, best, avoid, name) for every position in an EPD file."""
    cases = []
    # Published suites are old files that have been through a lot of tools:
    # expect stray NULs, CRs and the occasional non-UTF-8 byte, none of which
    # should stop a run.
    with open(path, errors="replace") as handle:
        for lineno, raw in enumerate(handle, 1):
            line = re.sub(r"[\x00-\x1f\x7f]", " ", raw).strip()
            if not line or line.startswith("#"):
                continue
            head, _, rest = line.partition(";")
            fields = head.split()
            if len(fields) < 4:
                raise SuiteError(f"{path}:{lineno}: not a FEN: {head.strip()!r}")
            # EPD proper stops after the en passant square; the halfmove and
            # fullmove counters are optional and some suites include them.
            defaults = ["", "", "", "", "0", "1"]
            fen = " ".join(fields[i] if i < len(fields) else defaults[i] for i in range(6))

            best, avoid, name = [], [], f"{os.path.basename(path)}:{lineno}"
            for opcode in rest.split(";"):
                opcode = opcode.strip()
                if opcode.startswith("bm "):
                    best = normalise_moves(opcode[3:])
                elif opcode.startswith("am "):
                    avoid = normalise_moves(opcode[3:])
                elif opcode.startswith("id "):
                    name = opcode[3:].strip().strip('"')
            if not best and not avoid:
                raise SuiteError(f"{path}:{lineno}: no bm or am, nothing to score")
            cases.append((fen, best, avoid, name))
    if not cases:
        raise SuiteError(f"{path}: no positions")
    return cases


def normalise_moves(text):
    """Split a bm/am operand into move tokens.

    Some published suites write "Qd1 +" with a stray space before the check
    mark, which would otherwise read as two moves.
    """
    return [move for move in re.sub(r"\s+([+#])", r"\1", text).split() if move not in "+#"]


# --------------------------------------------------------------------------
# Algebraic notation -> the engine's UCI move list
# --------------------------------------------------------------------------


def square_name(file_index, rank_index):
    return f"{FILES[file_index]}{rank_index + 1}"


def resolve(fen, san, legal):
    """Return the moves in `legal` (UCI strings) that `san` could name.

    Matching happens against the engine's own legal move list, so a move only
    resolves if the engine agrees it exists. Normally the answer is a single
    move; published suites contain the occasional under-disambiguated one like
    a bare "Ne5" with two knights able to reach e5, and one bad line should not
    end a run, so every candidate is returned and the caller is warned.
    """
    if san in legal:  # Already UCI
        return [san]

    text = san.rstrip("!?").rstrip("+#")

    if text in ("O-O", "0-0", "O-O-O", "0-0-0"):
        side = "g" if text in ("O-O", "0-0") else "c"
        rank = "1" if fen.split()[1] == "w" else "8"
        castles = [
            move
            for move in legal
            if move[2] == side
            and move[3] == rank
            and board_of(fen).get((FILES.index(move[0]), int(move[1]) - 1), "").upper() == "K"
        ]
        if not castles:
            raise SuiteError(f"no legal castling move for {san!r}")
        return castles

    promotion = ""
    if "=" in text:
        text, _, piece = text.partition("=")
        promotion = piece[:1].lower()

    piece = "P"
    if text and text[0] in "KQRBN":
        piece, text = text[0], text[1:]

    text = text.replace("x", "")
    if len(text) < 2:
        raise SuiteError(f"cannot read the move {san!r}")
    dest, hint = text[-2:], text[:-2]

    board = board_of(fen)
    white_to_move = fen.split()[1] == "w"
    wanted = piece.upper() if white_to_move else piece.lower()

    matches = []
    for move in legal:
        if move[:4][2:4] != dest:
            continue
        if (move[4:5] or "") != promotion:
            continue
        origin = (FILES.index(move[0]), int(move[1]) - 1)
        if board.get(origin) != wanted:
            continue
        if hint and not all(ch in move[:2] for ch in hint):
            continue
        matches.append(move)

    if not matches:
        raise SuiteError(f"{san!r} is not a legal move in {fen}")
    if len(matches) > 1:
        print(
            f"  note: {san!r} is ambiguous in this suite line, "
            f"counting any of {' '.join(matches)}",
            file=sys.stderr,
        )
    return matches


# --------------------------------------------------------------------------
# Running a suite
# --------------------------------------------------------------------------


class EnginePool:
    """One engine per worker thread. Engines are stateful, threads are not."""

    def __init__(self, path, hash_mb):
        self.path = path
        self.hash_mb = hash_mb
        self.local = threading.local()
        self.all = []
        self.lock = threading.Lock()

    def get(self):
        engine = getattr(self.local, "engine", None)
        if engine is None:
            engine = Engine(self.path, self.hash_mb, None)
            self.local.engine = engine
            with self.lock:
                self.all.append(engine)
        return engine

    def stop(self):
        for engine in self.all:
            engine.stop()


def run_suite(path, cases, movetime, concurrency, hash_mb, verbose):
    pool = EnginePool(path, hash_mb)

    def solve(case):
        fen, best, avoid, name = case
        engine = pool.get()
        legal = engine.legal_moves(fen, [])
        wanted = {uci for move in best for uci in resolve(fen, move, legal)}
        banned = {uci for move in avoid for uci in resolve(fen, move, legal)}
        # ucinewgame between positions, or a hit from the previous search's
        # table would make the result depend on the order of the file.
        engine.send("ucinewgame")
        played, info = engine.search(fen, [], movetime)
        ok = (not wanted or played in wanted) and played not in banned
        return name, ok, played, sorted(wanted), sorted(banned), info

    results = []
    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=concurrency) as pool_exec:
            for result in pool_exec.map(solve, cases):
                results.append(result)
                name, ok, played, wanted, banned, info = result
                if verbose or not ok:
                    expected = " ".join(wanted) or ("not " + " ".join(banned))
                    mark = "ok  " if ok else "FAIL"
                    print(
                        f"  {mark} {name:<24} played {played:<6} wanted {expected}"
                        f"   (depth {info.get('depth', '?')})",
                        file=sys.stderr,
                    )
    finally:
        pool.stop()
    return results


def build(ref):
    name = re.sub(r"[/~^: ]", "-", ref)
    proc = subprocess.run(
        [BUILD_REF, ref, f"tactics-{name}"], cwd=ROOT, stdout=subprocess.PIPE, text=True
    )
    if proc.returncode != 0:
        raise SuiteError(f"could not build {ref}")
    return proc.stdout.strip()


def main():
    parser = argparse.ArgumentParser(
        description="Score builds of the engine on EPD suites.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--suite", action="append", default=[], help="EPD file (repeatable)")
    parser.add_argument("--ref", action="append", default=[], help="git ref to build and score (repeatable)")
    parser.add_argument("--engine", action="append", default=[], help="engine binary to score (repeatable)")
    parser.add_argument("--movetime", type=int, default=1000, help="ms per position")
    parser.add_argument("--hash", type=int, default=16, help="hash in MB")
    parser.add_argument("--concurrency", type=int, default=max(1, (os.cpu_count() or 4) - 2))
    parser.add_argument("-v", "--verbose", action="store_true", help="print solved positions too")
    args = parser.parse_args()

    suites = args.suite or [DEFAULT_SUITE]
    for suite in suites:
        if not os.path.exists(suite):
            print(f"error: no suite at {suite}", file=sys.stderr)
            if ".tools" in suite:
                print("run scripts/setup_tools.sh to fetch the published suites", file=sys.stderr)
            return 1

    try:
        engines = [(ref, build(ref)) for ref in args.ref]
        engines += [(os.path.basename(path), path) for path in args.engine]
        if not engines:
            engines = [("worktree", build("worktree"))]
    except SuiteError as err:
        print(f"error: {err}", file=sys.stderr)
        return 1

    print(f"\n  {args.movetime} ms per position, {args.concurrency} at a time\n")

    table = {}
    for suite in suites:
        try:
            cases = parse_epd(suite)
        except SuiteError as err:
            print(f"error: {err}", file=sys.stderr)
            return 1
        for label, path in engines:
            print(f"== {os.path.basename(suite)}  ({len(cases)} positions)  {label} ==", file=sys.stderr)
            try:
                results = run_suite(path, cases, args.movetime, args.concurrency, args.hash, args.verbose)
            except (SuiteError, EngineError) as err:
                print(f"error: {err}", file=sys.stderr)
                return 1
            table[(suite, label)] = sum(1 for _, ok, *_ in results if ok)
            print(file=sys.stderr)

    width = max(len(label) for label, _ in engines) + 2
    print(f"  {'suite':<28}{''.join(f'{label:>{width}}' for label, _ in engines)}")
    for suite in suites:
        name = os.path.basename(suite)
        total = len(parse_epd(suite))
        scores = "".join(f"{f'{table[(suite, label)]}/{total}':>{width}}" for label, _ in engines)
        print(f"  {name:<28}{scores}")
    print("\n  A suite this small only shows large changes. Use scripts/sprt.sh for strength.\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
