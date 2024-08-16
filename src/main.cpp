#include<stdio.h>
#include "Move.h"


const char* g_StartingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";



int main() {
	//Lookup::PrintLineTable();
	//Lookup::PrintKnightTable();
	//Lookup::PrintKingTable();
//	Board board("rnbqkbnr/pppppppp/P7/8/8/8/RPPPPPPP/1NBQKBNR w KQkq - 0 1");
	//Board board(g_StartingFEN);
	//Board board("P7/1P6/2P5/3P4/4P3/5P2/6P1/7P w KQkq - 0 1");
	//printf("%s\n", BoardtoFen(board).c_str());
	//PrintMap(Slide(1ull << 6, Lookup::lines[6 * 4], board.m_Board));
	//PrintMap(Slide(1ull << 6, Lookup::linesEx[6*4], board.m_Board));
	//PrintMap(board.RookAttack(7, board.m_Board));

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
	//Lookup::PrintCheckMoves();
	//printf("%llu\n", Perft("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1)); 
	//printf("%llu\n", Perft("8/8/8/1r3P1K/8/8/1k6/8 w - - 0 1", 1));
	//printf("%llu\n", Perft("8/1r6/8/8/3P2k1/1P6/1KP3r1/8 w - - 0 1", 1));
	//printf("%llu\n", Perft("8/8/6k1/1p2p3/3pP3/2pK1b2/4P3/5b2 w - - 0 1", 1));
	//printf("%llu\n", Perft("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 2));
	
	
	//printf("%llu\n", Perft("r3k2r/p1ppqpb1/bn2pQp1/3PN3/1p2P3/2N4p/PPPBBPPP/R3K2R b KQkq - 0 1", 2));
	//printf("%llu\n", Perft("r3k3/p1ppqpb1/bn2pQp1/3PN3/1p2P2r/2N4p/PPPBBPPP/R3K2R w KQq - 1 2", 1));
	//printf("%llu\n", Perft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3));

	//printf("%llu\n", Perft("rnbqkbnr/1ppppppp/p7/1B6/4P3/8/PPPP1PPP/RNBQK1NR b KQkq - 1 2", 1));
	
	//printf("%llu\n", Perft("8/8/3p4/1Pp4r/KR3p1k/8/4P1P1/8 w - c6 0 2", 2));


	printf("%llu\n", Perft("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 2", 4));
	//printf("%llu\n", Perft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4));
	//printf("%llu\n", Perft(g_StartingFEN, 5));


	//Slide(1 << 38, Lookup::lines[], uint64 block) {

	scanf("%i");
}