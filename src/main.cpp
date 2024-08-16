#include<stdio.h>
#include "Move.h"


const char* g_StartingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";



int main() {


//	Board board("rnbqkbnr/pppppppp/P7/8/8/8/RPPPPPPP/1NBQKBNR w KQkq - 0 1");
	
	printf("%llu\n", Perft("3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1", 1));
	//printf("%llu\n", Perft("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5));
	//printf("%llu\n", Perft("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4)); // TODO: fails at depth 5
	//printf("%llu\n", Perft("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6));
	//printf("%llu\n", Perft("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 7));
	//printf("%llu\n", Perft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5));
	//printf("%llu\n", Perft(g_StartingFEN, 6));

	scanf("%i");
}