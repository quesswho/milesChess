#include "TestUtil.h"
#include "Positions.h"

#include "MoveGen.h"

int main() {
    const char* const fens[] = { kStartPos, kKiwipete, kEndgame, kPromo, kMidgame, kComplex };

    printf("-- fen roundtrip\n");
    for (const char* fen : fens) {
        Position pos;
        pos.SetPosition(fen);
        CHECK_EQ_STR(pos.ToFen(), fen);
    }

    printf("-- fen roundtrip is stable under reparse\n");
    for (const char* fen : fens) {
        Position a;
        a.SetPosition(fen);
        Position b;
        b.SetPosition(a.ToFen());
        CHECK_EQ(a.m_Hash, b.m_Hash);
        CHECK_EQ(a.m_PawnHash, b.m_PawnHash);
        CHECK_EQ(a.m_Board, b.m_Board);
        CHECK_EQ(a.m_WhiteMove, b.m_WhiteMove);
        CHECK_EQ(a.m_States[a.m_Ply].m_CastleRights, b.m_States[b.m_Ply].m_CastleRights);
        CHECK_EQ(a.m_States[a.m_Ply].m_EnPassant, b.m_States[b.m_Ply].m_EnPassant);
    }

    printf("-- en passant square survives a roundtrip\n");
    const char* const epFens[] = {
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2",
        "rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 2",
    };
    for (const char* fen : epFens) {
        Position pos;
        pos.SetPosition(fen);
        CHECK(pos.m_States[pos.m_Ply].m_EnPassant != 0);
        CHECK_EQ_STR(pos.ToFen(), fen);
    }

    printf("-- en passant arises from a double push and clears after\n");
    {
        Position pos;
        pos.SetPosition(kStartPos);
        for (Move move : GenerateMoves<ALL>(pos)) {
            if (MoveToString(move) != "e2e4") continue;
            pos.MovePiece(move);
            CHECK(pos.m_States[pos.m_Ply].m_EnPassant != 0);
            CHECK_EQ_STR(pos.ToFen(), "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
            break;
        }
    }

    return TestSummary("test_fen");
}
