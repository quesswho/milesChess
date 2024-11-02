#pragma once

using uint64 = unsigned long long;
using int64 = long long;
using uint32 = unsigned int;
using ushort = unsigned short;
using ubyte = unsigned char;
using uint8 = uint8_t;
using int8 = int8_t;


#define _COMPILETIME static inline constexpr

/*
    Boards start from bottom right and go right to left

    64 63 ... 57 56
    .
    .
    .
    8 7 ... 3 2 1

*/
using BitBoard = uint64;

enum Color : bool {
    WHITE = true,
    BLACK = false
};

constexpr Color operator!(Color color) {
    return color == WHITE ? BLACK : WHITE;
}

struct BoardInfo {
    BoardInfo()
        : m_WhiteMove(WHITE), m_EnPassant(0), m_WhiteCastleKing(true), m_WhiteCastleQueen(true), m_BlackCastleKing(true), m_BlackCastleQueen(true), m_HalfMoves(0), m_FullMoves(0)
    {}

    Color m_WhiteMove;
    BitBoard m_EnPassant;

    bool m_WhiteCastleKing;
    bool m_WhiteCastleQueen;

    bool m_BlackCastleKing;
    bool m_BlackCastleQueen;

    uint64 m_HalfMoves;
    uint64 m_FullMoves;
};