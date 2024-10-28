#include "Position.h"

#include "LookupTables.h"

Position::Position(const std::string& FEN)
    : m_WhitePawn(FenToMap(FEN, 'P')), m_WhiteKnight(FenToMap(FEN, 'N')), m_WhiteBishop(FenToMap(FEN, 'B')), m_WhiteRook(FenToMap(FEN, 'R')), m_WhiteQueen(FenToMap(FEN, 'Q')), m_WhiteKing(FenToMap(FEN, 'K')),
    m_BlackPawn(FenToMap(FEN, 'p')), m_BlackKnight(FenToMap(FEN, 'n')), m_BlackBishop(FenToMap(FEN, 'b')), m_BlackRook(FenToMap(FEN, 'r')), m_BlackQueen(FenToMap(FEN, 'q')), m_BlackKing(FenToMap(FEN, 'k')), 
    m_BoardInfo(FenToBoardInfo(FEN))
{
    m_Black = (m_BlackPawn | m_BlackKnight | m_BlackBishop | m_BlackRook | m_BlackQueen | m_BlackKing);
    m_White = (m_WhitePawn | m_WhiteKnight | m_WhiteBishop | m_WhiteRook | m_WhiteQueen | m_WhiteKing);
    m_Board = (m_White | m_Black);
    m_Hash = Zobrist_Hash(*this);
}

BitBoard Position::RookAttack(int pos, BitBoard occ) const {
    Lookup::BlackMagic m = Lookup::r_magics[pos];
    return m.attacks[((occ | m.mask) * m.hash) >> (64 - 12)];
}

BitBoard Position::BishopAttack(int pos, BitBoard occ) const {
    Lookup::BlackMagic m = Lookup::b_magics[pos];
    return m.attacks[((occ | m.mask) * m.hash) >> (64 - 9)];
}

BitBoard Position::QueenAttack(int pos, BitBoard occ) const {
    return RookAttack(pos, occ) | BishopAttack(pos, occ);
}

BitBoard Position::RookXray(int pos, BitBoard occ) const {
    BitBoard attacks = RookAttack(pos, occ);
    return attacks ^ RookAttack(pos, occ ^ (attacks & occ));
}

BitBoard Position::BishopXray(int pos, BitBoard occ) const {
    BitBoard attacks = BishopAttack(pos, occ);
    return attacks ^ BishopAttack(pos, occ ^ (attacks & occ));
}

uint64 Zobrist_Hash(const Position& position) {
    uint64 result = 0;

    BitBoard wp = position.m_WhitePawn, wkn = position.m_WhiteKnight, wb = position.m_WhiteBishop, wr = position.m_WhiteRook, wq = position.m_WhiteQueen, wk = position.m_WhiteKing,
        bp = position.m_BlackPawn, bkn = position.m_BlackKnight, bb = position.m_BlackBishop, br = position.m_BlackRook, bq = position.m_BlackQueen, bk = position.m_BlackKing;

    while (wp > 0) {
        int pos = PopPos(wp);
        result ^= Lookup::zobrist[pos];
    }

    while (bp > 0) {
        int pos = PopPos(bp);
        result ^= Lookup::zobrist[pos + 64];
    }

    // Knights
    while (wkn > 0) {
        int pos = PopPos(wkn);
        result ^= Lookup::zobrist[pos + 64 * 2];
    }

    while (bkn > 0) {
        int pos = PopPos(bkn);
        result ^= Lookup::zobrist[pos + 64 * 3];
    }

    // Bishops
    while (wb > 0) {
        int pos = PopPos(wb);
        result ^= Lookup::zobrist[pos + 64 * 4];
    }

    while (bb > 0) {
        int pos = PopPos(bb);
        result ^= Lookup::zobrist[pos + 64 * 5];
    }

    // Rooks
    while (wr > 0) {
        int pos = PopPos(wr);
        result ^= Lookup::zobrist[pos + 64 * 6];
    }

    while (br > 0) {
        int pos = PopPos(br);
        result ^= Lookup::zobrist[pos + 64 * 7];
    }

    while (wq > 0) {
        int pos = PopPos(wq);
        result ^= Lookup::zobrist[pos + 64 * 8];
    }

    while (bq > 0) {
        int pos = PopPos(bq);
        result ^= Lookup::zobrist[pos + 64 * 9];
    }

    while (wk > 0) {
        int pos = PopPos(wk);
        result ^= Lookup::zobrist[pos + 64 * 10];
    }

    while (bk > 0) {
        int pos = PopPos(bk);
        result ^= Lookup::zobrist[pos + 64 * 11];
    }

    if (position.m_WhiteMove) result ^= Lookup::zobrist[64 * 12];
    if (position.m_WhiteCastleKing) result ^= Lookup::zobrist[64 * 12 + 1];
    if (position.m_WhiteCastleQueen) result ^= Lookup::zobrist[64 * 12 + 2];
    if (position.m_BlackCastleKing) result ^= Lookup::zobrist[64 * 12 + 3];
    if (position.m_BlackCastleQueen) result ^= Lookup::zobrist[64 * 12 + 4];
    if (position.m_EnPassant) result ^= Lookup::zobrist[64 * 12 + 5 + (GET_SQUARE(position.m_EnPassant) % 8)];

    return result;
}

static uint64 Zobrist_PawnHash(const Position& position) {
    uint64 result = 0;

    BitBoard wp = position.m_WhitePawn, bp = position.m_BlackPawn;

    while (wp > 0) {
        int pos = PopPos(wp);
        result ^= Lookup::zobrist[pos];
    }

    while (bp > 0) {
        int pos = PopPos(bp);
        result ^= Lookup::zobrist[pos + 64];
    }

    return result;
}