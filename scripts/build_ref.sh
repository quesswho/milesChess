#!/usr/bin/env bash
# Build the engine as it existed at some git ref, without touching the worktree.
#
#     scripts/build_ref.sh HEAD~1          -> build/engines/HEAD~1
#     scripts/build_ref.sh v0.3 baseline   -> build/engines/baseline
#
# The ref is checked out into a temporary git worktree and built there, so
# uncommitted changes stay put and the build below build/ is never disturbed.
# "worktree" is accepted as a ref and means "whatever is in the working tree
# right now", which is how you benchmark an unfinished change.
#
# Prints the path of the finished binary on stdout; everything else goes to
# stderr, so callers can do ENGINE=$(scripts/build_ref.sh HEAD~1).
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUTDIR="$ROOT/build/engines"

usage() {
    echo "usage: $(basename "$0") <ref|worktree> [name]" >&2
    exit 2
}

[ $# -ge 1 ] || usage
REF=$1
# `/` and `~` are legal in refs but not in filenames, so HEAD~1 becomes HEAD-1.
NAME=${2:-$(echo "$REF" | tr '/~^: ' '-----')}
DEST="$OUTDIR/$NAME"

mkdir -p "$OUTDIR"

# A binary named after a moving ref is a trap: build/engines/HEAD would quietly
# stay at whatever HEAD meant last week. Always rebuild rather than cache.
if [ "$REF" = "worktree" ]; then
    echo "building the working tree -> $DEST" >&2
    SRC="$ROOT"
    BUILD="$ROOT/build" # Shares the incremental build with `make engine`
else
    git -C "$ROOT" rev-parse --verify --quiet "$REF^{commit}" >/dev/null \
        || { echo "no such commit: $REF" >&2; exit 1; }
    SHA=$(git -C "$ROOT" rev-parse --short "$REF")
    echo "building $REF ($SHA) -> $DEST" >&2

    SRC=$(mktemp -d "${TMPDIR:-/tmp}/milesChess-$NAME-XXXXXX")
    trap 'git -C "$ROOT" worktree remove --force "$SRC" >/dev/null 2>&1 || rm -rf "$SRC"' EXIT
    git -C "$ROOT" worktree add --detach --quiet "$SRC" "$REF"
    BUILD="$SRC/build"
fi

# -DMILESCHESS_NATIVE is left at its default: both sides of a comparison are
# built the same way on the same machine, which is what makes them comparable.
cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release >&2
cmake --build "$BUILD" --target engine -j >&2

cp "$BUILD/engine" "$DEST"
echo "$DEST"
