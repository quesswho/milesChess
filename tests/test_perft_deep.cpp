#include "TestUtil.h"
#include "Positions.h"

#include "Perft.h"

int main() {
    for (const PerftCase& c : kPerftDeep) {
        Position pos;
        pos.SetPosition(c.fen);
        uint64 staged = Perft<PerftGen::Staged>(pos, c.depth);
        printf("  %s depth %d: %" PRIu64 " (expected %" PRIu64 ")\n",
               c.name, c.depth, staged, c.nodes);
        CHECK_EQ(staged, c.nodes);
    }
    return TestSummary("test_perft_deep");
}
