#include "TestUtil.h"
#include "Positions.h"

#include "MoveGen.h"

#include <cstring>

struct Snapshot {
    BitBoard pieces[7][2];
    BitBoard board;
    Color whiteMove;
    uint64 fullMoves;
    int ply;
    IrreversibleState state;
    uint64 hash;
    uint64 pawnHash;
    bool inCheck;
};

static Snapshot Capture(const Position& pos) {
    Snapshot s;
    std::memcpy(s.pieces, pos.m_Pieces, sizeof(s.pieces));
    s.board = pos.m_Board;
    s.whiteMove = pos.m_WhiteMove;
    s.fullMoves = pos.m_FullMoves;
    s.ply = pos.m_Ply;
    s.state = pos.m_States[pos.m_Ply];
    s.hash = pos.m_Hash;
    s.pawnHash = pos.m_PawnHash;
    s.inCheck = pos.m_InCheck;
    return s;
}

static bool Same(const Snapshot& a, const Snapshot& b) {
    return std::memcmp(a.pieces, b.pieces, sizeof(a.pieces)) == 0
        && a.board == b.board && a.whiteMove == b.whiteMove
        && a.fullMoves == b.fullMoves && a.ply == b.ply
        && a.state.m_CastleRights == b.state.m_CastleRights
        && a.state.m_HalfMoves == b.state.m_HalfMoves
        && a.state.m_EnPassant == b.state.m_EnPassant
        && a.state.m_Hash == b.state.m_Hash
        && a.hash == b.hash && a.pawnHash == b.pawnHash && a.inCheck == b.inCheck;
}

static void Walk(Position& pos, int depth, const char* name) {
    if (Zobrist_Hash(pos) != pos.m_Hash) {
        printf("  hash mismatch at %s: fen %s\n", name, pos.ToFen().c_str());
    }
    CHECK_EQ(Zobrist_Hash(pos), pos.m_Hash);

    if (depth == 0) return;

    for (Move move : GenerateMoves<ALL>(pos)) {
        Snapshot before = Capture(pos);
        pos.MovePiece(move);
        Walk(pos, depth - 1, name);
        pos.UndoMove(move);
        Snapshot after = Capture(pos);
        if (!Same(before, after)) {
            printf("  make/unmake mismatch at %s: move %s, fen %s\n",
                   name, MoveToString(move).c_str(), pos.ToFen().c_str());
        }
        CHECK(Same(before, after));
    }
}

static void NullRoundTrip(const char* fen, const char* name) {
    Position pos;
    pos.SetPosition(fen);
    Snapshot before = Capture(pos);
    pos.NullMove();
    pos.UndoNullMove();
    Snapshot after = Capture(pos);
    if (!Same(before, after)) printf("  null move mismatch at %s\n", name);
    CHECK(Same(before, after));
}

int main() {
    const char* const fens[] = { kStartPos, kKiwipete, kEndgame, kPromo, kMidgame, kComplex };
    const char* const names[] = { "startpos", "kiwipete", "endgame", "promo", "midgame", "complex" };

    printf("-- incremental hash + make/unmake roundtrip to depth 3\n");
    for (size_t i = 0; i < 6; i++) {
        Position pos;
        pos.SetPosition(fens[i]);
        Walk(pos, 3, names[i]);
    }

    printf("-- null move roundtrip\n");
    for (size_t i = 0; i < 6; i++) NullRoundTrip(fens[i], names[i]);

    return TestSummary("test_zobrist");
}
