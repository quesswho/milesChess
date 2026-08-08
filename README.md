# MilesBot
Simple C++ Chess engine written by Sebastian Miles.

## Playing against it

```sh
make play
```

That builds the engine and opens a board at <http://localhost:8080>. Drag or
click pieces to move; the engine answers with its depth, score and principal
variation next to the board. `play.py` only needs python3 from the standard
library — it drives `./build/engine` over UCI and asks the engine itself for
the legal moves (`perft 1`) and the current position (`d`), so the board never
disagrees with the move generator.

Useful flags (`make play PLAY_ARGS="..."`, or run `./play.py` directly):

| Flag | Effect |
| --- | --- |
| `--host 0.0.0.0` | let others on the LAN join — everyone shares one board, so a group can play the engine together |
| `--port 9000` | serve somewhere else |
| `--hash 256` | engine hash in MB |
| `--syzygy tb/` | point the engine at tablebases |

The panel also takes a FEN, which makes it easy to spar from a position the
engine got wrong. Thinking time per move is a dropdown, from 0.2 s up to 30 s.

To use a normal chess GUI instead (CuteChess, Arena, BanksiaGUI), add
`./build/engine` as a UCI engine — it needs no arguments.

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