#include "board.h"
#include <immintrin.h> // _lzcnt_u64(x)
#include <stdio.h>

Board::Board(const std::string& FEN) 
    : m_WhitePawn(FenToMap(FEN, 'P')), m_WhiteKnight(FenToMap(FEN, 'N')), m_WhiteBishop(FenToMap(FEN, 'B')), m_WhiteRook(FenToMap(FEN, 'R')), m_WhiteQueen(FenToMap(FEN, 'Q')), m_WhiteKing(FenToMap(FEN, 'K')),
    m_BlackPawn(FenToMap(FEN, 'p')), m_BlackKnight(FenToMap(FEN, 'n')), m_BlackBishop(FenToMap(FEN, 'b')), m_BlackRook(FenToMap(FEN, 'r')), m_BlackQueen(FenToMap(FEN, 'q')), m_BlackKing(FenToMap(FEN, 'k')),
    m_Black(m_BlackPawn | m_BlackKnight | m_BlackBishop | m_BlackRook | m_BlackQueen | m_BlackKing), m_White(m_WhitePawn | m_WhiteKnight | m_WhiteBishop | m_WhiteRook | m_WhiteQueen | m_WhiteKing),
    m_Board(m_White | m_Black)
{}

uint64 Board::RookAttack(int pos, uint64 occ) const {
    Lookup::BlackMagic m = Lookup::r_magics[pos];
    return m.attacks[((occ | m.mask) * m.hash) >> (64 - 12)];
}

uint64 Board::BishopAttack(int pos, uint64 occ) const {
    Lookup::BlackMagic m = Lookup::b_magics[pos];
    return m.attacks[((occ | m.mask) * m.hash) >> (64 - 9)];
}

uint64 Board::QueenAttack(int pos, uint64 occ) const {
    return RookAttack(pos, occ) | BishopAttack(pos, occ);
}

uint64_t Board::RookXray(int pos, uint64_t occ) const {
    uint64_t attacks = RookAttack(pos, occ);
    return attacks ^ RookAttack(pos, occ ^ (attacks & occ));
}

uint64_t Board::BishopXray(int pos, uint64_t occ) const {
    uint64_t attacks = BishopAttack(pos, occ);
    return attacks ^ BishopAttack(pos, occ ^ (attacks & occ));
}