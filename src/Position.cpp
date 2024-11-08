#include "Position.h"

#include "LookupTables.h"

void Position::SetPosition(const std::string& FEN) {
    m_WhitePawn = FenToMap(FEN, 'P');
    m_WhiteKnight = FenToMap(FEN, 'N');
    m_WhiteBishop = FenToMap(FEN, 'B');
    m_WhiteRook = FenToMap(FEN, 'R');
    m_WhiteQueen = FenToMap(FEN, 'Q');
    m_WhiteKing = FenToMap(FEN, 'K');
    m_BlackPawn = FenToMap(FEN, 'p'); 
    m_BlackKnight = FenToMap(FEN, 'n'); 
    m_BlackBishop = FenToMap(FEN, 'b'); 
    m_BlackRook = FenToMap(FEN, 'r');
    m_BlackQueen = FenToMap(FEN, 'q');
    m_BlackKing = FenToMap(FEN, 'k');
    m_Black = (m_BlackPawn | m_BlackKnight | m_BlackBishop | m_BlackRook | m_BlackQueen | m_BlackKing);
    m_White = (m_WhitePawn | m_WhiteKnight | m_WhiteBishop | m_WhiteRook | m_WhiteQueen | m_WhiteKing);
    m_Board = (m_White | m_Black);
    SetState(FEN);
    m_Hash = Zobrist_Hash(*this);
    m_PawnHash = Zobrist_PawnHash(*this);
    m_States[m_Ply].m_Hash = m_Hash;
}

void Position::SetState(const std::string& FEN) {
    m_Ply = 0;
    int i = 0;
    while (FEN[i++] != ' ') {
        // Skip piece placement
    }

    char ac = FEN[i++]; // Active color
    switch (ac) {
    case 'w':
        m_WhiteMove = WHITE;
        break;
    case 'b':
        m_WhiteMove = BLACK;
        break;
    default:
        printf("Invalid FEN for active color\n");
        return;
    }

    i++;
    m_States[m_Ply].m_CastleRights = 0;
    char c = {};
    while ((c = FEN[i++]) != ' ') {
        if (c == '-') {
            break;
        }

        switch (c) {
        case 'K':
            m_States[m_Ply].m_CastleRights |= CASTLE_WHITEKING;
            break;
        case 'Q':
            m_States[m_Ply].m_CastleRights |= CASTLE_WHITEQUEEN;
            break;
        case 'k':
            m_States[m_Ply].m_CastleRights |= CASTLE_BLACKKING;
            break;
        case 'q':
            m_States[m_Ply].m_CastleRights |= CASTLE_BLACKQUEEN;
            break;
        }
    }

    while ((c = FEN[i]) == ' ') i++;
    char a = FEN[i++];

    if (a != '-') {
        if (m_WhiteMove) {
            m_States[m_Ply].m_EnPassant = 1ull << (40 + ('h' - a)); // TODO: need to be tested
        }
        else {
            m_States[m_Ply].m_EnPassant = 1ull << (16 + ('h' - a)); // TODO: need to be tested
        }
    }
    else {
        m_States[m_Ply].m_EnPassant = 0;
    }

    m_States[m_Ply].m_HalfMoves = 0;
    m_FullMoves = 0;
    int t = 1;

    while ((c = FEN[i++]) != ' ');
    while ((c = FEN[i++]) != ' ') m_States[m_Ply].m_HalfMoves = m_States[m_Ply].m_HalfMoves * 10 + (int)(c - '0');
    while (i < FEN.size() && FEN[i] >= '0' && FEN[i] <= '9') {
        c = FEN[i++];
        m_FullMoves = m_FullMoves * 10 + (c - '0');
    }
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
    const int fPos = From(move);
    const int tPos = To(move);
    const BitBoard to = (1ull << tPos);
    const BitBoard from = (1ull << fPos);
    const BitBoard re = ~(to);
    const BitBoard swp = (from) | (to);
    const int promotion = Promotion(move);
    const bool enPassant = EnPassant(move);


    if (m_States[m_Ply].m_EnPassant) { // Remove old en passant from hash
        m_Hash ^= Lookup::zobrist[64 * 12 + 5 + (GET_SQUARE(m_States[m_Ply].m_EnPassant) & 0x7)];
    }

    m_Ply++;
    m_States[m_Ply] = m_States[m_Ply - 1];

    m_States[m_Ply].m_HalfMoves++;
    if (!m_WhiteMove) m_FullMoves++;


    m_States[m_Ply].m_EnPassant = 0;

    m_Hash ^= Lookup::zobrist[64 * 12]; // White Move
    m_PawnHash ^= Lookup::zobrist[64 * 12];


    const ColoredPieceType type = MovePieceType(move);

    assert("Cant move to same color piece", move.m_To & m_White);
    switch (type) {
    case WPAWN:
        if (tPos - fPos == 16) {
            m_States[m_Ply].m_EnPassant = to >> 8;
            m_Hash ^= Lookup::zobrist[tPos] ^ Lookup::zobrist[fPos] ^ Lookup::zobrist[64 * 12 + 5 + (tPos % 8)];
            m_PawnHash ^= Lookup::zobrist[tPos] ^ Lookup::zobrist[fPos];
            m_WhitePawn ^= swp;
        } else if (promotion) {
            switch (promotion) {
            case 0x400000:
                m_Hash ^= Lookup::zobrist[fPos] ^ Lookup::zobrist[64 * 2 + tPos];
                m_PawnHash ^= Lookup::zobrist[fPos];
                m_WhitePawn ^= from;
                m_WhiteKnight ^= to;
                break;
            case 0x800000:
                m_Hash ^= Lookup::zobrist[fPos] ^ Lookup::zobrist[64 * 4 + tPos];
                m_PawnHash ^= Lookup::zobrist[fPos];
                m_WhitePawn ^= from;
                m_WhiteBishop ^= to;
                break;
            case 0x1000000:
                m_Hash ^= Lookup::zobrist[fPos] ^ Lookup::zobrist[64 * 6 + tPos];
                m_PawnHash ^= Lookup::zobrist[fPos];
                m_WhitePawn ^= from;
                m_WhiteRook ^= to;
                break;
            case 0x2000000:
                m_Hash ^= Lookup::zobrist[fPos] ^ Lookup::zobrist[64 * 8 + tPos];
                m_PawnHash ^= Lookup::zobrist[fPos];
                m_WhitePawn ^= from;
                m_WhiteQueen ^= to;
                break;
            }
        } else {
            m_Hash ^= Lookup::zobrist[tPos] ^ Lookup::zobrist[fPos];
            m_PawnHash ^= Lookup::zobrist[tPos] ^ Lookup::zobrist[fPos];
            m_WhitePawn ^= swp;
        }
        
        m_States[m_Ply].m_HalfMoves = 0;
        break;
    case WKNIGHT:
        m_Hash ^= Lookup::zobrist[64 * 2 + tPos] ^ Lookup::zobrist[64 * 2 + fPos];
        m_WhiteKnight ^= swp;
        break;
    case WBISHOP:
        m_Hash ^= Lookup::zobrist[64 * 4 + tPos] ^ Lookup::zobrist[64 * 4 + fPos];
        m_WhiteBishop ^= swp;
        break;
    case WROOK:
        if (from == 0b1ull) {
            m_States[m_Ply].m_CastleRights &= ~(CASTLE_WHITEKING);
            if ((m_States[m_Ply-1].m_CastleRights & CASTLE_WHITEKING)) {
                m_Hash ^= Lookup::zobrist[64 * 12 + 1];
            }
        } else if (from == 0b10000000ull) {
            m_States[m_Ply].m_CastleRights &= ~(CASTLE_WHITEQUEEN); // Remove WhiteQueen
            if ((m_States[m_Ply - 1].m_CastleRights & CASTLE_WHITEQUEEN)) {
                m_Hash ^= Lookup::zobrist[64 * 12 + 2];
            }
        }
        m_Hash ^= Lookup::zobrist[64 * 6 + tPos] ^ Lookup::zobrist[64 * 6 + fPos];
        m_WhiteRook ^= swp;
        break;
    case WQUEEN:
        m_Hash ^= Lookup::zobrist[64 * 8 + tPos] ^ Lookup::zobrist[64 * 8 + fPos];
        m_WhiteQueen ^= swp;
        break;
    case WKING:
        m_States[m_Ply].m_CastleRights &= ~(CASTLE_WHITEKING | CASTLE_WHITEQUEEN);
        if (m_States[m_Ply - 1].m_CastleRights & CASTLE_WHITEKING) {
            m_Hash ^= Lookup::zobrist[64 * 12 + 1];
        }
        if (m_States[m_Ply - 1].m_CastleRights & CASTLE_WHITEQUEEN) {
            m_Hash ^= Lookup::zobrist[64 * 12 + 2];
        }
        m_WhiteKing ^= swp;
        if (Castle(move)) {
            if (to == 0b10) { // King side castle
                m_Hash ^= Lookup::zobrist[64 * 10 + 1] ^ Lookup::zobrist[64 * 10 + 3] ^ Lookup::zobrist[64 * 6] ^ Lookup::zobrist[64 * 6 + 2];
                m_WhiteRook ^= 0b101ull;
            }
            else {
                m_Hash ^= Lookup::zobrist[64 * 10 + 5] ^ Lookup::zobrist[64 * 10 + 3] ^ Lookup::zobrist[64 * 6 + 7] ^ Lookup::zobrist[64 * 6 + 4];
                m_WhiteRook ^= 0b10010000ull;
            }
        }
        else {
            m_Hash ^= Lookup::zobrist[64 * 10 + tPos] ^ Lookup::zobrist[64 * 10 + fPos];
        }
        break;
    case BPAWN:
        if (fPos - tPos == 16) {
            m_States[m_Ply].m_EnPassant = to << 8;
            m_Hash ^= Lookup::zobrist[64 + tPos] ^ Lookup::zobrist[64 + fPos] ^ Lookup::zobrist[64 * 12 + 5 + (tPos % 8)];
            m_PawnHash ^= Lookup::zobrist[64 + tPos] ^ Lookup::zobrist[64 + fPos];
            m_BlackPawn ^= swp;
        }
        else if (promotion) {
            switch (promotion) {
            case 0x400000:
                m_Hash ^= Lookup::zobrist[64 + fPos] ^ Lookup::zobrist[64 * 3 + tPos];
                m_PawnHash ^= Lookup::zobrist[64 + fPos];
                m_BlackPawn ^= from;
                m_BlackKnight ^= to;
                break;
            case 0x800000:
                m_Hash ^= Lookup::zobrist[64 + fPos] ^ Lookup::zobrist[64 * 5 + tPos];
                m_PawnHash ^= Lookup::zobrist[64 + fPos];
                m_BlackPawn ^= from;
                m_BlackBishop ^= to;
                break;
            case 0x1000000:
                m_Hash ^= Lookup::zobrist[64 + fPos] ^ Lookup::zobrist[64 * 7 + tPos];
                m_PawnHash ^= Lookup::zobrist[64 + fPos];
                m_BlackPawn ^= from;
                m_BlackRook ^= to;
                break;
            case 0x2000000:
                m_Hash ^= Lookup::zobrist[64 + fPos] ^ Lookup::zobrist[64 * 9 + tPos];
                m_PawnHash ^= Lookup::zobrist[64 + fPos];
                m_BlackPawn ^= from;
                m_BlackQueen ^= to;
                break;
            }
        }
        else {
            m_Hash ^= Lookup::zobrist[64 + tPos] ^ Lookup::zobrist[64 + fPos];
            m_PawnHash ^= Lookup::zobrist[64 + tPos] ^ Lookup::zobrist[64 + fPos];
            m_BlackPawn ^= swp;
        }

        m_States[m_Ply].m_HalfMoves = 0;
        break;
    case BKNIGHT:
        m_Hash ^= Lookup::zobrist[64 * 3 + tPos] ^ Lookup::zobrist[64 * 3 + fPos];
        m_BlackKnight ^= swp;
        break;
    case BBISHOP:
        m_Hash ^= Lookup::zobrist[64 * 5 + tPos] ^ Lookup::zobrist[64 * 5 + fPos];
        m_BlackBishop ^= swp;
        break;
    case BROOK:
        if (from == (1ull << 56)) {
            m_States[m_Ply].m_CastleRights &= ~(CASTLE_BLACKKING);
            if ((m_States[m_Ply - 1].m_CastleRights & CASTLE_BLACKKING)) {
                m_Hash ^= Lookup::zobrist[64 * 12 + 3];
            }
        }
        else if (from == (0b10000000ull << 56)) {
            m_States[m_Ply].m_CastleRights &= ~(CASTLE_BLACKQUEEN); // Remove WhiteQueen
            if ((m_States[m_Ply - 1].m_CastleRights & CASTLE_BLACKQUEEN)) {
                m_Hash ^= Lookup::zobrist[64 * 12 + 4];
            }
        }
        m_Hash ^= Lookup::zobrist[64 * 7 + tPos] ^ Lookup::zobrist[64 * 7 + fPos];
        m_BlackRook ^= swp;
        break;
    case BQUEEN:
        m_Hash ^= Lookup::zobrist[64 * 9 + tPos] ^ Lookup::zobrist[64 * 9 + fPos];
        m_BlackQueen ^= swp;
        break;
    case BKING:
        m_States[m_Ply].m_CastleRights &= ~(CASTLE_BLACKKING | CASTLE_BLACKQUEEN);
        if (m_States[m_Ply - 1].m_CastleRights & CASTLE_BLACKKING) {
            m_Hash ^= Lookup::zobrist[64 * 12 + 3];
        }
        if (m_States[m_Ply - 1].m_CastleRights & CASTLE_BLACKQUEEN) {
            m_Hash ^= Lookup::zobrist[64 * 12 + 4];
        }
        m_BlackKing ^= swp;
        if (Castle(move)) {
            if (to == 0b10ull << 56) { // King side castle
                m_Hash ^= Lookup::zobrist[64 * 11 + 57] ^ Lookup::zobrist[64 * 11 + 59] ^ Lookup::zobrist[64 * 7 + 56] ^ Lookup::zobrist[64 * 7 + 58];
                m_BlackRook ^= (0b101ull << 56);
            }
            else {
                m_Hash ^= Lookup::zobrist[64 * 11 + 61] ^ Lookup::zobrist[64 * 11 + 59] ^ Lookup::zobrist[64 * 7 + 60] ^ Lookup::zobrist[64 * 7 + 63];
                m_BlackRook ^= (0b10010000ull << 56);
            }
        }
        else {
            m_Hash ^= Lookup::zobrist[64 * 11 + tPos] ^ Lookup::zobrist[64 * 11 + fPos];
        }
        break;
    }

    const ColoredPieceType capture = CaptureType(move);

    if (capture != NOPIECE) m_States[m_Ply].m_HalfMoves = 0;

    switch (capture) {
        // White pieces
    case BPAWN:
        if (enPassant) {
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
        if (enPassant) {
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
    case NOPIECE:
        // No capture
        break;
    }
    m_Black = (m_BlackPawn | m_BlackKnight | m_BlackBishop | m_BlackRook | m_BlackQueen | m_BlackKing);
    m_White = (m_WhitePawn | m_WhiteKnight | m_WhiteBishop | m_WhiteRook | m_WhiteQueen | m_WhiteKing);
    m_Board = (m_White | m_Black);

    m_WhiteMove = !m_WhiteMove;
    m_States[m_Ply].m_Hash = m_Hash;
}

void Position::UndoMove(Move move) {
    const int tPos = To(move);
    const int fPos = From(move);
    const BitBoard to = (1ull << tPos);
    const BitBoard from = (1ull << fPos);
    const BitBoard swp = (from) | (to);
    const int promotion = Promotion(move);

    m_Ply--;
    m_Hash = m_States[m_Ply].m_Hash;

    if (m_WhiteMove) m_FullMoves--;

    m_PawnHash ^= Lookup::zobrist[64 * 12];


    const ColoredPieceType type = MovePieceType(move);

    assert("Cant move to same color piece", move.m_To & m_White);
    switch (type) {
    case WPAWN:
        if (promotion) {
            switch (promotion) {
            case 0x400000:
                m_PawnHash ^= Lookup::zobrist[fPos];
                m_WhitePawn ^= from;
                m_WhiteKnight ^= to;
                break;
            case 0x800000:
                m_PawnHash ^= Lookup::zobrist[fPos];
                m_WhitePawn ^= from;
                m_WhiteBishop ^= to;
                break;
            case 0x1000000:
                m_PawnHash ^= Lookup::zobrist[fPos];
                m_WhitePawn ^= from;
                m_WhiteRook ^= to;
                break;
            case 0x2000000:
                m_PawnHash ^= Lookup::zobrist[fPos];
                m_WhitePawn ^= from;
                m_WhiteQueen ^= to;
                break;
            }
        }
        else {
            m_PawnHash ^= Lookup::zobrist[tPos] ^ Lookup::zobrist[fPos];
            m_WhitePawn ^= swp;
        }
        break;
    case WKNIGHT:
        m_WhiteKnight ^= swp;
        break;
    case WBISHOP:
        m_WhiteBishop ^= swp;
        break;
    case WROOK:
        m_WhiteRook ^= swp;
        break;
    case WQUEEN:
        m_WhiteQueen ^= swp;
        break;
    case WKING:
        m_WhiteKing ^= swp;
        if (Castle(move)) {
            if (to == 0b10) { // King side castle
                m_WhiteRook ^= 0b101ull;
            }
            else {
                m_WhiteRook ^= 0b10010000ull;
            }
        }
        break;
    case BPAWN:
        if (promotion) {
            switch (promotion) {
            case 0x400000:
                m_PawnHash ^= Lookup::zobrist[64 + fPos];
                m_BlackPawn ^= from;
                m_BlackKnight ^= to;
                break;
            case 0x800000:
                m_PawnHash ^= Lookup::zobrist[64 + fPos];
                m_BlackPawn ^= from;
                m_BlackBishop ^= to;
                break;
            case 0x1000000:
                m_PawnHash ^= Lookup::zobrist[64 + fPos];
                m_BlackPawn ^= from;
                m_BlackRook ^= to;
                break;
            case 0x2000000:
                m_PawnHash ^= Lookup::zobrist[64 + fPos];
                m_BlackPawn ^= from;
                m_BlackQueen ^= to;
                break;
            }
        }
        else {
            m_PawnHash ^= Lookup::zobrist[64 + tPos] ^ Lookup::zobrist[64 + fPos];
            m_BlackPawn ^= swp;
        }
        break;
    case BKNIGHT:
        m_BlackKnight ^= swp;
        break;
    case BBISHOP:
        m_BlackBishop ^= swp;
        break;
    case BROOK:
        m_BlackRook ^= swp;
        break;
    case BQUEEN:
        m_BlackQueen ^= swp;
        break;
    case BKING:
        m_BlackKing ^= swp;
        if (Castle(move)) {
            if (to == 0b10ull << 56) { // King side castle
                m_BlackRook ^= (0b101ull << 56);
            }
            else {
                m_BlackRook ^= (0b10010000ull << 56);
            }
        }
        break;
    }

    const ColoredPieceType capture = CaptureType(move);

    if (capture != NOPIECE) m_States[m_Ply].m_HalfMoves = 0;

    switch (capture) {
        // White pieces
    case BPAWN:
        if (EnPassant(move)) {
            m_PawnHash ^= Lookup::zobrist[56 + tPos];
            m_BlackPawn ^= (to >> 8);
        }
        else {
            m_PawnHash ^= Lookup::zobrist[64 + tPos];
            m_BlackPawn ^= to;
        }
        break;
    case BKNIGHT:
        m_BlackKnight ^= to;
        break;
    case BBISHOP:
        m_BlackBishop ^= to;
        break;
    case BROOK:
        m_BlackRook ^= to;
        break;
    case BQUEEN:
        m_BlackQueen ^= to;
        break;
        // Black pieces
    case WPAWN:
        if (EnPassant(move)) {
            m_PawnHash ^= Lookup::zobrist[8 + tPos];
            m_WhitePawn ^= (to << 8);
        }
        else {
            m_PawnHash ^= Lookup::zobrist[tPos];
            m_WhitePawn ^= to;
        }
        break;
    case WKNIGHT:
        m_WhiteKnight ^= to;
        break;
    case WBISHOP:
        m_WhiteBishop ^= to;
        break;
    case WROOK:
        m_WhiteRook ^= to;
        break;
    case WQUEEN:
        m_WhiteQueen ^= to;
        break;
    case NOPIECE:
        // No capture
        break;
    }

    m_Black = (m_BlackPawn | m_BlackKnight | m_BlackBishop | m_BlackRook | m_BlackQueen | m_BlackKing);
    m_White = (m_WhitePawn | m_WhiteKnight | m_WhiteBishop | m_WhiteRook | m_WhiteQueen | m_WhiteKing);
    m_Board = (m_White | m_Black);

    m_WhiteMove = !m_WhiteMove;
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
    if (position.m_States[position.m_Ply].m_CastleRights & CASTLE_WHITEKING) result ^= Lookup::zobrist[64 * 12 + 1];
    if (position.m_States[position.m_Ply].m_CastleRights & CASTLE_WHITEQUEEN) result ^= Lookup::zobrist[64 * 12 + 2];
    if (position.m_States[position.m_Ply].m_CastleRights & CASTLE_BLACKKING) result ^= Lookup::zobrist[64 * 12 + 3];
    if (position.m_States[position.m_Ply].m_CastleRights & CASTLE_BLACKQUEEN) result ^= Lookup::zobrist[64 * 12 + 4];
    if (position.m_States[position.m_Ply].m_EnPassant) result ^= Lookup::zobrist[64 * 12 + 5 + (GET_SQUARE(position.m_States[position.m_Ply].m_EnPassant) % 8)];

    return result;
}

std::string Position::ToFen() const {
    std::string FEN;
    FEN.reserve(27); // Shortest FEN could be: k7/8/8/8/8/8/8/7K w - - 0 1

    // Board
    int skip = 0; // Count number of empty positions in each rank
    for (int i = 63; i >= 0; i--) {
        uint64 pos = 1ull << i;

        if (i % 8 == 7) {
            if (skip > 0) {
                FEN += (char)('0' + skip);
                skip = 0;
            }
            if (i != 63) FEN += '/';
        }

        if (!(m_Board & pos)) {
            skip++;
            continue;
        }
        else if (skip > 0) {
            FEN += (char)('0' + skip);
            skip = 0;
        }

        if ((m_BlackPawn & pos) > 0) {
            FEN += "p";
            continue;
        }
        if ((m_BlackKnight & pos) > 0) {
            FEN += "n";
            continue;
        }
        if ((m_BlackBishop & pos) > 0) {
            FEN += "b";
            continue;
        }
        if ((m_BlackRook & pos) > 0) {
            FEN += "r";
            continue;
        }
        if ((m_BlackQueen & pos) > 0) {
            FEN += "q";
            continue;
        }
        if ((m_BlackKing & pos) > 0) {
            FEN += "k";
            continue;
        }
        if ((m_WhitePawn & pos) > 0) {
            FEN += "P";
            continue;
        }
        if ((m_WhiteKnight & pos) > 0) {
            FEN += "N";
            continue;
        }
        if ((m_WhiteBishop & pos) > 0) {
            FEN += "B";
            continue;
        }
        if ((m_WhiteRook & pos) > 0) {
            FEN += "R";
            continue;
        }
        if ((m_WhiteQueen & pos) > 0) {
            FEN += "Q";
            continue;
        }
        if ((m_WhiteKing & pos) > 0) {
            FEN += "K";
            continue;
        }
    }

    if (skip > 0) {
        FEN += (char)('0' + skip);
        skip = 0;
    }
    

    if (m_WhiteMove) {
        FEN += " w ";
    }
    else {
        FEN += " b ";
    }

    // Castle rights
    uint8 castling = m_States[m_Ply].m_CastleRights;
    if (castling & 0b1) {
        FEN += "K";
    }
    if (castling & 0b10) {
        FEN += "Q";
    }
    if (castling & 0b100) {
        FEN += "k";
    }
    if (castling & 0b1000) {
        FEN += "q";
    }
    if (!castling) {
        FEN += '-';
    }

    FEN += ' ';
    if (m_States[m_Ply].m_EnPassant > 0) {
        char pos = 'h' - ((int)(log2(m_States[m_Ply].m_EnPassant)) % 8);
        if (m_WhiteMove) {
            FEN += pos + std::to_string(6);
        }
        else {
            FEN += pos + std::to_string(3);
        }
    }
    else {
        FEN += '-';
    }

    FEN += ' ' + std::to_string(m_States[m_Ply].m_HalfMoves);
    FEN += ' ' + std::to_string(m_FullMoves);

    return FEN;
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
    if((move == WPAWN || move == BPAWN) && position.m_States[position.m_Ply].m_EnPassant & (1ull << to)) {
        result |= 0x100000;
    }
    if (move == WKING || move == BKING) {
        if ((from & 0x7) + (to & 0x7) > 1) { // If piece moved two squares horizontally
            result |= 0x200000;
        }
    }

    return result;
}