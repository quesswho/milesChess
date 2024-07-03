#include "board.h"

Board::Board() {
	StartingPosition();
}

void Board::StartingPosition() {

	// Clear the board
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			m_Board[i][j] = 0;
		}
	}

	m_Board[0][0] = Piece::WHITE_ROOK;
	m_Board[7][0] = Piece::WHITE_ROOK;
	m_Board[1][0] = Piece::WHITE_KNIGHT;
	m_Board[6][0] = Piece::WHITE_KNIGHT;
	m_Board[2][0] = Piece::WHITE_BISHOP;
	m_Board[5][0] = Piece::WHITE_BISHOP;
	m_Board[3][0] = Piece::WHITE_QUEEN;
	m_Board[4][0] = Piece::WHITE_KING;

	for (int i = 0; i < 8; i++) {
		m_Board[i][1] = Piece::WHITE_PAWN;
	}


	m_Board[0][7] = Piece::BLACK_ROOK;
	m_Board[7][7] = Piece::BLACK_ROOK;
	m_Board[1][7] = Piece::BLACK_KNIGHT;
	m_Board[6][7] = Piece::BLACK_KNIGHT;
	m_Board[2][7] = Piece::BLACK_BISHOP;
	m_Board[5][7] = Piece::BLACK_BISHOP;
	m_Board[3][7] = Piece::BLACK_QUEEN;
	m_Board[4][7] = Piece::BLACK_KING;

	for (int i = 0; i < 8; i++) {
		m_Board[i][6] = Piece::BLACK_PAWN;
	}
}
