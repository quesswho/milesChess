#pragma once

#include "board.h"

static int64 Evaluate(const Board& board) {

	int64 centipawns = 0;

	uint64 wp = board.m_WhitePawn, wkn = board.m_WhiteKnight, wb = board.m_WhiteBishop, wr = board.m_WhiteRook, wq = board.m_WhiteQueen, wk = board.m_WhiteKing,
		bp = board.m_BlackPawn, bkn = board.m_BlackKnight, bb = board.m_BlackBishop, br = board.m_BlackRook, bq = board.m_BlackQueen, bk = board.m_BlackKing;

	const uint64 pawnVal = 100, knightVal = 350, bishopVal = 350, rookVal = 525, queenVal = 1000;

	// Pawns
	while (wp > 0) {
		int pos = PopPos(wp); // Create value map
		centipawns += pawnVal;
	}

	while (bp > 0) {
		int pos = PopPos(bp); // Create value map
		centipawns -= pawnVal;
	}

	// Knights
	while (wkn > 0) {
		int pos = PopPos(wkn); // Create value map
		centipawns += knightVal;
	}

	while (bkn > 0) {
		int pos = PopPos(bkn); // Create value map
		centipawns -= knightVal;
	}

	// Bishops
	while (wb > 0) {
		int pos = PopPos(wb); // Create value map
		centipawns += bishopVal;
	}

	while (bb > 0) {
		int pos = PopPos(bb); // Create value map
		centipawns -= bishopVal;
	}

	// Rooks
	while (wr > 0) {
		int pos = PopPos(wr); // Create value map
		centipawns += rookVal;
	}

	while (br > 0) {
		int pos = PopPos(br); // Create value map
		centipawns -= rookVal;
	}

	while (wq > 0) {
		int pos = PopPos(wq); // Create value map
		centipawns += queenVal;
	}

	while (bq > 0) {
		int pos = PopPos(bq); // Create value map
		centipawns -= queenVal;
	}

	return centipawns;
}

static uint64 Perft_r(Board board, int depth, int maxdepth) {
	const std::vector<Move> moves = board.GenerateMoves();
	if (depth == maxdepth-1) return moves.size();

	int64 result = 0;
	for (const Move& move : moves) {
		int64 count = Perft_r(board.MovePiece(move), depth + 1, maxdepth);
		result += count;
		if (depth == 0) {
			printf("%s: %llu: %s\n", move.toString().c_str(), count, BoardtoFen(board.MovePiece(move)).c_str());
		}
	}
	return result;
}

static uint64 Perft(std::string str, int i) {
	Board board(str);
	return Perft_r(board, 0, i);
}

static int64 BestMove_r(Board board, int depth) {
	int64 sgn = board.m_BoardInfo.m_WhiteMove ? 1 : -1;
	if (depth == 0) return sgn * Evaluate(board);
	const std::vector<Move> moves = board.GenerateMoves();
	if (moves.size() == 0) {
		return -sgn * 10000; // Mate is 10000 centipawns worth
	}
	int64 bestscore = 0;
	for (const Move& move : moves) {
		int64 score = -BestMove_r(board.MovePiece(move), depth - 1);
		if (score > bestscore) {
			bestscore = score;
		}
	}
	return bestscore;
}

static Move BestMove(Board board, int depth) {
	const std::vector<Move> moves = board.GenerateMoves();
	if (moves.size() == 0) return Move(0, 0, MoveType::NONE);
	Move best = moves[0];
	int64 bestscore = -10000;
	for (const Move& move : moves) {
		int64 score = -BestMove_r(board.MovePiece(move), depth - 1);
		if (score > bestscore) {
			bestscore = score;
			best = move;
		}
	}
	return best;
}
