#pragma once

#include <string>

#include "Types.h"


enum ColoredPieceType : uint8 {
    NONE = 0, WPAWN = 1, WKNIGHT = 2, WBISHOP = 3, WROOK = 4, WQUEEN = 5, WKING = 6,
    BPAWN = 7, BKNIGHT = 8, BBISHOP = 9, BROOK = 10, BQUEEN = 11, BKING = 12
};

// Move:
// from: 6 bits
// to:   6 bits
// PieceType: 4 bits
// CaptureType: 4 bits
// MoveFlag: EP, Castle, PN, PB, PR, PQ, 5 bits
// 
// Total is 25 bits

using Move = uint32;

static inline int From(Move move) {
    return (int) (move & 0x3F);
}

static inline int To(Move move) {
    return (int)((move >> 6) & 0x3F);
}

static inline ColoredPieceType PieceType(Move move) {
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

static inline bool Promotion(Move move) {
    return (bool)(move & 0x3C00000);
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