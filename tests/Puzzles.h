#pragma once

#include "Types.h"

// Small chess puzzles with a known answer. They are regression tests, not a
// strength benchmark: every case here is solved comfortably by the current
// search, so a failure means the engine started playing a position it used to
// understand differently. Keep the depths as low as the puzzle allows, the
// whole file is meant to run in a second.
//
// Moves are UCI strings. `best` and `avoid` are space separated lists; an
// empty list means "no expectation".
struct PuzzleCase {
    const char* name;
    const char* fen;
    int depth;
    const char* best = "";  // search must return one of these moves
    const char* avoid = ""; // search must not return any of these moves
    int mate = 0;           // 1: side to move mates, -1: side to move gets mated, 0: no mate expected
    int minScore = -32767;  // score bounds in centipawns, only checked when mate == 0
    int maxScore = 32767;
};

// Positions where the engine must find a forced mate. Verified move by move
// against every defence, so the mate has to be real and inside `moves`.
struct MateCase {
    const char* name;
    const char* fen;
    int depth;
    int moves; // mate in this many moves for the side to move
};

// clang-format off

// -- mates in one --------------------------------------------------------
// Rook to the back rank, the king is boxed in by its own pawns.
inline const char* const kMateBackRank    = "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1";
// Scholar's mate, the queen is defended by the bishop on c4.
inline const char* const kMateScholar     = "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR w KQkq - 4 4";
// Ladder mate, the rook on a7 takes the seventh rank away from the king.
inline const char* const kMateLadder      = "7k/R7/8/8/8/8/8/1R5K w - - 0 1";
// Same back rank motif with black to move, so a colour flipped bug shows up.
inline const char* const kMateBlackRank   = "r5k1/5ppp/8/8/8/8/5PPP/6K1 b - - 0 1";

// Back rank: queen or rook takes on e8, the other one covers it. Mate at once.
inline const char* const kMateDeflect     = "4r1k1/5ppp/2Q5/8/8/8/8/4R1K1 w - - 0 1";
// The same mate with a loose black queen on a5 as bait: mate beats material.
inline const char* const kMateOverMat     = "4r1k1/5ppp/2Q5/q7/8/8/8/4R1K1 w - - 0 1";

// -- mates in two --------------------------------------------------------
// Queen sacrifice on g8, the rook is deflected and the knight smothers.
inline const char* const kMateSmother2    = "6rk/6pp/7N/8/8/1Q6/8/6K1 w - - 0 1";
// No check works, only a quiet move does: Kg6 (Kg8, Qa8#) or Qg1 (Kh7, Qg7#).
inline const char* const kMateQuiet2      = "7k/8/5K2/8/8/8/8/Q7 w - - 0 1";

// -- mate in three -------------------------------------------------------
// Philidor's legacy. Only Nh6 mates: it is double check, so the rook cannot
// block on f7 the way it can after any other knight move. The search needs
// depth 12 for this one, at depth 10 it still prefers Ng5.
inline const char* const kMateSmother3    = "5rk1/5Npp/8/8/8/1Q6/8/6K1 w - - 0 1";

// -- getting mated -------------------------------------------------------
// Black has exactly one legal move and is mated next move anyway.
inline const char* const kMatedOnlyMove   = "7k/8/6KQ/8/8/8/8/8 b - - 0 1";

// -- tactics -------------------------------------------------------------
// Ng7+ forks the king and the queen.
inline const char* const kTacticFork      = "4k3/8/4N3/7q/8/8/8/4K3 w - - 0 1";
// The rook on h5 hangs, take it.
inline const char* const kTacticHangRook  = "4k3/8/8/7r/8/8/8/4K2R w K - 0 1";
// d5 is defended twice, Bxd5 drops the bishop for a pawn.
inline const char* const kTacticPoison    = "4k3/8/2p1p3/3p4/8/1B6/8/4K3 w - - 0 1";
// e8=N+ forks king and queen, e8=Q is not even check.
inline const char* const kTacticUnderPromo = "8/4P1q1/3k4/8/8/8/8/7K w - - 0 1";
// Winning by a queen, but Qb6 is stalemate.
inline const char* const kTacticStalemate = "k7/8/8/8/8/8/5Q2/6K1 w - - 0 1";
// Nothing to do but promote, and a queen is the promotion to pick.
inline const char* const kTacticPromote   = "8/6P1/8/8/8/8/8/K6k w - - 0 1";
// Black just played b7-b5; only cxb6 e.p. keeps the pawn race won.
inline const char* const kTacticEnPassant = "8/8/8/1pP5/8/8/8/4K2k w - b6 0 2";

// -- drawn ---------------------------------------------------------------
inline const char* const kDrawBareKings   = "8/8/4k3/8/8/4K3/8/8 w - - 0 1";

// -- terminal ------------------------------------------------------------
// Black to move is stalemated: no move, score 0.
inline const char* const kTerminalStalemate = "k7/P7/K7/8/8/8/8/8 b - - 0 1";
// Black to move is checkmated: no move, mated score.
inline const char* const kTerminalMate      = "7k/6RR/8/8/8/8/8/K7 b - - 0 1";

inline const PuzzleCase kPuzzles[] = {
    { .name = "mate-back-rank",  .fen = kMateBackRank,       .depth =  4, .best = "a1a8",           .mate =  1 },
    { .name = "mate-scholar",    .fen = kMateScholar,        .depth =  4, .best = "f3f7",           .mate =  1 },
    { .name = "mate-ladder",     .fen = kMateLadder,         .depth =  4, .best = "b1b8",           .mate =  1 },
    { .name = "mate-black-rank", .fen = kMateBlackRank,      .depth =  4, .best = "a8a1",           .mate =  1 },
    { .name = "mate-deflect",    .fen = kMateDeflect,        .depth =  4, .best = "c6e8 e1e8",      .mate =  1 },
    { .name = "mate-over-mat",   .fen = kMateOverMat,        .depth =  4, .best = "c6e8 e1e8",      .mate =  1 },
    { .name = "mate-smother-2",  .fen = kMateSmother2,       .depth =  6, .best = "b3g8",           .mate =  1 },
    { .name = "mate-quiet-2",    .fen = kMateQuiet2,         .depth =  6, .best = "f6g6 a1g1",      .mate =  1 },
    { .name = "mate-smother-3",  .fen = kMateSmother3,       .depth = 12, .best = "f7h6",           .mate =  1 },
    { .name = "mated-only-move", .fen = kMatedOnlyMove,      .depth =  4, .best = "h8g8",           .mate = -1 },

    // Both forks win the queen but land in KNvK, which is a dead draw.
    { .name = "fork-queen",      .fen = kTacticFork,       .depth =  6, .best = "e6g7",  .minScore = -50, .maxScore = 50 },
    { .name = "hanging-rook",    .fen = kTacticHangRook,   .depth =  6, .best = "h1h5",  .minScore =  300 },
    { .name = "poisoned-pawn",   .fen = kTacticPoison,     .depth =  6, .avoid = "b3d5", .minScore = -200, .maxScore = 200 },
    { .name = "underpromotion",  .fen = kTacticUnderPromo, .depth =  8, .best = "e7e8n", .minScore = -50, .maxScore = 50 },
    { .name = "avoid-stalemate", .fen = kTacticStalemate,  .depth =  6, .avoid = "f2b6", .minScore =  500 },
    { .name = "promotion",       .fen = kTacticPromote,    .depth =  8, .best = "g7g8q", .minScore =  500 },
    { .name = "en-passant",      .fen = kTacticEnPassant,  .depth = 10, .best = "c5b6",  .minScore =  500 },

    { .name = "draw-bare-kings", .fen = kDrawBareKings,    .depth =  6, .minScore = -50, .maxScore =  50 },
};

inline const MateCase kMates[] = {
    { "mate-back-rank",  kMateBackRank,  4, 1 },
    { "mate-scholar",    kMateScholar,   4, 1 },
    { "mate-ladder",     kMateLadder,    4, 1 },
    { "mate-black-rank", kMateBlackRank, 4, 1 },
    { "mate-deflect",    kMateDeflect,   4, 1 },
    { "mate-over-mat",   kMateOverMat,   4, 1 },
    { "mate-smother-2",  kMateSmother2,  6, 2 },
    { "mate-quiet-2",    kMateQuiet2,    6, 2 },
    { "mate-smother-3",  kMateSmother3, 12, 3 },
};
// clang-format on
