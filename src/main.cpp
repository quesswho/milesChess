#include<stdio.h>
#include "Move.h"


const char* g_StartingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";



int main() {


	//Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	//Board board("rnbqkbnr/ppp1pppp/8/3p4/8/7P/PPPPPPP1/RNBQKBNR w KQkq - 0 2");
	//Board board("rnbqkbnr/ppp2ppp/8/3pp3/8/6PP/PPPPPP2/RNBQKBNR w KQkq - 0 3");
	//Board board("r1bqkbnr/ppp2ppp/2n5/3pp3/8/5PPP/PPPPP3/RNBQKBNR w KQkq - 1 4");
	//Board board("r1b1kbnr/ppp2ppp/2n5/3pp1q1/8/4PPPP/PPPP4/RNBQKBNR w KQkq - 1 5");
	//Board board("r1b1kbnr/ppp2ppp/2n5/3pp3/8/3PPPqP/PPP5/RNBQKBNR w KQkq - 0 6");
	//Board board("r1b1k1nr/ppp2ppp/2n5/2bpp3/8/3PPPqP/PPP1K3/RNBQ1BNR w kq - 2 7");
	//Board board("r1b1k2r/ppp2ppp/2n2n2/2bpp3/8/2PPPPqP/PP2K3/RNBQ1BNR w kq - 1 8");
	//Board board("r1b1k2r/ppp2ppp/2n2n2/2bp4/4p3/1PPPPPqP/P3K3/RNBQ1BNR w kq - 0 9");
	//Board board("r1b1k2r/ppp2ppp/2n2n2/2bp4/8/PPPpPPqP/4K3/RNBQ1BNR w kq - 0 10");
	//Board board("r1b2rk1/ppp2ppp/2n2n2/2bp4/8/PPPQPPqP/4K3/RNB2BNR w - - 1 11");
	//Board board("r1b1r1k1/ppp2ppp/2n2n2/2bp4/7P/PPPQPPq1/4K3/RNB2BNR w - - 1 12");
	//Board board("r1b1r1k1/ppp2ppp/2n5/2bp4/5PnP/PPPQP1q1/4K3/RNB2BNR w - - 1 13");
	//Board board("r1b1r1k1/ppp2ppp/2n5/2bp4/4PPnP/PPPQ4/4Kq2/RNB2BNR w - - 1 14");
	//Board board("r1b1r1k1/ppp2ppp/2n5/2bp4/4PP1P/PPPQn3/5q2/RNBK1BNR w - - 3 15");
	//Board board("r1b1r1k1/ppp2ppp/2n5/3p4/4PP1P/PPPQb3/5q2/RN1K1BNR w - - 0 16");
	//Board board("r1b1r1k1/ppp2ppp/8/3p4/2PnPP1P/PP1Qb3/5q2/RN1K1BNR w - - 1 17");
	Board board("r1b1r1k1/ppp2ppp/8/8/1PPnpP1P/P2Qb3/5q2/RN1K1BNR w - - 0 18");
	//Board board("r1b1r1k1/ppp2ppp/8/8/PPPn1P1P/3pb3/5q2/RN1K1BNR w - - 0 19");
	//Board board("r1b1r1k1/ppp2ppp/8/8/PPPn1P1P/4b3/4pq2/RN1K1B1R w - - 0 20");
	//Board board("r1b1r1k1/ppp2ppp/8/8/PPPn1P1P/4b3/4q3/RN1K3R w - - 0 21");


	
	//printf("%llu\n", Perft("3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1", 1));
	//printf("%llu\n", Perft("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5));
	//printf("%llu\n", Perft("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4)); // TODO: fails at depth 5
	//printf("%llu\n", Perft("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6));
	//printf("%llu\n", Perft("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 7));
	//printf("%llu\n", Perft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5));
	printf("%llu\n", Perft(g_StartingFEN, 5));

	printf("%s\n", BestMove(board, 6).toString().c_str());

	scanf("%i");
}