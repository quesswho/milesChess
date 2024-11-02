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

void Position::MovePiece(Move move) {
    const int tPos = To(move);
    const int fPos = From(move);
    const BitBoard to = (1ull << tPos);
    const BitBoard from = (1ull << fPos);
    const BitBoard re = ~(to);
    const BitBoard swp = (from) | (to);

    m_HalfMoves++;
    if (!m_WhiteMove) m_FullMoves++;

    if (m_EnPassant) { // Remove old en passant
        m_Hash ^= Lookup::zobrist[64 * 12 + 5 + (GET_SQUARE(m_EnPassant) & 0x7)];
    }


    m_EnPassant = 0;
    m_WhiteMove = !m_WhiteMove;

    m_Hash ^= Lookup::zobrist[64 * 12]; // White Move
    m_PawnHash ^= Lookup::zobrist[64 * 12];


    const ColoredPieceType type = CaptureType(move);

    assert("Cant move to same color piece", move.m_To & m_White);
    switch (type) {
    case WPAWN:
        if (tPos - fPos == 16) {
            m_EnPassant = to >> 8;
            m_Hash ^= Lookup::zobrist[tPos] ^ Lookup::zobrist[fPos] ^ Lookup::zobrist[64 * 12 + 5 + (tPos % 8)];
            m_PawnHash ^= Lookup::zobrist[tPos] ^ Lookup::zobrist[fPos] ^ Lookup::zobrist[64 * 12 + 5 + (tPos % 8)];
        } else {
            m_Hash ^= Lookup::zobrist[tPos] ^ Lookup::zobrist[fPos];
            m_PawnHash ^= Lookup::zobrist[tPos] ^ Lookup::zobrist[fPos];
        }
        m_WhitePawn ^= swp;
        m_HalfMoves = 0;
        break;
    case WKNIGHT:
        m_Hash ^= Lookup::zobrist[64 * 2 + tPos] ^ Lookup::zobrist[64 * 2 + fPos];
        m_WhiteKnight ^= swp;
    case WBISHOP:
        m_Hash ^= Lookup::zobrist[64 * 4 + tPos] ^ Lookup::zobrist[64 * 4 + fPos];
        m_WhiteBishop ^= swp;
    case WROOK:
        if (from == 0b1ull) {
            m_WhiteCastleKing = false;
            if (m_WhiteCastleKing) {
                m_Hash ^= Lookup::zobrist[64 * 12 + 1];
            }
        } else if (from == 0b10000000ull) {
            m_WhiteCastleQueen = false;
            if (m_WhiteCastleQueen) {
                m_Hash ^= Lookup::zobrist[64 * 12 + 2];
            }
        }
        m_Hash ^= Lookup::zobrist[64 * 6 + tPos] ^ Lookup::zobrist[64 * 6 + fPos];
        m_WhiteRook ^= swp;
    case WQUEEN:
        m_Hash ^= Lookup::zobrist[64 * 8 + tPos] ^ Lookup::zobrist[64 * 8 + fPos];
        m_WhiteQueen ^= swp;
    case WKING:
        m_WhiteCastleQueen = false;
        m_WhiteCastleKing = false;
        if (m_Info[ply - 1].m_WhiteCastleQueen) {
            m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 2];
        }
        if (m_Info[ply - 1].m_WhiteCastleKing) {
            m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 1];
        }
        m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 10 + tPos] ^ Lookup::zobrist[64 * 10 + fPos];
        return Board(wp, wkn, wb, wr, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
    case EPASSANT:
        m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[tPos] ^ Lookup::zobrist[fPos];
        m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[tPos] ^ Lookup::zobrist[fPos];
        return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & ~(move.m_To >> 8), bkn, bb, br, bq, bk);
    case KCASTLE:
        m_Info[ply].m_WhiteCastleQueen = false;
        m_Info[ply].m_WhiteCastleKing = false;
        if (m_Info[ply - 1].m_WhiteCastleQueen) {
            m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 2];
        }
        if (m_Info[ply - 1].m_WhiteCastleKing) {
            m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 1];
        }
        m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 10 + 1] ^ Lookup::zobrist[64 * 10 + 3] ^ Lookup::zobrist[64 * 6] ^ Lookup::zobrist[64 * 6 + 2];
        return Board(wp, wkn, wb, wr ^ 0b101ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
    case QCASTLE:
        m_Info[ply].m_WhiteCastleQueen = false;
        m_Info[ply].m_WhiteCastleKing = false;
        if (m_Info[ply - 1].m_WhiteCastleQueen) {
            m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 2];
        }
        if (m_Info[ply - 1].m_WhiteCastleKing) {
            m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 1];
        }
        m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 10 + 5] ^ Lookup::zobrist[64 * 10 + 3] ^ Lookup::zobrist[64 * 6 + 7] ^ Lookup::zobrist[64 * 6 + 4];
        return Board(wp, wkn, wb, wr ^ 0b10010000ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
    case P_KNIGHT:
        m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[fPos] ^ Lookup::zobrist[64 * 2 + tPos];
        m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[fPos];
        return Board(wp & (~move.m_From), wkn | move.m_To, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
    case P_BISHOP:
        m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[fPos] ^ Lookup::zobrist[64 * 4 + tPos];
        m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[fPos];
        return Board(wp & (~move.m_From), wkn, wb | move.m_To, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
    case P_ROOK:
        m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[fPos] ^ Lookup::zobrist[64 * 6 + tPos];
        m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[fPos];
        return Board(wp & (~move.m_From), wkn, wb, wr | move.m_To, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
    case P_QUEEN:
        m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[fPos] ^ Lookup::zobrist[64 * 8 + tPos];
        m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[fPos];
        return Board(wp & (~move.m_From), wkn, wb, wr, wq | move.m_To, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
    case MoveType::NONE:
        assert("Invalid move!");
        break;
    }

    const ColoredPieceType capture = CaptureType(move);

    if (capture != NONE) m_HalfMoves = 0;

    switch (capture) {
        // White pieces
    case BPAWN:
        if (EnPassant(move)) {
            m_Hash ^= Lookup::zobrist[56 + tPos];
            m_PawnHash ^= Lookup::zobrist[56 + tPos];
            m_BlackPawn &= ~(to >> 8);
        } else {
            m_Hash ^= Lookup::zobrist[64 + tPos];
            m_PawnHash ^= Lookup::zobrist[64 + tPos];
            m_BlackPawn &= re;
        }
        break;
    case BKNIGHT:
        m_Hash ^= Lookup::zobrist[64 * 3 + tPos];
        m_BlackKnight &= re;
        break;
    case BBISHOP:
        m_Hash ^= Lookup::zobrist[64 * 5 + tPos];
        m_BlackBishop &= re;
        break;
    case BROOK:
        m_Hash ^= Lookup::zobrist[64 * 7 + tPos];
        m_BlackRook &= re;
        break;
    case BQUEEN:
        m_Hash ^= Lookup::zobrist[64 * 9 + tPos];
        m_BlackQueen &= re;
        break;
        // Black pieces
    case WPAWN:
        if (EnPassant(move)) {
            m_Hash ^= Lookup::zobrist[8 + tPos];
            m_PawnHash ^= Lookup::zobrist[8 + tPos];
            m_WhitePawn &= ~(to << 8);
        } else {
            m_Hash ^= Lookup::zobrist[tPos];
            m_PawnHash ^= Lookup::zobrist[tPos];
            m_WhitePawn &= re;
        }
        break;
    case WKNIGHT:
        m_Hash ^= Lookup::zobrist[64 * 2 + tPos];
        m_WhiteKnight &= re;
        break;
    case WBISHOP:
        m_Hash ^= Lookup::zobrist[64 * 4 + tPos];
        m_WhiteBishop &= re;
        break;
    case WROOK:
        m_Hash ^= Lookup::zobrist[64 * 6 + tPos];
        m_WhiteRook &= re;
        break;
    case WQUEEN:
        m_Hash ^= Lookup::zobrist[64 * 8 + tPos];
        m_WhiteQueen &= re;
        break;
    case NONE:
        // No capture
        break;
    }
    m_History[m_Info[ply].m_HalfMoves] = m_Hash[ply];
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

static Move GetMove(const Position& position, std::string str) {

    const uint32 from = ('h' - str[0] + (str[1] - '1') * 8);
    const uint32 to = ('h' - str[2] + (str[3] - '1') * 8);
    Move result = from | (to << 6);

    uint32 moveFlag = 0;
    if (str.size() > 4) {
        switch (str[4]) {
        case 'k':
            result |= 0x200000;
            break;
        case 'b':
            result |= 0x400000;
            break;
        case 'r':
            result |= 0x800000;
            break;
        case 'q':
            result |= 0x1000000;
            break;
        }
    }
    ColoredPieceType move = position.m_Squares[from];
    result |= move << 12;
    ColoredPieceType capture = position.m_Squares[to];
    result |= capture << 16;
    if((move == WPAWN || move == BPAWN) && position.m_EnPassant & (1ull << to)) {
        result |= 0x100000;
    }
    if (move == WKING || move == BKING) {
        if ((from & 0x7) + (to & 0x7) > 1) { // If piece moved two squares horizontally
            result |= 0x200000;
        }
    }

    return result;
}