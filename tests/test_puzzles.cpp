#include "TestUtil.h"
#include "Puzzles.h"

#include "Search.h"
#include "MoveGen.h"

#include <string>
#include <vector>

// Space separated UCI move list, as written in PuzzleCase::best / ::avoid
static bool ListContains(const char* list, const std::string& move) {
    std::string entry;
    for (const char* c = list;; c++) {
        if (*c != ' ' && *c != '\0') {
            entry += *c;
            continue;
        }
        if (entry == move) return true;
        if (*c == '\0') return false;
        entry.clear();
    }
}

static bool IsMateScore(int64 score) {
    return score >= MATE_SCORE - MAX_DEPTH || score <= -MATE_SCORE + MAX_DEPTH;
}

static bool IsLegal(Position& position, Move move) {
    for (Move legal : GenerateMoves<ALL>(position)) {
        if (legal == move) return true;
    }
    return false;
}

static std::string BestString(const SearchResult& result) {
    return result.best != Move() ? MoveToString(result.best) : "0000";
}

// Cleared tables, fixed depth and no tablebase, so the answer only depends on
// the search itself and not on the puzzles that ran before it
static SearchResult Go(Search& search, int depth) {
    search.ClearTables();
    SearchLimits limits;
    limits.maxDepth = depth;
    limits.maxTimeMs = -1;
    limits.maxNodes = 0;
    limits.useTablebase = false;
    return search.Go(limits);
}

static SearchResult GoFen(Search& search, const char* fen, int depth) {
    search.LoadPosition(fen);
    return Go(search, depth);
}

// Play the search against every defence and see the mate actually land within
// `movesLeft` moves, whatever distance the score claims.
static bool ForcesMate(Search& search, int movesLeft, int depth) {
    if (movesLeft <= 0) return false;

    Position& position = search.m_Position;
    SearchResult result = Go(search, depth);
    if (result.best == Move() || !IsLegal(position, result.best)) {
        printf("    no move in %s\n", position.ToFen().c_str());
        return false;
    }

    position.MovePiece(result.best);

    std::vector<Move> replies = GenerateMoves<ALL>(position);
    bool forced = replies.empty() ? position.m_InCheck : true;
    for (Move reply : replies) {
        position.MovePiece(reply);
        bool mates = ForcesMate(search, movesLeft - 1, depth);
        position.UndoMove(reply);
        if (!mates) {
            forced = false;
            break;
        }
    }

    if (!forced) {
        printf("    %s does not mate in %d (%s)\n", MoveToString(result.best).c_str(), movesLeft,
               position.ToFen().c_str());
    }
    position.UndoMove(result.best);
    return forced;
}

static void CheckPuzzle(Search& search, const PuzzleCase& puzzle) {
    SearchResult result = GoFen(search, puzzle.fen, puzzle.depth);
    std::string best = BestString(result);

    printf("  %-16s depth %2d  best %-5s  score %6" PRId64 "  nodes %9" PRIu64 "\n", puzzle.name, puzzle.depth,
           best.c_str(), result.score, result.nodes);

    CHECK(result.best != Move());
    CHECK(IsLegal(search.m_Position, result.best));

    if (puzzle.best[0] != '\0' && !ListContains(puzzle.best, best)) {
        printf("    %s: played %s, expected one of '%s'\n", puzzle.name, best.c_str(), puzzle.best);
    }
    if (puzzle.best[0] != '\0') CHECK(ListContains(puzzle.best, best));

    if (puzzle.avoid[0] != '\0' && ListContains(puzzle.avoid, best)) {
        printf("    %s: played %s, which is on the avoid list '%s'\n", puzzle.name, best.c_str(), puzzle.avoid);
    }
    if (puzzle.avoid[0] != '\0') CHECK(!ListContains(puzzle.avoid, best));

    if (puzzle.mate != 0) {
        if (!IsMateScore(result.score) || (result.score > 0) != (puzzle.mate > 0)) {
            printf("    %s: score %" PRId64 " is not a mate score for %s\n", puzzle.name, result.score,
                   puzzle.mate > 0 ? "the side to move" : "the opponent");
        }
        CHECK(IsMateScore(result.score));
        CHECK((result.score > 0) == (puzzle.mate > 0));
    } else {
        if (result.score < puzzle.minScore || result.score > puzzle.maxScore) {
            printf("    %s: score %" PRId64 " outside [%d, %d]\n", puzzle.name, result.score, puzzle.minScore,
                   puzzle.maxScore);
        }
        CHECK(result.score >= puzzle.minScore);
        CHECK(result.score <= puzzle.maxScore);
    }
}

int main() {
    Search search(DEFAULT_HASH_MB);

    printf("-- puzzle answers\n");
    for (const PuzzleCase& puzzle : kPuzzles) CheckPuzzle(search, puzzle);

    printf("-- forced mates hold up against every defence\n");
    for (const MateCase& mate : kMates) {
        search.LoadPosition(mate.fen);
        bool forced = ForcesMate(search, mate.moves, mate.depth);
        if (!forced) printf("  %s: no forced mate in %d found\n", mate.name, mate.moves);
        CHECK(forced);
    }

    // Clearing has to reset whole entries, not just their hashes, or the
    // replacement counters of the previous search decide this one as well
    printf("-- a cleared engine gives the same answer again\n");
    for (const PuzzleCase& puzzle : kPuzzles) {
        SearchResult first = GoFen(search, puzzle.fen, puzzle.depth);
        SearchResult second = GoFen(search, puzzle.fen, puzzle.depth);
        if (first.best != second.best || first.score != second.score || first.nodes != second.nodes) {
            printf("  %s: %s/%" PRId64 "/%" PRIu64 " then %s/%" PRId64 "/%" PRIu64 "\n", puzzle.name,
                   BestString(first).c_str(), first.score, first.nodes, BestString(second).c_str(), second.score,
                   second.nodes);
        }
        CHECK_EQ(second.best, first.best);
        CHECK_EQ(second.score, first.score);
        CHECK_EQ(second.nodes, first.nodes);
    }

    printf("-- terminal positions report no move\n");
    SearchResult stalemate = GoFen(search, kTerminalStalemate, 4);
    CHECK(GenerateMoves<ALL>(search.m_Position).empty());
    CHECK_EQ(stalemate.best, Move());
    CHECK_EQ(stalemate.score, 0);

    SearchResult checkmate = GoFen(search, kTerminalMate, 4);
    CHECK(GenerateMoves<ALL>(search.m_Position).empty());
    CHECK_EQ(checkmate.best, Move());
    CHECK_EQ(checkmate.score, -MATE_SCORE);

    return TestSummary("test_puzzles");
}
