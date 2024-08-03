#include<stdio.h>
#include "board.h"


const char* g_StartingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

int main() {
    Board board(g_StartingFEN);


	printf("%s\n", BoardtoFen(board).c_str());
	
	//printf("%i, %i, %i\n", board.m_BoardInfo.m_BlackCastleKing, board.m_BoardInfo.m_BlackCastleQueen, board.m_BoardInfo.m_WhiteCastleKing);
	scanf("%i");
}