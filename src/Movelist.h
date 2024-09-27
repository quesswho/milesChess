#pragma once

#include "Board.h"


_COMPILETIME uint64 PawnForward(uint64 pawns, const bool white) {
    return white ? pawns << 8 : pawns >> 8;
}

_COMPILETIME uint64 Pawn2Forward(uint64 pawns, const bool white) {
    return white ? pawns << 16 : pawns >> 16;
}


_COMPILETIME uint64 PawnRight(const Board& board, const bool white) {
    if (white) {
        return (board.m_WhitePawn & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    }
    else {
        return (board.m_BlackPawn & (~Lookup::lines[0])) >> 9;
    }
}

_COMPILETIME uint64 PawnLeft(const Board& board, const bool white) {
    if (white) {
        return (board.m_WhitePawn & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    }
    else {
        return (board.m_BlackPawn & (~Lookup::lines[7 * 4])) >> 7;
    }
}

_COMPILETIME uint64 PawnAttack(const Board& board, const bool white) {
    return PawnLeft(board, white) | PawnRight(board, white);
}

_COMPILETIME uint64 PawnAttackRight(uint64 pawns, const bool white) {
    if (white) {
        return (pawns & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    } else {
        return (pawns & (~Lookup::lines[0])) >> 9;
    }
}
_COMPILETIME uint64 PawnAttackLeft(uint64 pawns, const bool white) {
    if (white) {
        return (pawns & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    } else {
        return (pawns & (~Lookup::lines[7 * 4])) >> 7;
    }
}

_COMPILETIME uint64 Player(const Board& board, const bool white) {
    return white ? board.m_White : board.m_Black;
}

_COMPILETIME uint64 Enemy(const Board& board, const bool white) {
    return white ? board.m_Black : board.m_White;
}

_COMPILETIME uint64 Pawn(const Board& board, const bool white) {
    return white ? board.m_WhitePawn : board.m_BlackPawn;
}

_COMPILETIME uint64 Knight(const Board& board, const bool white) {
    return white ? board.m_WhiteKnight : board.m_BlackKnight;
}

_COMPILETIME uint64 Bishop(const Board& board, const bool white) {
    return white ? board.m_WhiteBishop : board.m_BlackBishop;
}

_COMPILETIME uint64 Rook(const Board& board, const bool white) {
    return white ? board.m_WhiteRook : board.m_BlackRook;
}

_COMPILETIME uint64 Queen(const Board& board, const bool white) {
    return white ? board.m_WhiteQueen : board.m_BlackQueen;
}

_COMPILETIME uint64 King(const Board& board, const bool white) {
    return white ? board.m_WhiteKing : board.m_BlackKing;
}