#include "TestUtil.h"
#include "Positions.h"

#include "Perft.h"
#include "MoveGen.h"

#include <algorithm>
#include <vector>

static void RunTable(const PerftCase* cases, size_t count) {
    for (size_t i = 0; i < count; i++) {
        const PerftCase& c = cases[i];
        Position pos;
        pos.SetPosition(c.fen);
        uint64 staged = Perft<PerftGen::Staged>(pos, c.depth);

        pos.SetPosition(c.fen);
        uint64 bulk = Perft<PerftGen::Bulk>(pos, c.depth);

        if (staged != c.nodes || bulk != c.nodes) {
            printf("  %s depth %d: staged=%" PRIu64 " bulk=%" PRIu64 " expected=%" PRIu64 "\n", c.name, c.depth, staged,
                   bulk, c.nodes);
        }
        CHECK_EQ(staged, c.nodes);
        CHECK_EQ(bulk, c.nodes);
        CHECK_EQ(staged, bulk);
    }
}

static std::vector<Move> StagedMoves(Position& pos) {
    std::vector<Move> moves;
    MoveGen gen(pos, 0, false);
    Move move;
    while ((move = gen.Next()) != 0) moves.push_back(move);
    return moves;
}

static void CompareMoveSets(Position& pos, int depth, const char* name) {
    std::vector<Move> staged = StagedMoves(pos);
    std::vector<Move> bulk = GenerateMoves<ALL>(pos);

    std::vector<Move> a = staged, b = bulk;
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());

    if (a != b) {
        printf("  move set mismatch at %s (depth %d), fen %s\n", name, depth, pos.ToFen().c_str());
        printf("    staged (%zu): ", staged.size());
        for (Move m : a) printf("%s ", MoveToString(m).c_str());
        printf("\n    bulk   (%zu): ", bulk.size());
        for (Move m : b) printf("%s ", MoveToString(m).c_str());
        printf("\n");
    }
    CHECK(a == b);

    if (depth <= 1) return;
    for (Move move : bulk) {
        pos.MovePiece(move);
        CompareMoveSets(pos, depth - 1, name);
        pos.UndoMove(move);
    }
}

int main() {
    printf("-- perft node counts (staged vs bulk vs reference)\n");
    RunTable(kPerftFast, sizeof(kPerftFast) / sizeof(kPerftFast[0]));

    printf("-- move set equality to depth 3\n");
    const char* const fens[] = { kStartPos, kKiwipete, kEndgame, kPromo, kMidgame, kComplex };
    const char* const names[] = { "startpos", "kiwipete", "endgame", "promo", "midgame", "complex" };
    for (size_t i = 0; i < 6; i++) {
        Position pos;
        pos.SetPosition(fens[i]);
        CompareMoveSets(pos, 3, names[i]);
    }

    return TestSummary("test_perft");
}
