#include "board.h"

#include <stdio.h>

Board::Board(const std::string& FEN) 
    : m_WhitePawn(FenToMap(FEN, 'P')), m_WhiteKnight(FenToMap(FEN, 'N')), m_WhiteBishop(FenToMap(FEN, 'B')), m_WhiteRook(FenToMap(FEN, 'R')), m_WhiteQueen(FenToMap(FEN, 'Q')), m_WhiteKing(FenToMap(FEN, 'K')),
    m_BlackPawn(FenToMap(FEN, 'P')), m_BlackKnight(FenToMap(FEN, 'N')), m_BlackBishop(FenToMap(FEN, 'B')), m_BlackRook(FenToMap(FEN, 'R')), m_BlackQueen(FenToMap(FEN, 'Q')), m_BlackKing(FenToMap(FEN, 'K')),
    m_BoardInfo(FenInfo(FEN))

{}