#!/usr/bin/env bash
# One time setup for the match harness: fetch and build fastchess, and fetch the
# opening book the matches start from. Everything lands in .tools/, which is
# gitignored - none of it belongs in the repo.
#
#     scripts/setup_tools.sh            # both, skipping whatever is already there
#     scripts/setup_tools.sh --force    # redo it all
#
# fastchess is the referee. It runs two engines against each other, enforces the
# clock, adjudicates, and does the statistics: it is what turns "the new build
# won 412 games" into "+22 elo, and here is the error bar".
#
# The book matters as much as the referee. From the start position two similar
# engines draw most of their games, and draws carry almost no information about
# which one is stronger. UHO ("unbalanced human openings") starts every game
# from a position one side already prefers, played from both sides, so games end
# decisively and a given number of them says much more.
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
TOOLS="$ROOT/.tools"
BOOK="$TOOLS/UHO_4060_v2.epd"
FASTCHESS="$TOOLS/fastchess/fastchess"
FORCE=${1:-}

mkdir -p "$TOOLS"

need() {
    command -v "$1" >/dev/null 2>&1 || { echo "missing required tool: $1" >&2; exit 1; }
}
need git
need make
need curl
need unzip

if [ "$FORCE" = "--force" ] || [ ! -x "$FASTCHESS" ]; then
    echo "==> building fastchess"
    rm -rf "$TOOLS/fastchess"
    git clone --depth 1 https://github.com/Disservin/fastchess.git "$TOOLS/fastchess"
    make -C "$TOOLS/fastchess" -j"$(nproc 2>/dev/null || echo 4)"
else
    echo "==> fastchess already built ($FASTCHESS)"
fi

if [ "$FORCE" = "--force" ] || [ ! -s "$BOOK" ]; then
    echo "==> fetching the UHO opening book"
    curl -fsSL -o "$TOOLS/book.zip" \
        https://github.com/official-stockfish/books/raw/master/UHO_4060_v2.epd.zip
    unzip -oq "$TOOLS/book.zip" -d "$TOOLS"
    rm -f "$TOOLS/book.zip"
    [ -s "$BOOK" ] || { echo "the book archive did not contain $(basename "$BOOK")" >&2; exit 1; }
else
    echo "==> book already present ($BOOK)"
fi

SUITES="$TOOLS/suites"
mkdir -p "$SUITES"
# Published test suites for scripts/tactics.py. bratko-kopec and kaufman are the
# two classics, small and mostly positional; silent-but-deadly is large and
# quiet, which makes it a better check on evaluation than on tactics.
for suite in bratko-kopec kaufman silent-but-deadly; do
    if [ "$FORCE" = "--force" ] || [ ! -s "$SUITES/$suite.epd" ]; then
        echo "==> fetching $suite.epd"
        curl -fsSL -o "$SUITES/$suite.epd" \
            "https://raw.githubusercontent.com/ChrisWhittington/Chess-EPDs/master/$suite.epd"
    fi
done

echo
echo "ready:"
echo "  referee : $FASTCHESS"
echo "  book    : $BOOK ($(wc -l <"$BOOK") positions)"
echo "  suites  : $SUITES"
echo
echo "next: scripts/sprt.sh HEAD~1"
