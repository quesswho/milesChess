#pragma once

#include "Types.h"

struct PerftCase {
    const char* name;
    const char* fen;
    int depth;
    uint64 nodes;
};

inline const char* const kStartPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
inline const char* const kKiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
inline const char* const kEndgame  = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
inline const char* const kPromo    = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
inline const char* const kMidgame  = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
inline const char* const kComplex  = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

inline const PerftCase kPerftFast[] = {
    { "startpos", kStartPos, 1,         20 },
    { "startpos", kStartPos, 2,        400 },
    { "startpos", kStartPos, 3,       8902 },
    { "startpos", kStartPos, 4,     197281 },
    { "startpos", kStartPos, 5,    4865609 },

    { "kiwipete", kKiwipete, 1,         48 },
    { "kiwipete", kKiwipete, 2,       2039 },
    { "kiwipete", kKiwipete, 3,      97862 },
    { "kiwipete", kKiwipete, 4,    4085603 },

    { "endgame",  kEndgame,  1,         14 },
    { "endgame",  kEndgame,  2,        191 },
    { "endgame",  kEndgame,  3,       2812 },
    { "endgame",  kEndgame,  4,      43238 },
    { "endgame",  kEndgame,  5,     674624 },

    { "promo",    kPromo,    1,          6 },
    { "promo",    kPromo,    2,        264 },
    { "promo",    kPromo,    3,       9467 },
    { "promo",    kPromo,    4,     422333 },

    { "midgame",  kMidgame,  1,         44 },
    { "midgame",  kMidgame,  2,       1486 },
    { "midgame",  kMidgame,  3,      62379 },
    { "midgame",  kMidgame,  4,    2103487 },

    { "complex",  kComplex,  1,         46 },
    { "complex",  kComplex,  2,       2079 },
    { "complex",  kComplex,  3,      89890 },
    { "complex",  kComplex,  4,    3894594 },
};

inline const PerftCase kPerftDeep[] = {
    { "startpos", kStartPos, 6,  119060324 },
    { "kiwipete", kKiwipete, 5,  193690690 },
    { "endgame",  kEndgame,  6,   11030083 },
    { "promo",    kPromo,    5,   15833292 },
    { "midgame",  kMidgame,  5,   89941194 },
    { "complex",  kComplex,  5,  164075551 },
};
