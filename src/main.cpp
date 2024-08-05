#include<stdio.h>
#include "board.h"


const char* g_StartingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";



int main() {
	Lookup::Init();

	//Board board("rnbqkbnr/pppppppp/P7/8/8/8/RPPPPPPP/1NBQKBNR w KQkq - 0 1");
	Board board("P7/1P6/2P5/3P4/4P3/5P2/6P1/7P w KQkq - 0 1");
	//printf("%s\n", BoardtoFen(board).c_str());
	
	//PrintMap(board.Slide(1 << 15, Lookup::InitFile(0), board.m_Board));

	/*PrintMap(board.m_WhitePawn);
	printf("\n");
	PrintMap(Lookup::InitFile(20) | Lookup::InitRank(20));
	printf("\n");
	PrintMap(Lookup::InitRank(20));
	*/
	for (int i = 0; i < 24; i++) {
		printf("Pos:\n");
		PrintMap(1ull << i);
		printf("knight:\n");
		PrintMap(Lookup::knight_attacks[i]);
	}

	PrintMap(1ull << Lookup::CoordToPos(4, 4));
	

	//printf("%i, %i, %i\n", board.m_BoardInfo.m_BlackCastleKing, board.m_BoardInfo.m_BlackCastleQueen, board.m_BoardInfo.m_WhiteCastleKing);
	scanf("%i");
}