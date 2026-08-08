#include "TestUtil.h"

#include "Transposition.h" // Evaluate.h uses PawnTable but does not include it
#include "Evaluate.h"
#include "MoveGen.h"
#include "Search.h"

#include <cctype>
#include <memory>
#include <string>
#include <vector>

// Endgames where one side is down to a bare king. The general evaluation has
// nothing to say about them: no pawns to push, no structure to judge, and the
// mate is far past the horizon, so the score itself has to point the way. These
// tests pin down the two halves of that: the shape of the static score, and the
// search actually walking down it to a mate.
//
// Known gaps, deliberately not asserted here: KNN vs K and same coloured KBB vs
// K are scored as wins even though neither can force mate, and the search does
// not convert KBB vs K within a reasonable number of moves.

// -- position helpers ----------------------------------------------------

// square = rank * 8 + file, file 0 = 'a' and rank 0 = '1'.
static std::string MakeFen(const char (&squares)[64], bool whiteToMove) {
    std::string board;
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            char piece = squares[rank * 8 + file];
            if (!piece) {
                empty++;
                continue;
            }
            if (empty) {
                board += (char)('0' + empty);
                empty = 0;
            }
            board += piece;
        }
        if (empty) board += (char)('0' + empty);
        if (rank) board += '/';
    }
    return board + (whiteToMove ? " w - - 0 1" : " b - - 0 1");
}

static std::string Fen(const std::vector<std::pair<int, char>>& pieces, bool whiteToMove = true) {
    char squares[64] = {};
    for (auto& piece : pieces) squares[piece.first] = piece.second;
    return MakeFen(squares, whiteToMove);
}

static std::string SquareName(int square) {
    return std::string(1, (char)('a' + square % 8)) + (char)('1' + square / 8);
}

static bool Adjacent(int a, int b) {
    return SquareDistance(a, b) <= 1;
}

// clang-format off
enum Square {
    A1 = 0, B1 = 1, C1 = 2, G1 = 6, H1 = 7,
    A2 = 8, C2 = 10, G2 = 14,
    D4 = 27,
    E5 = 36,
    A7 = 48,
    A8 = 56, C8 = 58, H8 = 63,
};
// clang-format on

static std::unique_ptr<PawnTable> g_PawnTable;

// Evaluate() is relative to the side to move; most checks here are easier to
// read from white's side of the board.
static int64 EvalWhite(const std::string& fen) {
    Position position;
    position.SetPosition(fen);
    int64 score = Evaluate(position, g_PawnTable.get());
    return position.m_WhiteMove ? score : -score;
}

static bool BareKing(const std::string& fen, int64& score) {
    Position position;
    position.SetPosition(fen);
    return BareKingScore(position, score);
}

// -- which positions the bare king score claims --------------------------

static void Detection() {
    int64 score = 0;
    // One side has pieces, the other has nothing but its king.
    CHECK(BareKing(Fen({ { A1, 'K' }, { H1, 'Q' }, { E5, 'k' } }), score));
    CHECK(BareKing(Fen({ { A1, 'K' }, { E5, 'k' }, { H8, 'q' } }), score));
    CHECK(BareKing(Fen({ { A1, 'K' }, { H1, 'R' }, { G1, 'R' }, { E5, 'k' } }), score));

    // Both bare: nothing to steer towards, so the normal evaluation handles it.
    CHECK(!BareKing(Fen({ { A1, 'K' }, { E5, 'k' } }), score));
    // Both sides still have material.
    CHECK(!BareKing(Fen({ { A1, 'K' }, { H1, 'Q' }, { E5, 'k' }, { H8, 'n' } }), score));
    // Any pawn at all means the position can still change character.
    CHECK(!BareKing(Fen({ { A1, 'K' }, { H1, 'Q' }, { A2, 'P' }, { E5, 'k' } }), score));
    CHECK(!BareKing(Fen({ { A1, 'K' }, { H1, 'Q' }, { E5, 'k' }, { A7, 'p' } }), score));
}

// -- material that cannot mate is a dead draw ----------------------------

static void InsufficientMaterial() {
    const char* const minors = "NB";
    for (int i = 0; i < 2; i++) {
        char white = minors[i], black = (char)std::tolower(minors[i]);
        for (int stm = 0; stm < 2; stm++) {
            CHECK_EQ(EvalWhite(Fen({ { A1, 'K' }, { C1, white }, { E5, 'k' } }, stm != 0)), 0);
            CHECK_EQ(EvalWhite(Fen({ { A1, 'K' }, { E5, 'k' }, { C8, black } }, stm != 0)), 0);
        }
    }

    // A lone minor is a draw wherever the kings stand.
    for (int weak = 0; weak < 64; weak++) {
        if (weak == A1 || weak == C1 || Adjacent(weak, A1)) continue;
        CHECK_EQ(EvalWhite(Fen({ { A1, 'K' }, { C1, 'B' }, { weak, 'k' } })), 0);
    }

    // The same minor with a pawn is a different game, and must not be zeroed.
    CHECK(EvalWhite(Fen({ { A1, 'K' }, { C1, 'B' }, { A2, 'P' }, { E5, 'k' } })) > 0);
    CHECK(EvalWhite(Fen({ { A1, 'K' }, { E5, 'k' }, { C8, 'b' }, { A7, 'p' } })) < 0);
}

// -- mating material is worth at least the material -----------------------

struct MaterialCase {
    const char* name;
    std::vector<std::pair<int, char>> extra; // white pieces beside the king
    int64 material;
};

static void MatingMaterial() {
    const MaterialCase cases[] = {
        { "KQ", { { H1, 'Q' } }, QUEEN_VALUE },
        { "KR", { { H1, 'R' } }, ROOK_VALUE },
        { "KRR", { { H1, 'R' }, { G1, 'R' } }, 2 * ROOK_VALUE },
        { "KBB", { { C1, 'B' }, { B1, 'B' } }, 2 * BISHOP_VALUE },
        { "KBN", { { C1, 'B' }, { G1, 'N' } }, BISHOP_VALUE + KNIGHT_VALUE },
        { "KQR", { { H1, 'Q' }, { G1, 'R' } }, QUEEN_VALUE + ROOK_VALUE },
    };

    for (const MaterialCase& material : cases) {
        std::vector<std::pair<int, char>> white = material.extra;
        white.push_back({ A1, 'K' });
        white.push_back({ E5, 'k' });

        int64 score = EvalWhite(Fen(white));
        if (score < material.material) {
            printf("  %s scores %" PRId64 ", below its material %" PRId64 "\n", material.name, score,
                   material.material);
        }
        // The drive bonus only ever adds to the winning side's score, and it
        // must stay small enough not to look like an extra piece.
        CHECK(score >= material.material);
        CHECK(score < material.material + KNIGHT_VALUE);

        // Mirroring the position swaps who is winning and nothing else.
        std::vector<std::pair<int, char>> black;
        for (auto& piece : white) {
            char swapped = std::isupper((unsigned char)piece.second) ? (char)std::tolower(piece.second)
                                                                     : (char)std::toupper(piece.second);
            black.push_back({ piece.first ^ 56, swapped });
        }
        CHECK_EQ(EvalWhite(Fen(black, false)), -score);

        // Relative score: the same board seen from the other side of the move.
        Position position;
        position.SetPosition(Fen(white, false));
        CHECK_EQ(Evaluate(position, g_PawnTable.get()), -score);
    }
}

// -- the score has to push the weak king to an edge and a corner ----------

static void DriveToCorner() {
    // White king and queen fixed, the black king walked over the board. The
    // best square for white must be a corner and the worst a central one.
    const int strong = D4, queen = H1;
    int best = -1, worst = -1;
    int64 bestScore = 0, worstScore = 0;
    for (int weak = 0; weak < 64; weak++) {
        if (weak == strong || weak == queen || Adjacent(weak, strong)) continue;
        int64 score = EvalWhite(Fen({ { strong, 'K' }, { queen, 'Q' }, { weak, 'k' } }));
        if (best < 0 || score > bestScore) best = weak, bestScore = score;
        if (worst < 0 || score < worstScore) worst = weak, worstScore = score;
    }
    printf("  best square for white is the black king on %s (%" PRId64 "), worst is %s (%" PRId64 ")\n",
           SquareName(best).c_str(), bestScore, SquareName(worst).c_str(), worstScore);
    CHECK_EQ(CenterDistance(best), 6); // a corner
    // The centre squares themselves are next to the white king and so never
    // reached by the sweep; the ring around them is as central as it gets.
    CHECK(CenterDistance(worst) <= 1);

    // Each step the weak king takes away from the centre is worth something,
    // with the strong king parked out of the way so only that term moves.
    int64 previous = 0;
    for (int file = 4; file <= 7; file++) {
        int weak = 7 * 8 + file; // along the eighth rank, towards h8
        int64 score = EvalWhite(Fen({ { A1, 'K' }, { H1, 'Q' }, { weak, 'k' } }));
        if (file > 4) CHECK(score > previous);
        previous = score;
    }

    // Bringing the strong king closer is worth something too. It walks the
    // long diagonal so every step really does close the distance, and the
    // queen sits out of the way in a corner.
    previous = 0;
    for (int step = 0; step <= 5; step++) {
        int strongKing = step * 8 + step; // a1 walking up to f6, black king on h8
        int64 score = EvalWhite(Fen({ { strongKing, 'K' }, { H1, 'Q' }, { H8, 'k' } }));
        if (step > 0) CHECK(score > previous);
        previous = score;
    }
}

// Bishop and knight can only mate in a corner the bishop covers, so the score
// has to prefer that corner over the other one.
static void BishopKnightCorner() {
    // From d4 both corners are the same distance away, so the only difference
    // between the two positions is the corner the bishop can reach.
    const int strong = D4, knight = G2;
    struct {
        const char* name;
        int bishop; // c1 is dark, b1 is light
        int good, bad;
    } cases[] = {
        { "dark bishop", C1, H8, A8 },
        { "dark bishop", C1, A1, A8 },
        { "light bishop", B1, A8, H8 },
        { "light bishop", B1, H1, H8 },
    };

    for (auto& c : cases) {
        int64 good = EvalWhite(Fen({ { strong, 'K' }, { c.bishop, 'B' }, { knight, 'N' }, { c.good, 'k' } }));
        int64 bad = EvalWhite(Fen({ { strong, 'K' }, { c.bishop, 'B' }, { knight, 'N' }, { c.bad, 'k' } }));
        if (good <= bad) {
            printf("  %s: %s scores %" PRId64 ", not better than %s at %" PRId64 "\n", c.name,
                   SquareName(c.good).c_str(), good, SquareName(c.bad).c_str(), bad);
        }
        CHECK(good > bad);
    }

    // The wrong corner must not look better than the middle of the board
    // either, or the king gets herded to a corner where there is no mate.
    int64 wrongCorner = EvalWhite(Fen({ { strong, 'K' }, { C1, 'B' }, { knight, 'N' }, { A8, 'k' } }));
    int64 centre = EvalWhite(Fen({ { A1, 'K' }, { C2, 'B' }, { knight, 'N' }, { E5, 'k' } }));
    CHECK(wrongCorner < centre + BISHOP_VALUE);
}

// -- the search has to walk down that gradient to a mate ------------------

static SearchResult Go(Search& search, int depth) {
    search.ClearTables();
    SearchLimits limits;
    limits.maxDepth = depth;
    limits.maxTimeMs = -1;
    limits.maxNodes = 0;
    limits.useTablebase = false; // the answers must come from the search itself
    return search.Go(limits);
}

struct ConversionCase {
    const char* name;
    const char* fen;
    int depth;
    int maxPlies; // generous: this is a regression bound, not a distance to mate
};

// Play both sides at a fixed depth and see the mate arrive. The defending side
// is played by the same search, so it puts up the best fight it knows.
static void Convert(Search& search, const ConversionCase& conversion) {
    search.LoadPosition(conversion.fen);
    Position& position = search.m_Position;

    int64 finalScore = 0;
    for (int ply = 0; ply <= conversion.maxPlies; ply++) {
        if (GenerateMoves<ALL>(position).empty()) {
            if (!position.m_InCheck) {
                printf("  %s: stalemate after %d plies (%s)\n", conversion.name, ply, position.ToFen().c_str());
            }
            CHECK(position.m_InCheck);
            printf("  %-10s mate in %d plies\n", conversion.name, ply);
            // The move that mated has to have been played as a mate, not as a
            // material grab that happened to walk into one.
            CHECK(finalScore >= MATE_SCORE - MAX_DEPTH);
            return;
        }
        if (ply == conversion.maxPlies) break;

        SearchResult result = Go(search, conversion.depth);
        if (result.best == Move()) {
            printf("  %s: no move in %s\n", conversion.name, position.ToFen().c_str());
            CHECK(result.best != Move());
            return;
        }
        finalScore = result.score;
        position.MovePiece(result.best);
    }

    printf("  %s: no mate within %d plies (%s)\n", conversion.name, conversion.maxPlies, position.ToFen().c_str());
    CHECK(false);
}

// The search must never think it is winning when it cannot mate.
static void DrawnMaterialStaysDrawn(Search& search) {
    const char* const fens[] = {
        "8/8/8/4k3/8/8/8/KB6 w - - 0 1",
        "8/8/8/4k3/8/8/8/KN6 w - - 0 1",
        "8/8/8/4k3/8/8/8/KB6 b - - 0 1",
        "6bk/8/8/8/8/8/8/4K3 w - - 0 1",
    };
    for (const char* fen : fens) {
        search.LoadPosition(fen);
        SearchResult result = Go(search, 8);
        if (result.score != 0) printf("  %s scores %" PRId64 "\n", fen, result.score);
        CHECK_EQ(result.score, 0);
    }
}

// The bare king shortcut must not swallow pawn endgames on its way past.
static void PawnEndgames(Search& search) {
    // A pawn one square from queening is worth the queen it becomes, which
    // only holds if the bare king score takes over cleanly after promotion.
    search.LoadPosition("8/3P4/3K4/8/8/8/8/7k w - - 0 1");
    SearchResult result = Go(search, 10);
    printf("  KP vs K, pawn on the seventh, scores %" PRId64 "\n", result.score);
    CHECK(result.score > QUEEN_VALUE / 2);

    // The same pawn further back, with the black king too far away to stop it.
    search.LoadPosition("8/8/8/3K4/3P4/8/8/7k w - - 0 1");
    result = Go(search, 10);
    printf("  KP vs K, pawn on the fourth, scores %" PRId64 "\n", result.score);
    CHECK(result.score > 2 * PAWN_VALUE);

    // Two classic pawn endgame stalemates: no move, and no advantage.
    const char* const stalemates[] = {
        "8/8/8/8/8/1k6/1p6/1K6 w - - 0 1", // the pawn takes every square away
        "7k/7P/7K/8/8/8/8/8 b - - 0 1",    // rook pawn, the corner saves black
    };
    for (const char* fen : stalemates) {
        search.LoadPosition(fen);
        result = Go(search, 6);
        CHECK(GenerateMoves<ALL>(search.m_Position).empty());
        CHECK_EQ(result.best, Move());
        CHECK_EQ(result.score, 0);
    }
}

int main() {
    // Nothing here hashes a pawn structure worth keeping, so the table is only
    // present because Evaluate() takes one.
    g_PawnTable = std::make_unique<PawnTable>(4 * 1024);

    printf("-- bare king positions are recognised\n");
    Detection();

    printf("-- a lone minor is a draw\n");
    InsufficientMaterial();

    printf("-- mating material is worth its material\n");
    MatingMaterial();

    printf("-- the weak king is driven to a corner\n");
    DriveToCorner();

    printf("-- bishop and knight pick the corner the bishop covers\n");
    BishopKnightCorner();

    Search search(DEFAULT_HASH_MB);

    printf("-- drawn material stays drawn\n");
    DrawnMaterialStaysDrawn(search);

    printf("-- pawn endgames are still played out\n");
    PawnEndgames(search);

    printf("-- the search converts a bare king ending\n");
    const ConversionCase kConversions[] = {
        { "KQ centre", "8/8/8/4k3/8/8/8/K6Q w - - 0 1", 10, 50 },
        { "KQ edge", "7k/8/8/8/8/8/8/K6Q w - - 0 1", 10, 40 },
        { "KQ black", "7q/8/8/4K3/8/8/8/k7 b - - 0 1", 10, 50 },
        { "KR centre", "8/8/8/4k3/8/8/8/K6R w - - 0 1", 10, 70 },
        { "KR corner", "8/8/4k3/8/8/8/8/R3K3 w - - 0 1", 10, 60 },
        { "KRR centre", "8/8/8/4k3/8/8/8/K5RR w - - 0 1", 8, 30 },
    };
    for (const ConversionCase& conversion : kConversions) Convert(search, conversion);

    return TestSummary("test_endgame");
}
