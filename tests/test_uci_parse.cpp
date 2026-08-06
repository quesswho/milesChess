#include "TestUtil.h"
#include "Positions.h"

#include "Search.h"
#include "MoveGen.h"

int main() {
    printf("-- trim_str does not throw on degenerate input\n");
    CHECK_EQ_STR(trim_str(""), "");
    CHECK_EQ_STR(trim_str(" "), " ");
    CHECK_EQ_STR(trim_str("\t"), "");
    CHECK_EQ_STR(trim_str("a"), "a");
    CHECK_EQ_STR(trim_str("go  depth  5"), "go depth 5");
    CHECK_EQ_STR(trim_str("go\tdepth"), "godepth");

    printf("-- MoveToString/GetMove roundtrip over all legal moves\n");
    Search search;
    const char* const fens[] = { kStartPos, kKiwipete, kEndgame, kPromo, kMidgame, kComplex };
    const char* const names[] = { "startpos", "kiwipete", "endgame", "promo", "midgame", "complex" };

    for (size_t i = 0; i < 6; i++) {
        search.m_Position.SetPosition(fens[i]);
        for (Move move : GenerateMoves<ALL>(search.m_Position)) {
            std::string str = MoveToString(move);
            Move parsed = search.GetMove(str);
            if (parsed != move) {
                printf("  %s: '%s' generated %08x, parsed %08x\n",
                       names[i], str.c_str(), move, parsed);
            }
            CHECK_EQ(parsed, move);
        }
    }

    return TestSummary("test_uci_parse");
}
