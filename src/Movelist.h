#pragma once

#include "Board.h"


uint64 PawnForward(uint64 pawns, const bool white) {
    return white ? pawns << 8 : pawns >> 8;
}

uint64 Pawn2Forward(uint64 pawns, const bool white) {
    return white ? pawns << 16 : pawns >> 16;
}


uint64 PawnRight(const Board& board, const bool white) {
    if (white) {
        return (board.m_WhitePawn & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    }
    else {
        return (board.m_BlackPawn & (~Lookup::lines[0])) >> 9;
    }
}

uint64 PawnLeft(const Board& board, const bool white) {
    if (white) {
        return (board.m_WhitePawn & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    }
    else {
        return (board.m_BlackPawn & (~Lookup::lines[7 * 4])) >> 7;
    }
}

uint64 PawnAttack(const Board& board, const bool white) {
    return PawnLeft(board, white) | PawnRight(board, white);
}

uint64 PawnAttackRight(uint64 pawns, const bool white) {
    if (white) {
        return (pawns & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    }
    else {
        return (pawns & (~Lookup::lines[0])) >> 9;
    }
}
uint64 PawnAttackLeft(uint64 pawns, const bool white) {
    if (white) {
        return (pawns & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    }
    else {
        return (pawns & (~Lookup::lines[7 * 4])) >> 7;
    }
}


inline uint64 Player(const Board& board, const bool white) {
    return white ? board.m_White : board.m_Black;
}

inline uint64 Enemy(const Board& board, const bool white) {
    return white ? board.m_Black : board.m_White;
}

inline uint64 Pawn(const Board& board, const bool white) {
    return white ? board.m_WhitePawn : board.m_BlackPawn;
}

inline uint64 Knight(const Board& board, const bool white) {
    return white ? board.m_WhiteKnight : board.m_BlackKnight;
}

inline uint64 Bishop(const Board& board, const bool white) {
    return white ? board.m_WhiteBishop : board.m_BlackBishop;
}

inline uint64 Rook(const Board& board, const bool white) {
    return white ? board.m_WhiteRook : board.m_BlackRook;
}

inline uint64 Queen(const Board& board, const bool white) {
    return white ? board.m_WhiteQueen : board.m_BlackQueen;
}

inline uint64 King(const Board& board, const bool white) {
    return white ? board.m_WhiteKing : board.m_BlackKing;
}