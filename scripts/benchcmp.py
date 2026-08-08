#!/usr/bin/env python3
"""Bench two builds of the engine against each other and print the difference.

    scripts/benchcmp.py                    # working tree vs HEAD
    scripts/benchcmp.py HEAD~1             # working tree vs HEAD~1
    scripts/benchcmp.py master my-branch   # <base> <new>, any two refs

Two measurements, because one number cannot answer the question on its own.

*Time mode* is the headline. Every position gets the same slice of clock and
the score is the summed depth reached, so it measures the engine under the
constraint it actually plays under: an evaluation that got twice as slow to
prune slightly better loses depth here, which is the truth about it. It is the
noisy one, so the runs are repeated and interleaved with the baseline and the
median is reported.

*Depth mode* then says what kind of change it was, because at a fixed depth the
search walks a reproducible tree:

    nodes  changed -> the search itself changed: pruning, ordering, extensions
    nodes  same    -> the tree is identical and only speed moved
    nps    changed -> the cost per node changed: evaluation, move generation

Neither mode settles whether a change is worth keeping. Only a game match at a
real time control does that; see scripts/sprt.sh. This is the cheap signal you
run on every change, not the verdict.
"""

import argparse
import os
import re
import statistics
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
BUILD_REF = os.path.join(HERE, "build_ref.sh")

# The last line of a bench run: bench: version=1 mode=time limit=400 depth=304 ...
BENCH_LINE = re.compile(r"^bench: (.*)$", re.M)


class BenchError(RuntimeError):
    pass


def build(ref, name):
    """Build `ref` into build/engines/<name> and return the binary path."""
    proc = subprocess.run(
        [BUILD_REF, ref, name], cwd=ROOT, stdout=subprocess.PIPE, text=True
    )
    if proc.returncode != 0:
        raise BenchError(f"could not build {ref}")
    return proc.stdout.strip()


def run_bench(engine, mode, limit, label):
    """Run one bench and return its trailing key=value fields as a dict."""
    proc = subprocess.run(
        [engine, "bench", mode, str(limit)],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    if proc.returncode != 0:
        raise BenchError(f"{label} exited with {proc.returncode}")
    match = BENCH_LINE.search(proc.stdout)
    if not match:
        # An older build has no bench command and just waits for UCI input,
        # which subprocess sees as a clean exit with nothing useful on stdout.
        raise BenchError(f"{label} has no `bench` command (it predates src/Bench.h)")
    fields = {}
    for pair in match.group(1).split():
        key, _, value = pair.partition("=")
        fields[key] = int(value) if value.lstrip("-").isdigit() else value
    return fields


def pct(new, base):
    return f"{(new - base) * 100.0 / base:+.1f}%" if base else "n/a"


def row(label, base, new, better=None, noise=0.0):
    """One comparison line.

    `better` is the sign of a change that counts as an improvement. `noise` is
    how large a difference has to be, in percent, before it is worth calling
    one at all: run to run these numbers move a little on their own, and
    labelling that drift as a regression is how you end up chasing nothing.
    """
    note = ""
    if better and base:
        change = (new - base) * 100.0 / base
        if abs(change) < noise:
            note = "  same"
        else:
            note = "  better" if change * better > 0 else "  worse"
    return f"  {label:<22}{base:>14,}{new:>14,}{pct(new, base):>10}{note}"


def main():
    parser = argparse.ArgumentParser(
        description="Compare the bench of two builds.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("refs", nargs="*", help="[base [new]]; default: HEAD worktree")
    parser.add_argument("--time-ms", type=int, default=400, help="ms per position in time mode")
    parser.add_argument("--repeat", type=int, default=3, help="time mode runs per build")
    parser.add_argument("--depth", type=int, default=7, help="plies in depth mode")
    parser.add_argument("--skip-depth", action="store_true", help="only run time mode")
    parser.add_argument("--skip-time", action="store_true", help="only run depth mode")
    args = parser.parse_args()

    if len(args.refs) == 0:
        base_ref, new_ref = "HEAD", "worktree"
    elif len(args.refs) == 1:
        base_ref, new_ref = args.refs[0], "worktree"
    elif len(args.refs) == 2:
        base_ref, new_ref = args.refs
    else:
        parser.error("expected at most two refs")

    try:
        base = build(base_ref, "benchcmp-base")
        new = build(new_ref, "benchcmp-new")
    except BenchError as err:
        print(f"error: {err}", file=sys.stderr)
        return 1

    print(f"\n  base  {base_ref}\n  new   {new_ref}")
    header = f"  {'':<22}{base_ref[:13]:>14}{new_ref[:13]:>14}{'diff':>10}"

    try:
        if not args.skip_time:
            print(
                f"\n== time mode: {args.time_ms} ms per position, "
                f"median of {args.repeat} ==\n"
            )
            print(header, flush=True)
            # Interleaved, so a machine that warms up or gets busy midway
            # through drags both builds the same way instead of only one.
            runs = {"base": [], "new": []}
            for i in range(args.repeat):
                for label, engine, ref in (("base", base, base_ref), ("new", new, new_ref)):
                    result = run_bench(engine, "time", args.time_ms, ref)
                    runs[label].append(result)
                    print(
                        f"    run {i + 1}  {label:<4}  "
                        f"depth {result['depth']}  nps {result['nps']:,}",
                        file=sys.stderr,
                    )
            print(file=sys.stderr)

            def median(label, key):
                return int(statistics.median(r[key] for r in runs[label]))

            # A change under a percent of summed depth is within the spread of
            # repeated runs of one unchanged build, and nps wanders further.
            print(row("summed depth", median("base", "depth"), median("new", "depth"), +1, noise=1.0))
            print(row("nodes", median("base", "nodes"), median("new", "nodes")))
            # Deliberately unjudged: in time mode both builds search different
            # trees, so nps mixes speed with what kind of nodes they happened to
            # land on. Depth mode below is where nps means only speed.
            print(row("nps", median("base", "nps"), median("new", "nps")))
            print(
                "\n  Summed depth is the number that matters here: more depth in the"
                "\n  same clock is a stronger engine, whatever the nodes did."
            )

        if not args.skip_depth:
            print(f"\n== depth mode: {args.depth} plies, reproducible ==\n")
            print(header, flush=True)
            base_d = run_bench(base, "depth", args.depth, base_ref)
            new_d = run_bench(new, "depth", args.depth, new_ref)
            print(row("nodes", base_d["nodes"], new_d["nodes"]))
            print(row("nps", base_d["nps"], new_d["nps"], +1, noise=3.0))
            if base_d["nodes"] == new_d["nodes"]:
                print("\n  Identical node counts: the search walks the same tree as before,")
                print("  so any change above is pure speed.")
            else:
                print("\n  The node counts differ, so the search itself changed. Fewer nodes")
                print("  is a better-pruned tree, not necessarily a stronger engine.")
    except BenchError as err:
        print(f"error: {err}", file=sys.stderr)
        return 1

    print("\n  Neither number is a verdict. Run scripts/sprt.sh for that.\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
