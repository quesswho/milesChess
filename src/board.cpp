#include "board.h"
#include <immintrin.h> // _lzcnt_u64(x)
#include <stdio.h>

Board::Board(const std::string& FEN) 
    : m_WhitePawn(FenToMap(FEN, 'P')), m_WhiteKnight(FenToMap(FEN, 'N')), m_WhiteBishop(FenToMap(FEN, 'B')), m_WhiteRook(FenToMap(FEN, 'R')), m_WhiteQueen(FenToMap(FEN, 'Q')), m_WhiteKing(FenToMap(FEN, 'K')),
    m_BlackPawn(FenToMap(FEN, 'p')), m_BlackKnight(FenToMap(FEN, 'n')), m_BlackBishop(FenToMap(FEN, 'b')), m_BlackRook(FenToMap(FEN, 'r')), m_BlackQueen(FenToMap(FEN, 'q')), m_BlackKing(FenToMap(FEN, 'k')),
    m_BoardInfo(FenInfo(FEN)), m_Black(m_BlackPawn | m_BlackKnight | m_BlackBishop | m_BlackRook | m_BlackQueen | m_BlackKing), m_White(m_WhitePawn | m_WhiteKnight | m_WhiteBishop | m_WhiteRook | m_WhiteQueen | m_WhiteKing),
    m_Board(m_White | m_Black)
{}

uint64 Board::RookAttack(int pos, uint64 occ) const {
    uint64 boardpos = 1ull << pos;
    return (Slide(boardpos, Lookup::linesEx[4 * pos], occ) | Slide(boardpos, Lookup::linesEx[4 * pos + 1], occ));
}

uint64 Board::BishopAttack(int pos, uint64 occ) const {
    uint64 boardpos = 1ull << pos;
    return (Slide(boardpos, Lookup::linesEx[4 * pos + 2], occ) | Slide(boardpos, Lookup::linesEx[4 * pos + 3], occ));
}

uint64 Board::QueenAttack(int pos, uint64 occ) const {
    uint64 boardpos = 1ull << pos;
    return (Slide(boardpos, Lookup::linesEx[4 * pos], occ) | Slide(boardpos, Lookup::linesEx[4 * pos + 1], occ)
        | Slide(boardpos, Lookup::linesEx[4 * pos + 2], occ) | Slide(boardpos, Lookup::linesEx[4 * pos + 3], occ));
}

uint64_t Board::RookXray(int pos, uint64_t occ) const {
    uint64_t attacks = RookAttack(pos, occ);
    return attacks ^ RookAttack(pos, occ ^ (attacks & occ));
}

uint64_t Board::BishopXray(int pos, uint64_t occ) const {
    uint64_t attacks = BishopAttack(pos, occ);
    return attacks ^ BishopAttack(pos, occ ^ (attacks & occ));
}


uint64 Board::PawnForward(uint64 pawns, const bool white) const {
    return white ? pawns << 8 : pawns >> 8;
}

uint64 Board::Pawn2Forward(uint64 pawns, const bool white) const {
    return white ? pawns << 16 : pawns >> 16;
}


uint64 Board::PawnRight(const bool white) const {
    if (white) {
        return (m_WhitePawn & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    } else {
        return (m_BlackPawn & (~Lookup::lines[0])) >> 9;
    }
}

uint64 Board::PawnLeft(const bool white) const {
    if (white) {
        return (m_WhitePawn & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    }
    else {
        return (m_BlackPawn & (~Lookup::lines[7 * 4])) >> 7;
    }
}

uint64 Board::PawnAttack(const bool white) const {
    return PawnLeft(white) | PawnRight(white);
}

uint64 Board::PawnAttackRight(uint64 pawns, const bool white) const {
    if (white) {
        return (pawns & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    }
    else {
        return (pawns & (~Lookup::lines[0])) >> 9;
    }
}
uint64 Board::PawnAttackLeft(uint64 pawns, const bool white) const {
    if (white) {
        return (pawns & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    }
    else {
        return (pawns & (~Lookup::lines[7 * 4])) >> 7;
    }
}

Board Board::MovePiece(const Move& move) const {
    const uint64 re = ~move.m_To;
    const uint64 swp = move.m_From | move.m_To;
    const bool white = m_BoardInfo.m_WhiteMove;
    const uint64 wp = m_WhitePawn, wkn = m_WhiteKnight, wb = m_WhiteBishop, wr = m_WhiteRook, wq = m_WhiteQueen, wk = m_WhiteKing,
                 bp = m_BlackPawn, bkn = m_BlackKnight, bb = m_BlackBishop, br = m_BlackRook, bq = m_BlackQueen, bk = m_BlackKing;

    BoardInfo info = m_BoardInfo;
    info.m_HalfMoves++;
    if (!white) info.m_FullMoves++;
    info.m_WhiteMove = !info.m_WhiteMove;
    info.m_EnPassant = 0;

    if (m_BoardInfo.m_WhiteMove) {
        assert("Cant move to same color piece", move.m_To & m_White);
        switch (move.m_Type) {
        case MoveType::PAWN:
            return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::PAWN2:
            info.m_EnPassant = move.m_To >> 8;
            return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::KNIGHT:
            return Board(wp, wkn ^ swp, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::BISHOP:
            return Board(wp, wkn, wb ^ swp, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::ROOK:
            if(move.m_From == 0b1ull) info.m_WhiteCastleKing = false;
            else if(move.m_From == 0b10000000ull) info.m_WhiteCastleQueen = false;
            return Board(wp, wkn, wb, wr ^ swp, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::QUEEN:
            return Board(wp, wkn, wb, wr, wq ^ swp, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::KING:
            info.m_WhiteCastleKing = false;
            info.m_WhiteCastleQueen = false;
            return Board(wp, wkn, wb, wr, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::EPASSANT:
            return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & ~(move.m_To >> 8), bkn, bb, br, bq, bk, info);
        case MoveType::KCASTLE:
            info.m_WhiteCastleKing = false;
            info.m_WhiteCastleQueen = false;
            return Board(wp, wkn, wb, wr ^ 0b101ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::QCASTLE:
            info.m_WhiteCastleKing = false;
            info.m_WhiteCastleQueen = false;
            return Board(wp, wkn, wb, wr ^ 0b10010000ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::P_KNIGHT:
            return Board(wp & (~move.m_From), wkn | move.m_To, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::P_BISHOP:
            return Board(wp & (~move.m_From), wkn, wb | move.m_To, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::P_ROOK:
            return Board(wp & (~move.m_From), wkn, wb, wr | move.m_To, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::P_QUEEN:
            return Board(wp & (~move.m_From), wkn, wb, wr, wq | move.m_To, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case MoveType::NONE:
            assert("Invalid move!");
            break;
        }
    }
    else {
        assert("Cant move to same color piece", move.m_To & m_Black);
        switch (move.m_Type) {
        case MoveType::PAWN:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk, info);
        case MoveType::PAWN2:
            info.m_EnPassant = move.m_To << 8;
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk, info);
        case MoveType::KNIGHT:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn ^ swp, bb, br, bq, bk, info);
        case MoveType::BISHOP:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb ^ swp, br, bq, bk, info);
        case MoveType::ROOK:
            if (move.m_From == (1ull << 56)) info.m_BlackCastleKing = false;
            else if (move.m_From == (0b10000000ull << 56)) info.m_BlackCastleQueen = false;
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ swp, bq, bk, info);
        case MoveType::QUEEN:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq ^ swp, bk, info);
        case MoveType::KING:
            info.m_BlackCastleKing = false;
            info.m_BlackCastleQueen = false;
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq, bk ^ swp, info);
        case MoveType::EPASSANT:
            return Board(wp & ~(move.m_To << 8), wkn, wb, wr, wq, wk, bp ^ swp, bkn, bb, br, bq, bk, info);
        case MoveType::KCASTLE:
            info.m_BlackCastleKing = false;
            info.m_BlackCastleQueen = false;
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ (0b101ull << 56), bq, bk ^ swp, info);
        case MoveType::QCASTLE:
            info.m_BlackCastleQueen = false;
            info.m_BlackCastleKing = false;
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ (0b10010000ull << 56), bq, bk ^ swp, info);
        case MoveType::P_KNIGHT:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn | move.m_To, bb, br, bq, bk, info);
        case MoveType::P_BISHOP:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb | move.m_To, br, bq, bk, info);
        case MoveType::P_ROOK:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb, br | move.m_To, bq, bk, info);
        case MoveType::P_QUEEN:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb, br, bq | move.m_To, bk, info);
        case MoveType::NONE:
            assert("Invalid move!");
            break;
        }
    }
}