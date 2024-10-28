#pragma once

#include <string>

#include "Types.h"


class Position {
public:

    union {
        struct {
            BitBoard m_White;
            BitBoard m_Black;

            const BitBoard m_WhitePawn;
            const BitBoard m_BlackPawn;

            const BitBoard m_WhiteKnight;
            const BitBoard m_BlackKnight;

            const BitBoard m_WhiteBishop;
            const BitBoard m_BlackBishop;

            const BitBoard m_WhiteRook;
            const BitBoard m_BlackRook;

            const BitBoard m_WhiteQueen;
            const BitBoard m_BlackQueen;

            const BitBoard m_WhiteKing;
            const BitBoard m_BlackKing;
        };
        const BitBoard m_Pieces[7][2]; // 6 pieces for each color: 0 for white and 1 for black

    };
    BitBoard m_Board;


    // Board info
    Color m_WhiteMove;
    uint64 m_EnPassant;

    bool m_WhiteCastleKing;
    bool m_WhiteCastleQueen;

    bool m_BlackCastleKing;
    bool m_BlackCastleQueen;

    uint64 m_HalfMoves;
    uint64 m_FullMoves;

    // Hash values
    uint64 m_Hash;
    uint64 m_PawnHash;

    Position(const std::string& FEN);

    Position(BitBoard wp, BitBoard wkn, BitBoard wb, BitBoard wr, BitBoard wq, BitBoard wk, BitBoard bp, BitBoard bkn, BitBoard bb, BitBoard br, BitBoard bq, BitBoard bk, Color color, uint64 enPassant)
        : m_WhitePawn(wp), m_WhiteKnight(wkn), m_WhiteBishop(wb), m_WhiteRook(wr), m_WhiteQueen(wq), m_WhiteKing(wk),
        m_BlackPawn(bp), m_BlackKnight(bkn), m_BlackBishop(bb), m_BlackRook(br), m_BlackQueen(bq), m_BlackKing(bk),
        m_White(wp | wkn | wb | wr | wq | wk), m_Black(bp | bkn | bb | br | bq | bk), m_Board(m_White | m_Black)
    {}

    Position(const Position& other)
        : m_WhitePawn(other.m_WhitePawn), m_WhiteKnight(other.m_WhiteKnight), m_WhiteBishop(other.m_WhiteBishop), m_WhiteRook(other.m_WhiteRook),
        m_WhiteQueen(other.m_WhiteQueen), m_WhiteKing(other.m_WhiteKing), m_BlackPawn(other.m_BlackPawn), m_BlackKnight(other.m_BlackKnight),
        m_BlackBishop(other.m_BlackBishop), m_BlackRook(other.m_BlackRook), m_BlackQueen(other.m_BlackQueen), m_BlackKing(other.m_BlackKing),
        m_White(other.m_White), m_Black(other.m_Black), m_Board(other.m_Board), 
        m_WhiteMove(other.m_WhiteMove), m_EnPassant(other.m_EnPassant), m_WhiteCastleKing(other.m_WhiteCastleKing), m_WhiteCastleQueen(other.m_WhiteCastleQueen),
        m_BlackCastleKing(other.m_BlackCastleKing), m_BlackCastleQueen(other.m_BlackCastleQueen), m_HalfMoves(other.m_HalfMoves), m_FullMoves(other.m_FullMoves), 
        m_Hash(other.m_Hash), m_PawnHash(other.m_PawnHash)
    {}

    uint64 RookAttack(int pos, BitBoard occ) const;
    uint64 BishopAttack(int pos, BitBoard occ) const;
    uint64 QueenAttack(int pos, BitBoard occ) const;

    uint64_t RookXray(int pos, BitBoard occ) const;
    uint64_t BishopXray(int pos, BitBoard occ) const;
};