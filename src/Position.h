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
    uint64 m_Hash; // TODO: Why do we have this when we already store it in m_States
    uint64 m_PawnHash;

    Position() = default;

    void SetPosition(const std::string& FEN);

    void MovePiece(Move move);
    void UndoMove(Move move);

    void NullMove();
    void UndoNullMove();

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