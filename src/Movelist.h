#pragma once

#include "Board.h"

template<bool white>
_COMPILETIME BitBoard FirstRank() {
    if constexpr (white) {
        return Lookup::lines[1];
    } else {
        return Lookup::lines[4 * 56 + 1];
    }
}

_COMPILETIME BitBoard FirstRank(const bool white) {
    return white ? Lookup::lines[1] : Lookup::lines[4 * 56 + 1];
}

template<bool white>
_COMPILETIME BitBoard PawnForward(BitBoard pawns) {
    if constexpr (white) {
        return pawns << 8;
    } else {
        return pawns >> 8;
    }
}

_COMPILETIME BitBoard PawnForward(BitBoard pawns, const bool white) {
    return white ? pawns << 8 : pawns >> 8;
}

template<bool white>
_COMPILETIME BitBoard Pawn2Forward(BitBoard pawns) {
    if constexpr (white) {
        return pawns << 16;
    } else {
        return pawns >> 16;
    }
}

_COMPILETIME BitBoard Pawn2Forward(BitBoard pawns, const bool white) {
    return white ? pawns << 16 : pawns >> 16;
}

template<bool white>
_COMPILETIME BitBoard PawnRight(const Board& board) {
    if constexpr (white) {
        return (board.m_WhitePawn & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8;
    } else {
        return (board.m_BlackPawn & (~Lookup::lines[0])) >> 9;
    }
}

_COMPILETIME BitBoard PawnRight(const Board& board, const bool white) {
    if (white) {
        return (board.m_WhitePawn & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    }
    else {
        return (board.m_BlackPawn & (~Lookup::lines[0])) >> 9;
    }
}

template<bool white>
_COMPILETIME BitBoard PawnLeft(const Board& board) {
    if constexpr (white) {
        return (board.m_WhitePawn & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 8;
    } else {
        return (board.m_BlackPawn & (~Lookup::lines[7 * 4])) >> 7;
    }
}

_COMPILETIME BitBoard PawnLeft(const Board& board, const bool white) {
    if (white) {
        return (board.m_WhitePawn & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    }
    else {
        return (board.m_BlackPawn & (~Lookup::lines[7 * 4])) >> 7;
    }
}


template<bool white>
_COMPILETIME BitBoard PawnAttack(const Board& board) {
    return PawnLeft<white>(board) | PawnRight<white>(board);
}

_COMPILETIME BitBoard PawnAttack(const Board& board, const bool white) {
    return PawnLeft(board, white) | PawnRight(board, white);
}

template<bool white>
_COMPILETIME BitBoard PawnAttackRight(BitBoard pawns) {
    if constexpr (white) {
        return (pawns & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    } else {
        return (pawns & (~Lookup::lines[0])) >> 9;
    }
}


_COMPILETIME BitBoard PawnAttackRight(BitBoard pawns, const bool white) {
    if (white) {
        return (pawns & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    } else {
        return (pawns & (~Lookup::lines[0])) >> 9;
    }
}

template<bool white>
_COMPILETIME BitBoard PawnAttackLeft(BitBoard pawns) {
    if constexpr (white) {
        return (pawns & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 8
    } else {
        return (pawns & (~Lookup::lines[7 * 4])) >> 7;
    }
}

_COMPILETIME BitBoard PawnAttackLeft(BitBoard pawns, const bool white) {
    if (white) {
        return (pawns & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    } else {
        return (pawns & (~Lookup::lines[7 * 4])) >> 7;
    }
}

template<bool white>
_COMPILETIME BitBoard Player(const Board& board) {
    if constexpr (white) {
        return board.m_White;
    } else {
        return board.m_Black;
    }
}

_COMPILETIME BitBoard Player(const Board& board, const bool white) {
    return white ? board.m_White : board.m_Black;
}

template<bool white>
_COMPILETIME BitBoard Enemy(const Board& board) {
    if constexpr (white) {
        return board.m_Black;
    } else {
        return board.m_White;
    }
}

_COMPILETIME BitBoard Enemy(const Board& board, const bool white) {
    return white ? board.m_Black : board.m_White;
}

template<bool white>
_COMPILETIME BitBoard Pawn(const Board& board) {
    if constexpr (white) {
        return board.m_WhitePawn;
    } else {
        return board.m_BlackPawn;
    }
}

_COMPILETIME BitBoard Pawn(const Board& board, const bool white) {
    return white ? board.m_WhitePawn : board.m_BlackPawn;
}

template<bool white>
_COMPILETIME BitBoard Knight(const Board& board) {
    if constexpr (white) {
        return board.m_WhiteKnight;
    } else {
        return board.m_BlackKnight;
    }
}

_COMPILETIME BitBoard Knight(const Board& board, const bool white) {
    return white ? board.m_WhiteKnight : board.m_BlackKnight;
}

template<bool white>
_COMPILETIME BitBoard Bishop(const Board& board) {
    if constexpr (white) {
        return board.m_WhiteBishop;
    } else {
        return board.m_BlackBishop;
    }
}

_COMPILETIME BitBoard Bishop(const Board& board, const bool white) {
    return white ? board.m_WhiteBishop : board.m_BlackBishop;
}

template<bool white>
_COMPILETIME BitBoard Rook(const Board& board) {
    if constexpr (white) {
        return board.m_WhiteRook;
    } else {
        return board.m_BlackRook;
    }
}

_COMPILETIME BitBoard Rook(const Board& board, const bool white) {
    return white ? board.m_WhiteRook : board.m_BlackRook;
}

template<bool white>
_COMPILETIME BitBoard Queen(const Board& board) {
    if constexpr (white) {
        return board.m_WhiteQueen;
    } else {
        return board.m_BlackQueen;
    }
}

_COMPILETIME BitBoard Queen(const Board& board, const bool white) {
    return white ? board.m_WhiteQueen : board.m_BlackQueen;
}

template<bool white>
_COMPILETIME BitBoard King(const Board& board) {
    if constexpr (white) {
        return board.m_WhiteKing;
    } else {
        return board.m_BlackKing;
    }
}

_COMPILETIME BitBoard King(const Board& board, const bool white) {
    return white ? board.m_WhiteKing : board.m_BlackKing;
}