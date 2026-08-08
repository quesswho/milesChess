# MilesBot
Simple C++ Chess engine written by Sebastian Miles.

To play against it:
```sh
make play
```

That builds the engine and opens a board at <http://localhost:8080>. 

The engine supports UCI so it can be used on normal chess GUI instead (CuteChess, Arena, BanksiaGUI), add
`./build/engine` as a UCI engine

## Measuring a change

Four tools that compare two builds. Each takes a baseline and a challenger,
either of which can be any git ref; the default is the working tree against
HEAD. Baselines are built in a temporary git worktree, so uncommitted work is
untouched.

| Tool | Cost | Measures |
| --- | --- | --- |
| `make bench` | ~10 s | depth reached in a fixed slice of clock |
| `make benchcmp` | ~1 min | depth, tree size and speed, two builds side by side |
| `make tactics` | ~1 min | positions solved from an EPD suite |
| `make sprt` | hours | elo, from games |

`make setup-tools` once first for `sprt` and the published suites: it builds
[fastchess](https://github.com/Disservin/fastchess) and fetches the UHO opening
book and several EPD suites into `.tools/`.

### bench

28 fixed positions, same time for each, scored on total depth reached.

```sh
make bench                          # 400 ms per position
make bench BENCH_ARGS="time 1000"
make bench BENCH_ARGS="depth 7"     # fixed depth instead, reproducible node count
```

Depth mode's node count is exact: identical numbers mean the search walked an
identical tree. It cannot measure strength, since a slower engine scores the
same.

### benchcmp

Runs both bench modes on both builds. Time mode is repeated and interleaved
between the builds, and the median is reported.

```sh
make benchcmp                       # working tree vs HEAD
make benchcmp REF=HEAD~1
python3 scripts/benchcmp.py master my-branch --repeat 5
```

| Time mode | Depth mode | Reading |
| --- | --- | --- |
| more depth | same nodes | speedup |
| more depth | fewer nodes | better pruning |
| same depth | fewer nodes | pruning gained what speed lost |
| less depth | same nodes | evaluation or movegen got more expensive |

Only refs from this commit onwards have a `bench` command; older ones are
reported as such rather than compared.

### tactics

Each position of an EPD suite gets the same movetime; counts the ones solved.
`tests/suites/regression.epd` mirrors `tests/Puzzles.h` and needs no setup.
Moves may be written in algebraic or UCI notation, resolved against the
engine's own move generator.

```sh
make tactics
python3 scripts/tactics.py --suite .tools/suites/bratko-kopec.epd --movetime 2000
python3 scripts/tactics.py --ref HEAD~1 --ref worktree
```

### sprt

Plays the two builds against each other and stops when the result is
statistically settled. Ctrl-C prints the standings
so far. Games are written to `games/`, which is gitignored.

```sh
make sprt REF=HEAD~1
TC=20+0.2 make sprt REF=master       # slower games
ELO1=15 make sprt REF=HEAD~1         # only test for a large gain
```

Knobs, all environment variables: `TC`, `ELO0`, `ELO1`, `HASH`, `CONCURRENCY`,
`ROUNDS`, `BOOK`.

The last line is the result. `H1 accepted` means keep the change, `H0 accepted`
means do not. The elo figure comes with an error bar.

## Move Generation
* Staged move generation
* MVV-LVA move ordering

## Search

* Iterative Deepening
* Transposition Table
* Aspiration windows
* Quiescence Search
* PVS Search
* Check extensions
* Singular Extensions
* Multi-Cut Probing
 

## Evaluation
* Material
* Piece Square table
* Pawn structure
* 5-man Tablebase
