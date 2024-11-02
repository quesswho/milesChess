#pragma once

#include "Move.h"



class Position {
public:

    union {
        struct {
            BitBoard m_White;
            BitBoard m_Black;

            BitBoard m_WhitePawn;
            BitBoard m_BlackPawn;

            BitBoard m_WhiteKnight;
            BitBoard m_BlackKnight;

            BitBoard m_WhiteBishop;
            BitBoard m_BlackBishop;

            BitBoard m_WhiteRook;
            BitBoard m_BlackRook;

            BitBoard m_WhiteQueen;
            BitBoard m_BlackQueen;

            BitBoard m_WhiteKing;
            BitBoard m_BlackKing;
        };
        BitBoard m_Pieces[7][2]; // 6 pieces for each color: 0 for white and 1 for black

    };
    BitBoard m_Board;

    ColoredPieceType m_Squares[64];

    // Board info
    union {
        struct {
            Color m_WhiteMove;
            uint64 m_EnPassant;

            bool m_WhiteCastleKing;
            bool m_WhiteCastleQueen;

            bool m_BlackCastleKing;
            bool m_BlackCastleQueen;

            uint64 m_HalfMoves;
            uint64 m_FullMoves;
        };
        BoardInfo m_BoardInfo;
    };

    // Hash values
    uint64 m_Hash;
    uint64 m_PawnHash;

    Position(const std::string& FEN);

    Position(BitBoard wp, BitBoard wkn, BitBoard wb, BitBoard wr, BitBoard wq, BitBoard wk, BitBoard bp, BitBoard bkn, BitBoard bb, BitBoard br, BitBoard bq, BitBoard bk, const BoardInfo& info, uint64 hash, uint64 pawnhash)
        : m_WhitePawn(wp), m_WhiteKnight(wkn), m_WhiteBishop(wb), m_WhiteRook(wr), m_WhiteQueen(wq), m_WhiteKing(wk),
        m_BlackPawn(bp), m_BlackKnight(bkn), m_BlackBishop(bb), m_BlackRook(br), m_BlackQueen(bq), m_BlackKing(bk),
        m_White(wp | wkn | wb | wr | wq | wk), m_Black(bp | bkn | bb | br | bq | bk), m_Board(m_White | m_Black), 
        m_BoardInfo(info), m_Hash(hash), m_PawnHash(pawnhash)
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

    void MovePiece(Move move);

    uint64 RookAttack(int pos, BitBoard occ) const;
    uint64 BishopAttack(int pos, BitBoard occ) const;
    uint64 QueenAttack(int pos, BitBoard occ) const;

    uint64_t RookXray(int pos, BitBoard occ) const;
    uint64_t BishopXray(int pos, BitBoard occ) const;
};

static uint64 Zobrist_Hash(const Position& position);
static uint64 Zobrist_PawnHash(const Position& position);

static Move GetMove(const Position& position, std::string str);