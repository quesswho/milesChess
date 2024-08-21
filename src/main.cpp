#include<stdio.h>
#include <memory>
#include "UCI.h"

#include "Search.h"


const char* g_StartingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";



int main() {

	// First game 2024-08-17 (Lost against me)
	// 1. h3 d5 2. g3 e5 3. e3 e4 4. c3 Nf6 5. f3 Nc6 6. d3 Bd6 7. f4 O - O 8. b3 a5 9. a3 Ne7 10. h4 Ng4 11. d4(11. dxe4 dxe4 12. c4 Bc5 13. Qe2 h5 14. Nh3 Nf5 15. Qg2 Re8 16. b4 axb4 17. axb4 Rxa1 18. Qb2 Rxb1 19. Qxb1 Bxe3 20. Bxe3 Ngxe3 21. Rg1 Nxf1 22. Rxf1 Nxg3 23. Rg1 Bxh3 24. Qc2 Qxh4 25. f5 Bg4 26. c5 Ne2 + 27. Rg3 Qxg3 + 28. Kf1 Qg1#)

	// Second game with piece tables (Draw by repetition)
	// 1. Nc3 e5 2. e3 d5 3. Qh5 Nc6 4. Bb5 d4 5. Qxe5+ Be6 6. exd4 Bd6 (6... Nf6 7. Nf3 Bd6 8. Qe2 O-O 9. Bxc6 bxc6 10. h3 Re8 11. Kf1 c5 12. dxc5 Bxc5 13. Qb5 Qd6 14. a3 Bf5 15. d3 Bd4 16. Qxf5 Bxc3 17. bxc3 Qe7 18. Bg5 Qe2+ 19. Kg1 Qxc2 20. Bxf6 gxf6 21. Qxf6 Re2 22. Ne5 Rf8 23. Qg5+ Kh8 24. Qf6+ Kg8 25. Qg5+ Kh8)
	
	// Third game against stockfish 16
	// 1. Nc3 d5 2. e3 e6 3. Bb5+ c6 4. Bf1 e5 5. e4 d4 6. Nb1 Nf6 7. Bd3 Nbd7 8. Nf3 Bd6 9. O-O Nc5 10. h3 Nxd3 11. cxd3 g5 12. Nxg5 Rg8 13. f4 exf4 14. e5 Rxg5 15. exd6 Bxh3 16. Qe1+ Kd7 17. Qh4 Rxg2+ 18. Kh1 Ng4 19. Qxh3 Rh2+ 20. Qxh2 Nxh2 21. Kxh2 Qh4+ 22. Kg2 Qg3+ 23. Kh1 Rg8 24. b3 Qg2#

	//UCI uci;
	//uci.Start();

	//printf("%llu\n", Perft("3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1", 1));
	//printf("%llu\n", Perft("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5));
	//printf("%llu\n", Perft("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4)); // TODO: fails at depth 5
	//printf("%llu\n", Perft("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6));
	//printf("%llu\n", Perft("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 7));
	//printf("%llu\n", Perft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5));
	//printf("%llu\n", Perft(g_StartingFEN, 6));

	Lookup::PrintZobristConstants();
	char str[1000];
	fgets(str, 100, stdin);
	/*//std::unique_ptr<Board> board = std::make_unique<Board>(str);
	//std::unique_ptr<Board> board = std::make_unique<Board>(g_StartingFEN);
	std::unique_ptr<Board> board = std::make_unique<Board>("rnbqkbnr/pppppppp/8/8/8/2N5/PPPPPPPP/R1BQKBNR b KQkq - 1 1");
	while (true) {
		Move move = BestMove(*board, 6);
		if (move.m_Type == MoveType::NONE) {
			printf("You won!\n");
			scanf("");
			break;
		}

		board = std::make_unique<Board>(board->MovePiece(move));
		printf("%s, %s\n", move.toString().c_str(), BoardtoFen(*board).c_str());
		scanf("%s", &str);
		if (str[0] == 'q') break;

		move = GetMove(str, *board);
		board = std::make_unique<Board>(board->MovePiece(move));
	}*/

}