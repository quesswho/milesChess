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

static uint64 Perft(std::string str, int i) {
	Board board(str);
	return Perft_r(board, 0, i);
}
