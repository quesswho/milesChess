#include<stdio.h>
#include "board.h"



int main() {
    Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

	printf("%i, %i, %i\n", board.m_BoardInfo.m_BlackCastleKing, board.m_BoardInfo.m_BlackCastleQueen, board.m_BoardInfo.m_WhiteCastleKing);
	scanf("%i");
}