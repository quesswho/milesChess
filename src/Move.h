#pragma once

#include "board.h"

static uint64 Perft_r(Board board, int depth, int maxdepth) {
	const std::vector<Move> moves = board.GenerateMoves();
	if (depth == maxdepth-1) return moves.size();

	uint64 result = 0;
	for (const Move& move : moves) {
		uint64 count = Perft_r(board.MovePiece(move), depth + 1, maxdepth);
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
