#!/usr/bin/env bash
# Play a new build against an old one until the result is statistically settled.
#
#     scripts/sprt.sh                     # working tree vs HEAD
#     scripts/sprt.sh HEAD~1              # working tree vs HEAD~1
#     scripts/sprt.sh master my-branch    # <base> <new>, any two refs
#
# Knobs, all optional:
#     TC=8+0.08       time control, seconds + increment, per game per side
#     ELO0=0 ELO1=5   the hypotheses: "no gain" against "at least 5 elo"
#     HASH=16         hash per engine, in MB
#     CONCURRENCY=6   games in flight; leave a core or two free or the clock lies
#     ROUNDS=20000    give up after this many openings even if undecided
#     BOOK=path.epd   a different opening book
#
# What the test actually does: after every game it asks whether the evidence so
# far favours "the change is worth at least ELO1 elo" or "the change is worth
# nothing", and stops as soon as one of them is clearly ahead. That is why it
# has no fixed length. A clear gain can settle in a few hundred games; a change
# worth about ELO1 exactly can run for tens of thousands, because there is
# genuinely nothing to conclude.
#
# Read the ending line. "H1 accepted" means the change is good, "H0 accepted"
# means it is not (which includes "not good enough to keep the complexity").
# The elo number printed alongside is an estimate with a real error bar, so a
# +5 with +/- 12 next to it has not established anything.
#
# Ctrl-C is safe: fastchess prints the standings so far before exiting.
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
TOOLS="$ROOT/.tools"
FASTCHESS="$TOOLS/fastchess/fastchess"
BOOK=${BOOK:-$TOOLS/UHO_4060_v2.epd}

TC=${TC:-8+0.08}
ELO0=${ELO0:-0}
ELO1=${ELO1:-5}
HASH=${HASH:-16}
ROUNDS=${ROUNDS:-20000}
# Two engines share a game but only one thinks at a time, so cores, not
# processes, is the limit. Leaving two free keeps the timing honest.
CONCURRENCY=${CONCURRENCY:-$(( $(nproc 2>/dev/null || echo 4) - 2 ))}
[ "$CONCURRENCY" -ge 1 ] || CONCURRENCY=1

case $# in
    0) BASE=HEAD; NEW=worktree ;;
    1) BASE=$1;   NEW=worktree ;;
    2) BASE=$1;   NEW=$2 ;;
    *) echo "usage: $(basename "$0") [<base> [<new>]]" >&2; exit 2 ;;
esac

[ -x "$FASTCHESS" ] || { echo "no fastchess - run scripts/setup_tools.sh first" >&2; exit 1; }
[ -s "$BOOK" ]      || { echo "no opening book at $BOOK - run scripts/setup_tools.sh first" >&2; exit 1; }

BASE_ENGINE=$("$ROOT/scripts/build_ref.sh" "$BASE" "sprt-base")
NEW_ENGINE=$("$ROOT/scripts/build_ref.sh" "$NEW" "sprt-new")

mkdir -p "$ROOT/games"
PGN="$ROOT/games/sprt-$(echo "${BASE}-vs-${NEW}" | tr '/~^: ' '-----')-$(date +%Y%m%d-%H%M%S).pgn"

cat >&2 <<EOF

  base        $BASE
  new         $NEW
  tc          $TC
  hypotheses  H0: <= $ELO0 elo   H1: >= $ELO1 elo
  concurrency $CONCURRENCY
  book        $(basename "$BOOK")
  games       $PGN

EOF

# -repeat plus -games 2 plays every opening twice with the colours swapped, so
# neither engine gets the good side of an unbalanced position more often.
# The adjudication rules end games that are already decided or already drawn,
# which is most of the saved time in a run this long.
exec "$FASTCHESS" \
    -engine cmd="$NEW_ENGINE" name=new \
    -engine cmd="$BASE_ENGINE" name=base \
    -each tc="$TC" option.Hash="$HASH" proto=uci \
    -openings file="$BOOK" format=epd order=random \
    -repeat -games 2 -rounds "$ROUNDS" \
    -concurrency "$CONCURRENCY" \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha=0.05 beta=0.05 model=normalized \
    -resign movecount=3 score=400 \
    -draw movenumber=40 movecount=8 score=10 \
    -pgnout file="$PGN" \
    -recover
