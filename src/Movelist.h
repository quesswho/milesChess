#pragma once

#include "Board.h"


static uint64 PawnForward(uint64 pawns, const bool white) {
    return white ? pawns << 8 : pawns >> 8;
}

static uint64 Pawn2Forward(uint64 pawns, const bool white) {
    return white ? pawns << 16 : pawns >> 16;
}


static uint64 PawnRight(const Board& board, const bool white) {
    if (white) {
        return (board.m_WhitePawn & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    }
    else {
        return (board.m_BlackPawn & (~Lookup::lines[0])) >> 9;
    }
}

static uint64 PawnLeft(const Board& board, const bool white) {
    if (white) {
        return (board.m_WhitePawn & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    }
    else {
        return (board.m_BlackPawn & (~Lookup::lines[7 * 4])) >> 7;
    }
}

static uint64 PawnAttack(const Board& board, const bool white) {
    return PawnLeft(board, white) | PawnRight(board, white);
}

static uint64 PawnAttackRight(uint64 pawns, const bool white) {
    if (white) {
        return (pawns & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    }
    else {
        return (pawns & (~Lookup::lines[0])) >> 9;
    }
}
static uint64 PawnAttackLeft(uint64 pawns, const bool white) {
    if (white) {
        return (pawns & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    }
    else {
        return (pawns & (~Lookup::lines[7 * 4])) >> 7;
    }
}

static inline uint64 Player(const Board& board, const bool white) {
    return white ? board.m_White : board.m_Black;
}

static inline uint64 Enemy(const Board& board, const bool white) {
    return white ? board.m_Black : board.m_White;
}

static inline uint64 Pawn(const Board& board, const bool white) {
    return white ? board.m_WhitePawn : board.m_BlackPawn;
}

static inline uint64 Knight(const Board& board, const bool white) {
    return white ? board.m_WhiteKnight : board.m_BlackKnight;
}

static inline uint64 Bishop(const Board& board, const bool white) {
    return white ? board.m_WhiteBishop : board.m_BlackBishop;
}

static inline uint64 Rook(const Board& board, const bool white) {
    return white ? board.m_WhiteRook : board.m_BlackRook;
}

static inline uint64 Queen(const Board& board, const bool white) {
    return white ? board.m_WhiteQueen : board.m_BlackQueen;
}

static inline uint64 King(const Board& board, const bool white) {
    return white ? board.m_WhiteKing : board.m_BlackKing;
}