#pragma once

#include "board.h"

static int64 Evaluate(const Board& board) {

	int64 centipawns = 0;

	uint64 wp = board.m_WhitePawn, wkn = board.m_WhiteKnight, wb = board.m_WhiteBishop, wr = board.m_WhiteRook, wq = board.m_WhiteQueen, wk = board.m_WhiteKing,
		bp = board.m_BlackPawn, bkn = board.m_BlackKnight, bb = board.m_BlackBishop, br = board.m_BlackRook, bq = board.m_BlackQueen, bk = board.m_BlackKing;

	const uint64 pawnVal = 100, knightVal = 350, bishopVal = 350, rookVal = 525, queenVal = 1000, kingVal = 10000;

	// Pawns
	while (wp > 0) {
		int pos = 63 - PopPos(wp);
		centipawns += pawnVal + Lookup::pawn_table[pos];
	}

	while (bp > 0) {
		int pos = PopPos(bp);
		centipawns -= pawnVal + Lookup::pawn_table[pos];
	}

	// Knights
	while (wkn > 0) {
		int pos = 63 - PopPos(wkn);
		centipawns += knightVal + Lookup::knight_table[pos];
	}

	while (bkn > 0) {
		int pos = PopPos(bkn);
		centipawns -= knightVal + Lookup::knight_table[pos];
	}

	// Bishops
	while (wb > 0) {
		int pos = 63 - PopPos(wb);
		centipawns += bishopVal + Lookup::bishop_table[pos];
	}

	while (bb > 0) {
		int pos = PopPos(bb);
		centipawns -= bishopVal + Lookup::bishop_table[pos];
	}

	// Rooks
	while (wr > 0) {
		int pos = 63 - PopPos(wr);
		centipawns += rookVal + Lookup::rook_table[pos];
	}

	while (br > 0) {
		int pos = PopPos(br);
		centipawns -= rookVal + Lookup::rook_table[pos];
	}

	while (wq > 0) {
		int pos = 63 - PopPos(wq);
		centipawns += queenVal + Lookup::queen_table[pos];
	}

	while (bq > 0) {
		int pos = PopPos(bq);
		centipawns -= queenVal + Lookup::queen_table[pos];;
	}

	while (wk > 0) {
		int pos = 63 - PopPos(wk);
		centipawns += kingVal + Lookup::king_table[pos];
	}

	while (bk > 0) {
		int pos = PopPos(bk);
		centipawns -= kingVal + Lookup::king_table[pos];;
	}

	return centipawns;
}

static uint64 Perft_r(const Board& board, int depth, int maxdepth) {
	const std::vector<Move> moves = board.GenerateMoves();
	if (depth == 1) return moves.size();

	int64 result = 0;
	for (const Move& move : moves) {
		int64 count = Perft_r(board.MovePiece(move), depth - 1, maxdepth);
		result += count;
		if (depth == maxdepth) {
			printf("%s: %llu: %s\n", move.toString().c_str(), count, BoardtoFen(board.MovePiece(move)).c_str());
		}
	}
	return result;
}

static uint64 Perft(std::string str, int depth) {
	Timer time;
	Board board(str);
	time.Start();
	uint64 result = Perft_r(board, depth, depth) / 1000000.0f;
	float t = time.End();
	printf("%.3fMNodes/s\n", result / t);
	return result;
}

static int64 AlphaBeta_r(Board board, int64 alpha, int64 beta, int depth) {
	int64 sgn = board.m_BoardInfo.m_WhiteMove ? 1 : -1;
	if (depth == 0) return sgn * Evaluate(board);
	const std::vector<Move> moves = board.GenerateMoves();
	if (moves.size() == 0) {
		return -sgn * (10000+depth); // Mate in 0 is 10000 centipawns worth
	}
	int64 bestScore = -110000;
	for (const Move& move : moves) {
		int64 score = -AlphaBeta_r(board.MovePiece(move), -beta, -alpha, depth - 1);
		if (score > bestScore) {
			bestScore = score;
			if (score > alpha) {
				alpha = score;
			}
		}
		if (score >= beta) { // Exit out early
			return bestScore;
		}
	}
	return bestScore;
}

static Move BestMove(Board board, int depth) {
	const std::vector<Move> moves = board.GenerateMoves();
	if (moves.size() == 0) return Move(0, 0, MoveType::NONE);
	Move best = moves[0];
	int64 bestscore = -110000;
	for (const Move& move : moves) {
		int64 score = -AlphaBeta_r(board.MovePiece(move), -110000, 110000, depth - 1);
		if (score > bestscore) {
			bestscore = score;
			best = move;
		}
	}
	return best;
}
