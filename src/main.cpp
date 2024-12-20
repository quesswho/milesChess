#include<stdio.h>
#include "UCI.h"


const char* g_StartingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";



int main() {
	TableBase::Init("tb/");
	// First game 2024-08-17 (Lost against me)
	// 1. h3 d5 2. g3 e5 3. e3 e4 4. c3 Nf6 5. f3 Nc6 6. d3 Bd6 7. f4 O - O 8. b3 a5 9. a3 Ne7 10. h4 Ng4 11. d4(11. dxe4 dxe4 12. c4 Bc5 13. Qe2 h5 14. Nh3 Nf5 15. Qg2 Re8 16. b4 axb4 17. axb4 Rxa1 18. Qb2 Rxb1 19. Qxb1 Bxe3 20. Bxe3 Ngxe3 21. Rg1 Nxf1 22. Rxf1 Nxg3 23. Rg1 Bxh3 24. Qc2 Qxh4 25. f5 Bg4 26. c5 Ne2 + 27. Rg3 Qxg3 + 28. Kf1 Qg1#

	// Second game with piece tables (Draw by repetition)
	// 1. Nc3 e5 2. e3 d5 3. Qh5 Nc6 4. Bb5 d4 5. Qxe5+ Be6 6. exd4 Bd6 (6... Nf6 7. Nf3 Bd6 8. Qe2 O-O 9. Bxc6 bxc6 10. h3 Re8 11. Kf1 c5 12. dxc5 Bxc5 13. Qb5 Qd6 14. a3 Bf5 15. d3 Bd4 16. Qxf5 Bxc3 17. bxc3 Qe7 18. Bg5 Qe2+ 19. Kg1 Qxc2 20. Bxf6 gxf6 21. Qxf6 Re2 22. Ne5 Rf8 23. Qg5+ Kh8 24. Qf6+ Kg8 25. Qg5+ Kh8)
	
	// Third game against stockfish 16
	// 1. Nc3 d5 2. e3 e6 3. Bb5+ c6 4. Bf1 e5 5. e4 d4 6. Nb1 Nf6 7. Bd3 Nbd7 8. Nf3 Bd6 9. O-O Nc5 10. h3 Nxd3 11. cxd3 g5 12. Nxg5 Rg8 13. f4 exf4 14. e5 Rxg5 15. exd6 Bxh3 16. Qe1+ Kd7 17. Qh4 Rxg2+ 18. Kh1 Ng4 19. Qxh3 Rh2+ 20. Qxh2 Nxh2 21. Kxh2 Qh4+ 22. Kg2 Qg3+ 23. Kh1 Rg8 24. b3 Qg2#

	// Game against stockfish 16 with iterative deepening with 10s
	// 1. Nf3 d5 2. e3 Nf6 3. Nc3 e6 4. Bb5+ c6 5. Be2 Bd6 6. O-O e5 7. d4 e4 8. Ne5 Nbd7 9. Nxd7 Bxd7 10. f3 Qe7 11. fxe4 dxe4 12. Qe1 h5 13. Qh4 Rh6 14. Bc4 g5 15. Qxg5 Bxh2+ 16. Kh1 Rg6 17. Qa5 Nd5 18. Nxd5 Qh4 19. Rxf7 Bf4+ 20. Kg1 Qh2+ 21. Kf1 Qxg2+ 22. Ke1 Bg3+ 23. Kd1 Bg4+ 24. Be2 Qxe2#

	// Game against me & erik, we won
	// 1. e4 e5 2. d3 Nf6 3. Nc3 Bb4 4. Ne2 d5 5. a3 Bxc3+ 6. Nxc3 dxe4 7. dxe4 Qxd1+ 8. Kxd1 O-O 9. h3 Rd8+ 10. Bd3 b6 11. g3 Ba6 12. f4 Bxd3 13. cxd3 Rxd3+ 14. Ke2 Rxg3 15. fxe5 Nfd7 16. Nd5 Na6 17. b4 c6 18. Ne7+ Kf8 19. Nxc6 Nc7 20. h4 Nb5 21. a4 Nc3+ 22. Ke1 Nxe4 23. Rf1 Rc8 24. e6 Ndf6 25. e7+ Ke8 26. b5 Rc3 27. Ba3 Rc7 28. Rd1 Re3#

	// This should yield a draw
	// position startpos moves e2e4 d7d5 b1c3 d5d4 c3d5 e7e6 d5f4 g7g5 f4d3 b8c6 g1f3 g8f6 f3g5 h7h6 e4e5 h6g5 e5f6 d8f6 h2h3 e6e5 g2g4 e5e4 d1e2 f6e6 f1g2 e4d3 e2e6 c8e6 c2d3 e8d7 a2a4 f7f5 h1g1 f5f4 a4a5 f8c5 e1f1 b7b5 b2b4 c5e7 c1a3 a8g8 g2c6 d7c6 f1g2 a7a6 f2f3 e7d6 g1e1 e6d5 e1h1 h8h4 h1f1 g8e8 a1e1 e8e1 f1e1 c6d7 e1h1 c7c6 h1e1 d6e7 e1h1 e7d6

	if (false) {
		UCI uci;
		uci.Start();
	} else {
		char str[1000];
		Search* search = new Search();
		//search.LoadPosition(g_StartingFEN);
		//search->LoadPosition("r1bqk2r/ppp2ppp/4p3/3p1n2/3PnB2/4QN2/PPP1PPPP/1K1R1B1R w Kkq - 0 1");
		//search->LoadPosition("1k2r1r1/1p3pp1/1qpb1n1p/p2p1QP1/5P2/4PB2/PPP3KP/R1B4R b - - 0 1"); // Stockfish evaluates this as -4, while milesbot rates it as 0
		//search->LoadPosition("1k2r2r/1pb5/1qp5/p2p4/8/3Q4/PPP1p1RP/R1B4K w - - 0 11");
		//search->LoadPosition("rnnk1b1r/p3pppB/2b5/6N1/8/2N5/1P3PPP/R1B2RK1 b - - 1 15");
		//printf("%llu\n", search.Perft(6));
		//search->LoadPosition("3r4/8/1p1r2k1/1R2KRP1/p7/8/8/8 b - - 0 1"); // TODO: Wrong move here
		//search->LoadPosition("2krR3/1p3ppr/6b1/nP3p2/5BP1/2P5/1KP5/5BR1 w - - 5 25");
		//search->LoadPosition("2r2k2/pp3p1R/5p2/3Pp3/3qP1Q1/7P/P5PK/2r5 w - - 0 1");
		//search->LoadPosition("7k/8/8/8/8/5p2/6P1/7K w - - 0 1");
		//search->LoadPosition("5rk1/2P2qp1/7p/5p2/3B4/7P/r4PP1/2QR2K1 b - - 0 39"); // Supposed to do the move a2a8
		//search->LoadPosition("r2k4/1bnp1Bpp/8/pp2B3/1r6/6K1/PP4PP/3RR3 b - - 3 23"); // Correct move: b4e4
		search->LoadPosition("r2qkbr1/7p/2p2p1p/3pp3/p1b1P3/P1N1QN2/1PP2PPP/2KR2R1 w q - 0 17"); // Should do e4e5
		//search->LoadPosition("r2qkbr1/7p/2p2p1p/4p3/p1bpP3/P1N1Q3/1PPN1PPP/2KR2R1 w q - 0 18"); // Best Move: d2c4, Likely faulty due to eval function
		//search->LoadPosition("r4rk1/1pp2ppp/p7/3Q4/4p2P/2PnP2N/PP1Pq3/RK3nR1 w - - 1 23"); // Draw trap
		Timer time;
		time.Start();
		search->m_MaxTime = 100000;
		search->UCIMove_async(); // 3580-3770
		//search->Perft(6); // 102-105.5
						  // 102.3-105.3
		printf("%.3fs\n", time.End());
		
		//while (true) {
			
			/*Move move = search.BestMove(2);
			if (move.m_Type == MoveType::NONE) {
				printf("You won!\n");
				scanf("%s");
				break;
			}

			search.MoveRootPiece(move);
			printf("%s, %s\n", move.toString().c_str(), search.GetFen().c_str());
			scanf("%s", &str);
			if (str[0] == 'q') break;

			move = search.GetMove(str);
			search.MoveRootPiece(move);
		}*/
		scanf("%s", &str);
		delete search;
	}
}