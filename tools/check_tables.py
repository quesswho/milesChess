#!/usr/bin/env python3
"""Verify that the constexpr tables committed in src/LookupTables.h still match
what tools/gen_tables.cpp produces.

The tables in LookupTables.h are generator output that was pasted into the
header by hand. This script closes that loop: it runs each generator and
compares the values against the committed array, so a stale paste is a build
failure instead of a silent wrong-move bug.

Usage: check_tables.py <path-to-gen_tables-binary> [header]
"""

import re
import subprocess
import sys

# generator subcommand -> array name in LookupTables.h
TABLES = {
    "line": "lines",
    "knight": "knight_attacks",
    "king": "king_attacks",
    "king-safety-white": "w_king_safety",
    "king-safety-black": "b_king_safety",
    "rook": "rook_mask",
    "bishop": "bishop_mask",
    "check-moves": "check_mask",
    "active-moves": "active_moves",
    "passed-pawn-white": "white_passed",
    "passed-pawn-black": "black_passed",
    "forward-pawn-white": "white_forward",
    "forward-pawn-black": "black_forward",
    "isolated-pawn": "isolated_mask",
    "zobrist": "zobrist",
}

TOKEN = re.compile(r"\b(?:0[xX][0-9a-fA-F]+|\d+)(?:ull|ULL|ll|LL|u|U)?\b")


def values(text):
    """Parse an integer token list, normalising hex/octal/suffix spelling."""
    out = []
    for tok in TOKEN.findall(text):
        tok = re.sub(r"(?:ull|ULL|ll|LL|u|U)$", "", tok)
        # The header spells some zeros as 000000000000000000; int(x, 0) rejects
        # that as an invalid octal literal, so treat bare digits as decimal.
        out.append(int(tok, 16) if tok.lower().startswith("0x") else int(tok.lstrip("0") or "0"))
    return out


def committed(header, name):
    """Extract the initialiser body of `name` from the header."""
    m = re.search(r"\b" + re.escape(name) + r"\s*(?:\[[^\]]*\])?\s*=\s*\{", header)
    if not m:
        return None
    start = m.end()
    end = header.index("};", start)
    return values(header[start:end])


def main():
    if len(sys.argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    binary = sys.argv[1]
    path = sys.argv[2] if len(sys.argv) > 2 else "src/LookupTables.h"
    with open(path) as f:
        header = f.read()

    failed = []
    for cmd, name in TABLES.items():
        want = committed(header, name)
        if want is None:
            failed.append(f"{name}: not found in {path}")
            continue
        got = values(subprocess.run([binary, cmd], capture_output=True, text=True, check=True).stdout)
        if got == want:
            print(f"  ok   {cmd} -> {name} ({len(got)} entries)")
            continue
        if len(got) != len(want):
            failed.append(f"{name}: generated {len(got)} entries, header has {len(want)}")
            continue
        bad = [i for i, (g, w) in enumerate(zip(got, want)) if g != w]
        detail = ", ".join(f"[{i}] header={want[i]:#x} generated={got[i]:#x}" for i in bad[:5])
        failed.append(f"{name}: {len(bad)} of {len(want)} entries differ: {detail}")
        print(f"  FAIL {cmd} -> {name}")

    if failed:
        print("\n" + "\n".join(failed), file=sys.stderr)
        return 1
    print(f"\nall {len(TABLES)} tables match src/LookupTables.h")
    return 0


if __name__ == "__main__":
    sys.exit(main())
