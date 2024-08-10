#include "board.h"

#include <immintrin.h> // _lzcnt_u64(x)

#include <stdio.h>

Board::Board(const std::string& FEN) 
    : m_WhitePawn(FenToMap(FEN, 'P')), m_WhiteKnight(FenToMap(FEN, 'N')), m_WhiteBishop(FenToMap(FEN, 'B')), m_WhiteRook(FenToMap(FEN, 'R')), m_WhiteQueen(FenToMap(FEN, 'Q')), m_WhiteKing(FenToMap(FEN, 'K')),
    m_BlackPawn(FenToMap(FEN, 'p')), m_BlackKnight(FenToMap(FEN, 'n')), m_BlackBishop(FenToMap(FEN, 'b')), m_BlackRook(FenToMap(FEN, 'r')), m_BlackQueen(FenToMap(FEN, 'q')), m_BlackKing(FenToMap(FEN, 'k')),
    m_BoardInfo(FenInfo(FEN)), m_Black(m_BlackPawn | m_BlackKnight | m_BlackBishop | m_BlackRook | m_BlackQueen | m_BlackKing), m_White(m_WhitePawn | m_WhiteKnight | m_WhiteBishop | m_WhiteRook | m_WhiteQueen | m_WhiteKing),
    m_Board(m_White | m_Black)
{

}


uint64 Board::Slide(uint64 pos, uint64 line, uint64 block) {
    block = block & ~pos & line;

    uint64 bottom = pos - 1ull;

    uint64 mask_up = block & ~bottom;
    uint64 block_up = (mask_up ^ (mask_up - 1ull));

    uint64 mask_down = block & bottom;
    uint64 block_down = (0x7FFFFFFFFFFFFFFF) >> _lzcnt_u64(mask_down | 1);

    return (block_up ^ block_down) & line;
}

uint64 Board::RookAttack(int pos, uint64 occ) {
    uint64 boardpos = 1ull << pos;
    return (Slide(boardpos, Lookup::lines[4 * pos], occ) | Slide(boardpos, Lookup::lines[4 * pos + 1], occ));
}

uint64 Board::BishopAttack(int pos, uint64 occ) {
    uint64 boardpos = 1ull << pos;
    return (Slide(boardpos, Lookup::lines[4 * pos+2], occ) | Slide(boardpos, Lookup::lines[4 * pos + 1+3], occ));
}

uint64 Board::QueenAttack(int pos, uint64 occ) {
    uint64 boardpos = 1ull << pos;
    return (Slide(boardpos, Lookup::lines[4 * pos], occ) | Slide(boardpos, Lookup::lines[4 * pos + 1], occ)
        | Slide(boardpos, Lookup::lines[4 * pos + 2], occ) | Slide(boardpos, Lookup::lines[4 * pos + 1 + 3], occ));
}

uint64 Board::PawnForward(uint64 pawns, bool white) {
    return white ? pawns << 8 : pawns >> 8;
}

uint64 Board::Pawn2Forward(uint64 pawns, bool white) {
    return white ? pawns << 16 : pawns >> 16;
}


uint64 Board::PawnRight(bool white) {
    if (white) {
        return (m_WhitePawn & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    } else {
        return (m_BlackPawn & (~Lookup::lines[0])) >> 9;
    }
}

uint64 Board::PawnLeft(bool white) {
    if (white) {
        return (m_WhitePawn & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    }
    else {
        return (m_BlackPawn & (~Lookup::lines[7 * 4])) << 7;
    }
}

uint64 Board::PawnAttack(bool white) {
    return PawnLeft(white) | PawnRight(white);
}

uint64 Board::PawnAttackRight(uint64 pawns, bool white) {
    if (white) {
        return (pawns & (~Lookup::lines[0])) << 7; // Remove pawns at rank 8
    }
    else {
        return (pawns & (~Lookup::lines[0])) >> 9;
    }
}
uint64 Board::PawnAttackLeft(uint64 pawns, bool white) {
    if (white) {
        return (pawns & (~Lookup::lines[7 * 4])) << 9; // Remove pawns at rank 1
    }
    else {
        return (pawns & (~Lookup::lines[7 * 4])) << 7;
    }
}

bool Board::Validate() {
    uint64 danger = PawnAttack(m_BoardInfo.m_WhiteMove);
    if (m_BoardInfo.m_WhiteMove) {
        // TODO: Function to iterate every active bit in a bit map needed here
        //danger |= m_BlackKnight;
    }
    return true;
}

uint64 Board::Check() {
    bool white = m_BoardInfo.m_WhiteMove;
    bool enemy = !white;

    int kingsq = GET_SQUARE(King(white));

    uint64 knightPawnCheck = Lookup::knight_attacks[kingsq] & Knight(enemy);

    uint64 pr = PawnRight(enemy) & kingsq;
    uint64 pl = PawnLeft(enemy) & kingsq;
    if (pr > 0) {
        knightPawnCheck |= PawnAttackLeft(pr, white); // Reverse the pawn attack to find the attacking pawn
    }
    if(pl > 0) {
        knightPawnCheck |= PawnAttackRight(pl, white);
    }

    return knightPawnCheck;
}

std::vector<Move> Board::GenerateMoves() {
    std::vector<Move> result;

    bool white = m_BoardInfo.m_WhiteMove;
    bool enemy = !white;

    // Pawns

    uint64 pawns = Pawn(white);
    uint64 FPawns = PawnForward(pawns, white) & (~m_Board);
    uint64 F2Pawns = PawnForward(pawns & Lookup::StartingPawnRank(white), white) & ~m_Board;
    F2Pawns = PawnForward(F2Pawns, white) & ~m_Board;

    for (; uint64 bit = PopBit(FPawns); FPawns > 0) { // Loop each bit
        result.push_back({ PawnForward(bit, enemy), bit, Piece::PAWN });
    }

    for (; uint64 bit = PopBit(F2Pawns); F2Pawns > 0) { // Loop each bit
        result.push_back({ Pawn2Forward(bit, enemy), bit, Piece::PAWN });
    }

    // Kinghts

    uint64 knights = Knight(white);

    while (knights > 0) { // Loop each bit
        int pos = PopPos(knights);
        uint64 moves = Lookup::knight_attacks[pos] & ~Player(white);
        for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
            result.push_back({ 1ull << pos, bit, Piece::KNIGHT });
        }
    }

    return result;
}