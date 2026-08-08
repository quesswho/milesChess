#pragma once

#include <string>
#include "Search.h"

// A fixed workload for comparing one build of the engine against another.
//
// It runs in one of two modes, and the difference between them is the whole
// point.
//
// TIME is the default and the one that answers "is this better". Every position
// gets the same slice of clock, and the score is how far the search got:
// summed depth first, nodes second. It is the bench under the constraint the
// engine actually plays under, so a change that makes evaluation twice as slow
// to prune slightly better shows up here as lost depth, which is what it is. It
// is also slightly noisy, because a busy machine really does search less.
//
// DEPTH searches every position to the same depth instead, and its node count
// is exactly reproducible: identical numbers mean the search walked an
// identical tree. That makes it useless for judging strength - a slower engine
// scores the same - and ideal for the other question, "did this refactor change
// search behaviour at all, or only its speed".
//
// Neither one settles whether a change is worth keeping. Only a game match at a
// real time control does that; see scripts/sprt.sh. The bench is the cheap
// signal you run on every change, not the verdict.
//
// Tablebases stay off in both modes, so the numbers mean the same thing on a
// machine without `tb/`.
//
// Treat the position list as frozen. Editing it makes new numbers incomparable
// with every number recorded before the edit, which is the one thing the bench
// exists to prevent. If it has to change, bump kBenchVersion so old and new
// numbers cannot be mistaken for each other.
inline constexpr int kBenchVersion = 1;

// 28 positions * 400 ms is about eleven seconds, short enough to run on every
// change. The default depth is what the engine reaches in roughly that slice.
inline constexpr int kBenchTimeMs = 400;
inline constexpr int kBenchDepth = 7;

enum BenchMode { BENCH_TIME, BENCH_DEPTH };

// clang-format off
inline const char* const kBenchPositions[] = {
    // Openings and early middlegames.
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "rn1qkb1r/pp2pppp/5n2/3p1b2/3P4/2N1P3/PP3PPP/R1BQKBNR w KQkq - 0 1",
    "rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq - 0 1",
    "r1bqk2r/ppp2ppp/2n5/4P3/2Bp2n1/5N1P/PP1N1PP1/R2Q1RK1 b kq - 1 10",

    // Middlegames, quiet and sharp, both sides to move.
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "r1bqrnk1/pp2bp1p/2p2np1/3p2B1/3P4/2NBPN2/PPQ2PPP/1R3RK1 w - - 1 12",
    "2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b - - 0 1",
    "r2qkbr1/7p/2p2p1p/3pp3/p1b1P3/P1N1QN2/1PP2PPP/2KR2R1 w q - 0 17",
    "1k2r1r1/1p3pp1/1qpb1n1p/p2p1QP1/5P2/4PB2/PPP3KP/R1B4R b - - 0 1",
    "r4rk1/1pp2ppp/p7/3Q4/4p2P/2PnP2N/PP1Pq3/RK3nR1 w - - 1 23",
    "1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - - 0 1",
    "2krR3/1p3ppr/6b1/nP3p2/5BP1/2P5/1KP5/5BR1 w - - 5 25",
    "2r2k2/pp3p1R/5p2/3Pp3/3qP1Q1/7P/P5PK/2r5 w - - 0 1",
    "5rk1/2P2qp1/7p/5p2/3B4/7P/r4PP1/2QR2K1 b - - 0 39",
    "r2k4/1bnp1Bpp/8/pp2B3/1r6/6K1/PP4PP/3RR3 b - - 3 23",

    // Endgames, where the search runs deep and the eval gradients matter.
    "3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "3r4/8/1p1r2k1/1R2KRP1/p7/8/8/8 b - - 0 1",
    "6k1/5ppp/8/8/8/8/5PPP/6K1 w - - 0 1",
    "8/1p3pp1/p6p/8/8/P6P/1P3PP1/4K1k1 w - - 0 1",
    "8/2k5/8/8/8/8/5PPP/6K1 w - - 0 1",
    "8/8/4k3/8/8/4K3/8/3Q4 w - - 0 1",
    "8/5k2/8/8/8/8/3R4/4K3 w - - 0 1",
    "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1",
    "7k/8/8/8/8/5p2/6P1/7K w - - 0 1",
};
// clang-format on

inline constexpr int kBenchCount = sizeof(kBenchPositions) / sizeof(kBenchPositions[0]);

// Searches every bench position and returns the node total. The last line is
// machine readable and is what the comparison scripts parse.
inline uint64 RunBench(BenchMode mode = BENCH_TIME, int limit = 0, uint64 hashMB = DEFAULT_HASH_MB) {
    if (limit <= 0) limit = (mode == BENCH_TIME) ? kBenchTimeMs : kBenchDepth;

    Search search(hashMB);
    Timer timer;
    uint64 nodes = 0;
    int64 depthSum = 0;

    timer.Start();
    for (int i = 0; i < kBenchCount; i++) {
        search.ClearTables(); // Every position starts cold, or the order would matter

        SearchLimits limits;
        limits.useTablebase = false;
        limits.silent = true;
        if (mode == BENCH_TIME) {
            limits.maxTimeMs = limit;
        } else {
            limits.maxDepth = limit;
        }

        search.LoadPosition(kBenchPositions[i]);
        SearchResult result = search.Go(limits);
        nodes += result.nodes;
        depthSum += result.depth;

        sync_printf("%2i/%2i  depth %2i  score %6" PRId64 "  nodes %10" PRIu64 "  best %s\n", i + 1, kBenchCount,
                    result.depth, result.score, result.nodes,
                    result.best != Move() ? MoveToString(result.best).c_str() : "0000");
    }
    float elapsed = timer.End();
    uint64 nps = (uint64)(nodes / std::max(elapsed, 0.001f));

    sync_printf("\nbench version : %i\n", kBenchVersion);
    sync_printf("positions     : %i\n", kBenchCount);
    sync_printf("mode          : %s\n", mode == BENCH_TIME ? "time" : "depth");
    sync_printf("limit         : %i %s\n", limit, mode == BENCH_TIME ? "ms per position" : "plies");
    sync_printf("hash          : %" PRIu64 " MB\n", hashMB);
    sync_printf("time          : %.0f ms\n", elapsed * 1000.0f);

    // In depth mode the node count is exact and reproducible, so also emit it
    // in the "<nodes> nodes <nps> nps" form that engine tooling expects.
    if (mode == BENCH_DEPTH) sync_printf("%" PRIu64 " nodes %" PRIu64 " nps\n", nodes, nps);

    sync_printf("bench: version=%i mode=%s limit=%i depth=%" PRId64 " nodes=%" PRIu64 " nps=%" PRIu64 "\n",
                kBenchVersion, mode == BENCH_TIME ? "time" : "depth", limit, depthSum, nodes, nps);
    return nodes;
}

// Parses the `bench` argument list shared by the UCI command and the command
// line: "bench", "bench <ms>", "bench time <ms>", "bench depth <plies>".
inline void ParseBenchArgs(const std::string& first, const std::string& second, BenchMode& mode, int& limit) {
    mode = BENCH_TIME;
    limit = 0;
    if (first == "depth" || first == "time") {
        mode = (first == "depth") ? BENCH_DEPTH : BENCH_TIME;
        limit = std::atoi(second.c_str());
    } else if (!first.empty()) {
        limit = std::atoi(first.c_str()); // Bare number means milliseconds
    }
}
