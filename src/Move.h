#pragma once

#include <string>

#include "Utils.h"


enum ColoredPieceType : uint8 {
    NOPIECE = 0, WPAWN = 1, WKNIGHT = 2, WBISHOP = 3, WROOK = 4, WQUEEN = 5, WKING = 6,
    BPAWN = 7, BKNIGHT = 8, BBISHOP = 9, BROOK = 10, BQUEEN = 11, BKING = 12
};


template<Color c>
static inline constexpr ColoredPieceType GetColoredPiece(PieceType T) {
    constexpr ColoredPieceType pieceMap[2][7] = {
        { NOPIECE, WPAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, WKING },
        { NOPIECE, BPAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, BKING }
    };
    return pieceMap[c][T];
}

// Move:
// from: 6 bits
// to:   6 bits
// PieceType: 4 bits
// CaptureType: 4 bits
// MoveFlag: EP, Castle, PN, PB, PR, PQ, 6 bits
// 
// Total is 26 bits

using Move = uint32;

static inline Move BuildMove(uint8 from, uint8 to, ColoredPieceType type) {
    return from | to << 6 | type << 12;
}

static inline Move BuildMove(uint8 from, uint8 to, ColoredPieceType type, ColoredPieceType capture) {
    return from | to << 6 | type << 12 | capture << 16;
}

static inline Move BuildMove(uint8 from, uint8 to, ColoredPieceType type, ColoredPieceType capture, uint8 flags) {
    return from | to << 6 | type << 12 | capture << 16 | flags << 20;
}

static inline int From(Move move) {
    return (int) (move & 0x3F);
}

static inline int To(Move move) {
    return (int)((move >> 6) & 0x3F);
}

static inline ColoredPieceType MovePieceType(Move move) {
    return (ColoredPieceType)((move >> 12) & 0xF);
}

static inline ColoredPieceType CaptureType(Move move) {
    return (ColoredPieceType)((move >> 16) & 0xF);
}

static inline bool EnPassant(Move move) {
    return (bool)(move & 0x100000);
}

static inline bool Castle(Move move) {
    return (bool)(move & 0x200000);
}

static inline int Promotion(Move move) {
    return (int)(move & 0x3C00000);
}

static inline bool PromoteKnight(Move move) {
    return (bool)(move & 0x400000);
}

static inline bool PromoteBishop(Move move) {
    return (bool)(move & 0x800000);
}

static inline bool PromoteRook(Move move) {
    return (bool)(move & 0x1000000);
}

static inline bool PromoteQueen(Move move) {
    return (bool)(move & 0x2000000);
}

static inline std::string MoveToString(Move move) {
    std::string result;
    result.reserve(4); // Normal moves have 4 characters
    int f = From(move);
    result += 'h' - (f % 8);
    result += '1' + (f / 8);
    int t = To(move);
    result += 'h' - (t % 8);
    result += '1' + (t / 8);
    int promotion = Promotion(move);
    switch (promotion) {
    case 0x400000:
        result += 'N';
        break;
    case 0x800000:
        result += 'B';
        break;
    case 0x1000000:
        result += 'R';
        break;
    case 0x2000000:
        result += 'Q';
        break;
    case 0: // No promotion
        break;
    }
    return result;
}