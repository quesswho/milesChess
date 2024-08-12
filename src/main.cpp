#include<stdio.h>
#include "Move.h"


const char* g_StartingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";



int main() {
	//Lookup::PrintLineTable();
	//Lookup::PrintKnightTable();
	//Lookup::PrintKingTable();
	//Board board("rnbqkbnr/pppppppp/P7/8/8/8/RPPPPPPP/1NBQKBNR w KQkq - 0 1");
	//Board board(g_StartingFEN);
	//Board board("P7/1P6/2P5/3P4/4P3/5P2/6P1/7P w KQkq - 0 1");
	//printf("%s\n", BoardtoFen(board).c_str());
	
	//PrintMap(board.Slide(1 << 15, Lookup::InitFile(0), board.m_Board));

	/*PrintMap(board.m_WhitePawn);
	printf("\n");
	PrintMap(Lookup::InitFile(20) | Lookup::InitRank(20));
	printf("\n");
	PrintMap(Lookup::InitRank(20));
	*/
	/*for (int i = 0; i < 24; i++) {
		printf("Pos:\n");
		PrintMap(1ull << i);
		printf("knight:\n");
		PrintMap(Lookup::knight_attacks[i]);
	}

	*/
	//PrintMap(Lookup::StartingPawnRank(true));
	//PrintMap(Lookup::StartingPawnRank(false));
	//printf("\n");
	/*PrintMap(board.m_WhitePawn);
	printf("\n");
	PrintMap(board.PawnAttack(true));
	*/
	//printf("%i, %i, %i\n", board.m_BoardInfo.m_BlackCastleKing, board.m_BoardInfo.m_BlackCastleQueen, board.m_BoardInfo.m_WhiteCastleKing);
	//Lookup::PrintActiveMoves();
	//printf("%llu\n", Perft("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1)); 
	//printf("%llu\n", Perft("8/6k1/8/8/8/2b5/1K6/8 w - - 0 1", 1));
	printf("%llu\n", Perft(g_StartingFEN, 4)); // Perft(4)
	scanf("%i");
}