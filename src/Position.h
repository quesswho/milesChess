#pragma once

#include "Move.h"

#define CASTLE_WHITEKING 0b1
#define CASTLE_WHITEQUEEN 0b10
#define CASTLE_BLACKKING 0b100
#define CASTLE_BLACKQUEEN 0b1000

struct IrreversibleState {
    uint8 m_CastleRights; // bitfield 0: WhiteKing, 1: WhiteQueen, 2: BlackKing, 3: BlackQueen
    uint8 m_HalfMoves;
    BitBoard m_EnPassant; // Todo: Maybe make this to position instead of bitboard to save bytes
    uint64 m_Hash; // Stored here for the sake of counting repetition
};

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

    Color m_WhiteMove;
    uint64 m_FullMoves;

    int m_Ply;
    // Store irreversible variables
    IrreversibleState m_States[512] = { 0 }; // TODO: Will yield a crash once we reach above 512 as max moves


    // Hash values
    uint64 m_Hash;
    uint64 m_PawnHash;

    Position() = default;
    //Position(const std::string& FEN);

    /*Position(BitBoard wp, BitBoard wkn, BitBoard wb, BitBoard wr, BitBoard wq, BitBoard wk, BitBoard bp, BitBoard bkn, BitBoard bb, BitBoard br, BitBoard bq, BitBoard bk,
        ColoredPieceType square, uint64 hash, uint64 pawnhash)
        : m_WhitePawn(wp), m_WhiteKnight(wkn), m_WhiteBishop(wb), m_WhiteRook(wr), m_WhiteQueen(wq), m_WhiteKing(wk),
        m_BlackPawn(bp), m_BlackKnight(bkn), m_BlackBishop(bb), m_BlackRook(br), m_BlackQueen(bq), m_BlackKing(bk),
        m_White(wp | wkn | wb | wr | wq | wk), m_Black(bp | bkn | bb | br | bq | bk), m_Board(m_White | m_Black), 
        m_Squares(squares), m_Hash(hash), m_PawnHash(pawnhash)
    {}*/
    /*
    // Copy constructor
    Position(const Position& other)
        : m_WhitePawn(other.m_WhitePawn), m_WhiteKnight(other.m_WhiteKnight), m_WhiteBishop(other.m_WhiteBishop), m_WhiteRook(other.m_WhiteRook),
        m_WhiteQueen(other.m_WhiteQueen), m_WhiteKing(other.m_WhiteKing), m_BlackPawn(other.m_BlackPawn), m_BlackKnight(other.m_BlackKnight),
        m_BlackBishop(other.m_BlackBishop), m_BlackRook(other.m_BlackRook), m_BlackQueen(other.m_BlackQueen), m_BlackKing(other.m_BlackKing),
        m_White(other.m_White), m_Black(other.m_Black), m_Board(other.m_Board), 
        m_Squares(other.m_Squares)
        m_WhiteMove(other.m_WhiteMove), m_EnPassant(other.m_EnPassant), m_WhiteCastleKing(other.m_WhiteCastleKing), m_WhiteCastleQueen(other.m_WhiteCastleQueen),
        m_BlackCastleKing(other.m_BlackCastleKing), m_BlackCastleQueen(other.m_BlackCastleQueen), m_HalfMoves(other.m_HalfMoves), m_FullMoves(other.m_FullMoves), 
        m_Hash(other.m_Hash), m_PawnHash(other.m_PawnHash)
    {}
    */

    void SetPosition(const std::string& FEN);

    void MovePiece(Move move);
    void UndoMove(Move move);

    uint64 RookAttack(int pos, BitBoard occ) const;
    uint64 BishopAttack(int pos, BitBoard occ) const;
    uint64 QueenAttack(int pos, BitBoard occ) const;

    uint64_t RookXray(int pos, BitBoard occ) const;
    uint64_t BishopXray(int pos, BitBoard occ) const;

    std::string ToFen() const;
private:
    void SetState(const std::string& FEN);
};

static uint64 Zobrist_Hash(const Position& position);
static uint64 Zobrist_PawnHash(const Position& position);

static Move GetMove(const Position& position, std::string str);