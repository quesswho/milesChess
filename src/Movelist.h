#pragma once

#include "Board.h"

template<Color white>
_COMPILETIME BitBoard FirstRank() {
    if constexpr (white) {
        return Lookup::lines[1];
    } else {
        return Lookup::lines[4 * 56 + 1];
    }
}

template<Color white>
_COMPILETIME BitBoard PawnForward(BitBoard pawns) {
    if constexpr (white) {
        return pawns << 8;
    } else {
        return pawns >> 8;
    }
}

template<Color white>
_COMPILETIME BitBoard Pawn2Forward(BitBoard pawns) {
    if constexpr (white) {
        return pawns << 16;
    } else {
        return pawns >> 16;
    }
}

template<Color white>
_COMPILETIME BitBoard PawnRight(const Board& board) {
    if constexpr (white) {
        return (board.m_WhitePawn & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8;
    } else {
        return (board.m_BlackPawn & (~Lookup::lines[0])) >> 9;
    }
}

template<Color white>
_COMPILETIME BitBoard PawnLeft(const Board& board) {
    if constexpr (white) {
        return (board.m_WhitePawn & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 8;
    } else {
        return (board.m_BlackPawn & (~Lookup::lines[7 * 4])) >> 7;
    }
}

template<Color white>
_COMPILETIME BitBoard PawnAttack(const Board& board) {
    return PawnLeft<white>(board) | PawnRight<white>(board);
}

template<Color white>
_COMPILETIME BitBoard PawnAttackRight(BitBoard pawns) {
    if constexpr (white) {
        return (pawns & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    } else {
        return (pawns & (~Lookup::lines[0])) >> 9;
    }
}

template<Color white>
_COMPILETIME BitBoard PawnAttackLeft(BitBoard pawns) {
    if constexpr (white) {
        return (pawns & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 8
    } else {
        return (pawns & (~Lookup::lines[7 * 4])) >> 7;
    }
}

template<Color white>
_COMPILETIME BitBoard Player(const Board& board) {
    if constexpr (white) {
        return board.m_White;
    } else {
        return board.m_Black;
    }
}

template<Color white>
_COMPILETIME BitBoard Enemy(const Board& board) {
    if constexpr (white) {
        return board.m_Black;
    } else {
        return board.m_White;
    }
}

template<Color white>
_COMPILETIME BitBoard Pawn(const Board& board) {
    if constexpr (white) {
        return board.m_WhitePawn;
    } else {
        return board.m_BlackPawn;
    }
}

template<Color white>
_COMPILETIME BitBoard Knight(const Board& board) {
    if constexpr (white) {
        return board.m_WhiteKnight;
    } else {
        return board.m_BlackKnight;
    }
}

template<Color white>
_COMPILETIME BitBoard Bishop(const Board& board) {
    if constexpr (white) {
        return board.m_WhiteBishop;
    } else {
        return board.m_BlackBishop;
    }
}

template<Color white>
_COMPILETIME BitBoard Rook(const Board& board) {
    if constexpr (white) {
        return board.m_WhiteRook;
    } else {
        return board.m_BlackRook;
    }
}

template<Color white>
_COMPILETIME BitBoard Queen(const Board& board) {
    if constexpr (white) {
        return board.m_WhiteQueen;
    } else {
        return board.m_BlackQueen;
    }
}

template<Color white>
_COMPILETIME BitBoard King(const Board& board) {
    if constexpr (white) {
        return board.m_WhiteKing;
    } else {
        return board.m_BlackKing;
    }
}