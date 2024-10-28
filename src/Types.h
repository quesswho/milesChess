#pragma once

using uint64 = unsigned long long;
using int64 = long long;
using uint = unsigned int;
using ushort = unsigned short;
using ubyte = unsigned char;
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