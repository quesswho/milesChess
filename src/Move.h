#pragma once

#include "board.h"

static uint64 Perft_r(Board board, int depth, int maxdepth) {
	if (depth == maxdepth) return 1;
	const std::vector<Move> moves = board.GenerateMoves();

	uint64 result = 0;
	for (const Move& move : moves) {
		result += Perft_r(board.MovePiece(move), depth + 1, maxdepth);
	}
	return result;
}

static uint64 Perft(int i) {
	Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	return Perft_r(board, 0, i);
}
