#pragma once

#include "Position.h"

enum class PerftGen { Staged, Bulk };

template<PerftGen G>
uint64 Perft(Position& pos, int depth);

uint64 PerftDivide(Position& pos, int depth, bool print);
