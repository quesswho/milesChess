#include "board.h"

#include <immintrin.h> // _lzcnt_u64(x)
#include <cassert>
#include <stdio.h>

Board::Board(const std::string& FEN) 
    : m_WhitePawn(FenToMap(FEN, 'P')), m_WhiteKnight(FenToMap(FEN, 'N')), m_WhiteBishop(FenToMap(FEN, 'B')), m_WhiteRook(FenToMap(FEN, 'R')), m_WhiteQueen(FenToMap(FEN, 'Q')), m_WhiteKing(FenToMap(FEN, 'K')),
    m_BlackPawn(FenToMap(FEN, 'p')), m_BlackKnight(FenToMap(FEN, 'n')), m_BlackBishop(FenToMap(FEN, 'b')), m_BlackRook(FenToMap(FEN, 'r')), m_BlackQueen(FenToMap(FEN, 'q')), m_BlackKing(FenToMap(FEN, 'k')),
    m_BoardInfo(FenInfo(FEN)), m_Black(m_BlackPawn | m_BlackKnight | m_BlackBishop | m_BlackRook | m_BlackQueen | m_BlackKing), m_White(m_WhitePawn | m_WhiteKnight | m_WhiteBishop | m_WhiteRook | m_WhiteQueen | m_WhiteKing),
    m_Board(m_White | m_Black)
{

}

uint64 Board::RookAttack(int pos, uint64 occ) {
    uint64 boardpos = 1ull << pos;
    return (Slide(boardpos, Lookup::lines[4 * pos], occ) | Slide(boardpos, Lookup::lines[4 * pos + 1], occ));
}

uint64 Board::BishopAttack(int pos, uint64 occ) {
    uint64 boardpos = 1ull << pos;
    return (Slide(boardpos, Lookup::lines[4 * pos + 2], occ) | Slide(boardpos, Lookup::lines[4 * pos + 3], occ));
}

uint64 Board::QueenAttack(int pos, uint64 occ) {
    uint64 boardpos = 1ull << pos;
    return (Slide(boardpos, Lookup::lines[4 * pos], occ) | Slide(boardpos, Lookup::lines[4 * pos + 1], occ)
        | Slide(boardpos, Lookup::lines[4 * pos + 2], occ) | Slide(boardpos, Lookup::lines[4 * pos + 3], occ));
}

uint64_t Board::RookXray(int pos, uint64_t occ) {
    uint64_t attacks = RookAttack(pos, occ);
    return attacks ^ RookAttack(pos, occ ^ (attacks & occ));
}

uint64_t Board::BishopXray(int pos, uint64_t occ) {
    uint64_t attacks = BishopAttack(pos, occ);
    return attacks ^ BishopAttack(pos, occ ^ (attacks & occ));
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

uint64 Board::Check(uint64& danger, uint64& active, uint64& rookPin, uint64& bishopPin) {
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

    // Active moves, mask for checks
    active = 0xFFFFFFFFFFFFFFFFull;
    uint64 rookcheck = RookAttack(kingsq, m_Board) & (Rook(enemy) | Queen(enemy));
    if (rookcheck > 0) { // If a rook piece is attacking the king
        active = Lookup::active_moves[kingsq * 64 + GET_SQUARE(rookcheck)];
    }
    rookPin = 0;
    uint64 rookpinner = RookXray(kingsq, m_Board) & (Rook(enemy) | Queen(enemy));
    while (rookpinner > 0) {
        uint64 mask = Lookup::active_moves[kingsq * 64 + PopPos(rookpinner)];
        if (mask & Player(white)) rookPin |= mask;
    }

    uint64 bishopcheck = BishopAttack(kingsq, m_Board) & (Bishop(enemy) | Queen(enemy));
    if (bishopcheck > 0) { // If a rook piece is attacking the king
        active = Lookup::active_moves[kingsq * 64 + GET_SQUARE(bishopcheck)];
    }

    bishopPin = 0;
    uint64 bishoppinner = BishopXray(kingsq, m_Board) & (Bishop(enemy) | Queen(enemy));
    if (bishoppinner) {
        uint64 mask = Lookup::active_moves[kingsq * 64 + PopPos(bishoppinner)];
        if (mask & Player(white)) bishopPin |= mask;
    }
    
    if (knightPawnCheck && active) { // If double checked we have to move the king
        active = Lookup::king_attacks[kingsq];
    }

    danger = PawnAttack(enemy);

    uint64 knights = Knight(enemy);

    while (knights > 0) { // Loop each bit
        danger |= Lookup::knight_attacks[PopPos(knights)];
    }

    uint64 bishops = Bishop(enemy) | Queen(enemy);

    while (bishops > 0) { // Loop each bit
        int pos = PopPos(bishops);
        danger |= (BishopAttack(pos, m_Board) & ~(1ull << pos));
    }

    uint64 rooks = Rook(enemy) | Queen(enemy);

    while (rooks > 0) { // Loop each bit
        int pos = PopPos(rooks);
        danger |= (RookAttack(pos, m_Board) & ~(1ull << pos));
    }

    danger |= Lookup::king_attacks[GET_SQUARE(King(enemy))];

    return knightPawnCheck;
}

Board Board::MovePiece(const Move& move) {
    const uint64 re = ~move.m_From;
    const uint64 swp = move.m_From | move.m_To;
    bool white = m_BoardInfo.m_WhiteMove;
    const uint64 wp = m_WhitePawn, wkn = m_WhiteKnight, wb = m_WhiteBishop, wr = m_WhiteRook, wq = m_WhiteQueen, wk = m_WhiteKing,
                 bp = m_BlackPawn, bkn = m_BlackKnight, bb = m_BlackBishop, br = m_BlackRook, bq = m_BlackQueen, bk = m_BlackKing;

    BoardInfo info = m_BoardInfo;
    info.m_WhiteMove = !info.m_WhiteMove;

    if (m_BoardInfo.m_WhiteMove) {
        assert("Cant move to same color piece", move.m_To & m_White);
        switch (move.m_Piece) {
        case Piece::PAWN:
            return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case Piece::KNIGHT:
            return Board(wp, wkn ^ swp, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case Piece::BISHOP:
            return Board(wp, wkn, wb ^ swp, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case Piece::ROOK:
            return Board(wp, wkn, wb, wr ^ swp, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case Piece::QUEEN:
            return Board(wp, wkn, wb, wr, wq ^ swp, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        case Piece::KING:
            return Board(wp, wkn, wb, wr, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk, info);
        }
    }
    else {
        assert("Cant move to same color piece", move.m_To & m_Black);
        switch (move.m_Piece) {
        case Piece::PAWN:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk, info);
        case Piece::KNIGHT:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn ^ swp, bb, br, bq, bk, info);
        case Piece::BISHOP:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb ^ swp, br, bq, bk, info);
        case Piece::ROOK:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ swp, bq, bk, info);
        case Piece::QUEEN:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq ^ swp, bk, info);
        case Piece::KING:
            return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq, bk ^ swp, info);
        }
    }
}

std::vector<Move> Board::GenerateMoves() {
    std::vector<Move> result;

    bool white = m_BoardInfo.m_WhiteMove;
    bool enemy = !white;
    uint64 danger = 0, active = 0, rookPin = 0, bishopPin = 0;
    uint64 check = Check(danger, active, rookPin, bishopPin);
    uint64 moveable = ~Player(white) & active;

    // Pawns

    uint64 pawns = Pawn(white);
    uint64 FPawns = PawnForward(pawns & ~rookPin, white) & (~m_Board) & active;
    uint64 F2Pawns = PawnForward(pawns & Lookup::StartingPawnRank(white), white) & ~m_Board;
    F2Pawns = PawnForward(F2Pawns, white) & (~m_Board) & active;

    for (; uint64 bit = PopBit(FPawns); FPawns > 0) { // Loop each bit
        result.push_back({ PawnForward(bit, enemy), bit, Piece::PAWN });
    }

    for (; uint64 bit = PopBit(F2Pawns); F2Pawns > 0) { // Loop each bit
        result.push_back({ Pawn2Forward(bit, enemy), bit, Piece::PAWN });
    }

    uint64 RPawns = PawnRight(white) & Enemy(white);
    uint64 LPawns = PawnLeft(white) & Enemy(white);

    for (; uint64 bit = PopBit(RPawns); RPawns > 0) { // Loop each bit
        result.push_back({ PawnAttackRight(bit, enemy), bit, Piece::PAWN });
    }

    for (; uint64 bit = PopBit(LPawns); LPawns > 0) { // Loop each bit
        result.push_back({ PawnAttackLeft(bit, enemy), bit, Piece::PAWN });
    }

    // Kinghts

    uint64 knights = Knight(white);

    while (knights > 0) { // Loop each bit
        int pos = PopPos(knights);
        uint64 moves = Lookup::knight_attacks[pos] & moveable;
        for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
            result.push_back({ 1ull << pos, bit, Piece::KNIGHT });
        }
    }

    // Bishops

    uint64 bishops = Bishop(white);

    while (bishops > 0) {
        int pos = PopPos(bishops);
        uint64 moves = BishopAttack(pos, m_Board) & moveable;
        for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
            result.push_back({ 1ull << pos, bit, Piece::BISHOP });
        }
    }

    // Rooks
    uint64 rooks = Rook(white);

    while (rooks > 0) {
        int pos = PopPos(rooks);
        uint64 moves = RookAttack(pos, m_Board) & moveable;
        for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
            result.push_back({ 1ull << pos, bit, Piece::ROOK });
        }
    }

    // Queens
    uint64 queens = Queen(white);

    while (queens > 0) {
        int pos = PopPos(queens);
        uint64 moves = QueenAttack(pos, m_Board) & moveable;
        for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
            result.push_back({ 1ull << pos, bit, Piece::QUEEN });
        }
    }

     // Loop each bit
    int kingpos = GET_SQUARE(King(white));
    uint64 moves = Lookup::king_attacks[kingpos] & ~Player(white);
    moves &= ~danger;
    for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
        //PrintMap(m_Board);
        result.push_back({ 1ull << kingpos, bit, Piece::KING });
    }
    

    return result;
}