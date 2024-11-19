#pragma once

#include "Move.h"
#include "LookupTables.h"

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

    //
    bool m_InCheck;

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

    template<Color white>
    bool UpdateChecks() const {
        constexpr Color enemy = !white;
        BoardPos kingsq = GET_SQUARE(King<white>());

        if (
            (Lookup::knight_attacks[kingsq] & Knight<enemy>())                                         // Knight check
            || (PawnRight<enemy>() & King<white>())                                               // Pawn check
            || (PawnLeft<enemy>() & King<white>())                                                // Pawn check
            || (BishopAttack(kingsq, m_Board) & (Bishop<enemy>() | Queen<enemy>()))   // Bishop check
            || (RookAttack(kingsq, m_Board) & (Rook<enemy>() | Queen<enemy>()))       // Rook check
            ) {
            return true;
        }

        return false;
    }

    template<Color white>
    _COMPILETIME BitBoard PawnRight() const {
        if constexpr (white) {
            return (m_WhitePawn & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8;
        } else {
            return (m_BlackPawn & (~Lookup::lines[0])) >> 9;
        }
    }

    template<Color white>
    _COMPILETIME BitBoard PawnLeft() const {
        if constexpr (white) {
            return (m_WhitePawn & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 8;
        } else {
            return (m_BlackPawn & (~Lookup::lines[7 * 4])) >> 7;
        }
    }

    template<Color white>
    _COMPILETIME BitBoard Pawn() const {
        if constexpr (white) {
            return m_WhitePawn;
        } else {
            return m_BlackPawn;
        }
    }

    template<Color white>
    _COMPILETIME BitBoard Knight() const {
        if constexpr (white) {
            return m_WhiteKnight;
        } else {
            return m_BlackKnight;
        }
    }

    template<Color white>
    _COMPILETIME BitBoard Bishop() const {
        if constexpr (white) {
            return m_WhiteBishop;
        } else {
            return m_BlackBishop;
        }
    }

    template<Color white>
    _COMPILETIME BitBoard Rook() const {
        if constexpr (white) {
            return m_WhiteRook;
        } else {
            return m_BlackRook;
        }
    }

    template<Color white>
    _COMPILETIME BitBoard Queen() const {
        if constexpr (white) {
            return m_WhiteQueen;
        } else {
            return m_BlackQueen;
        }
    }

    template<Color white>
    _COMPILETIME BitBoard King() const {
        if constexpr (white) {
            return m_WhiteKing;
        } else {
            return m_BlackKing;
        }
    }
};

static uint64 Zobrist_Hash(const Position& position);
static uint64 Zobrist_PawnHash(const Position& position);

static Move GetMove(const Position& position, std::string str);