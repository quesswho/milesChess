#include "Perft.h"
#include "MoveGen.h"

#include <cinttypes>
#include <cstdio>

template<PerftGen G>
uint64 Perft(Position& pos, int depth) {
    if (depth == 0) return 1;

    uint64 count = 0;
    if constexpr (G == PerftGen::Staged) {
        MoveGen gen(pos, 0, false);
        Move move;
        while ((move = gen.Next()) != 0) {
            if (depth == 1) {
                count++;
                continue;
            }
            pos.MovePiece(move);
            count += Perft<G>(pos, depth - 1);
            pos.UndoMove(move);
        }
    } else {
        std::vector<Move> moves = GenerateMoves<ALL>(pos);
        if (depth == 1) return moves.size();
        for (Move move : moves) {
            pos.MovePiece(move);
            count += Perft<G>(pos, depth - 1);
            pos.UndoMove(move);
        }
    }
    return count;
}

template uint64 Perft<PerftGen::Staged>(Position&, int);
template uint64 Perft<PerftGen::Bulk>(Position&, int);

uint64 PerftDivide(Position& pos, int depth, bool print) {
    if (depth <= 0) return 1;

    uint64 total = 0;
    MoveGen gen(pos, 0, false);
    Move move;
    while ((move = gen.Next()) != 0) {
        pos.MovePiece(move);
        uint64 count = Perft<PerftGen::Staged>(pos, depth - 1);
        pos.UndoMove(move);
        total += count;
        if (print) printf("%s: %" PRIu64 "\n", MoveToString(move).c_str(), count);
    }
    if (print) printf("\n%" PRIu64 "\n", total);
    return total;
}
