#include "TestUtil.h"
#include "Positions.h"

#include "Transposition.h" // Evaluate.h uses PawnTable but does not include it
#include "Evaluate.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>
#include <vector>

// The evaluation must not care which colour is which: flipping a position
// vertically and swapping the colour of every piece has to leave the score
// (which is relative to the side to move) untouched.

// Vertically flip the board and swap colours. Castling rights swap case, the
// en passant square mirrors rank, and the side to move flips.
static std::string MirrorFen(const std::string& fen) {
    std::vector<std::string> fields;
    for (size_t i = 0; i < fen.size();) {
        while (i < fen.size() && fen[i] == ' ') i++;
        size_t start = i;
        while (i < fen.size() && fen[i] != ' ') i++;
        if (i > start) fields.push_back(fen.substr(start, i - start));
    }
    while (fields.size() < 6) fields.push_back(fields.size() == 4 ? "0" : "1");

    auto swapCase = [](char c) {
        return std::isalpha((unsigned char)c)
                   ? (char)(std::islower((unsigned char)c) ? std::toupper(c) : std::tolower(c))
                   : c;
    };

    std::vector<std::string> ranks;
    std::string cur;
    for (char c : fields[0]) {
        if (c == '/') {
            ranks.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    ranks.push_back(cur);

    std::string board;
    for (size_t i = ranks.size(); i-- > 0;) {
        for (char c : ranks[i]) board += swapCase(c);
        if (i) board += '/';
    }

    std::string castle;
    for (char want : std::string("KQkq")) {
        if (fields[2].find(swapCase(want)) != std::string::npos) castle += want;
    }
    if (castle.empty()) castle = "-";

    std::string ep = fields[3];
    if (ep != "-" && ep.size() == 2) ep[1] = (char)('0' + (9 - (ep[1] - '0')));

    return board + " " + (fields[1] == "w" ? "b" : "w") + " " + castle + " " + ep + " " + fields[4] + " " + fields[5];
}

static std::unique_ptr<PawnTable> g_PawnTable;

static int64 Eval(const std::string& fen) {
    Position pos;
    pos.SetPosition(fen);
    // The pawn hash ignores side to move, so a stale entry from the unmirrored
    // position could mask a real asymmetry. Start from an empty table.
    g_PawnTable->Clear();
    return Evaluate(pos, g_PawnTable.get());
}

static void CheckMirror(const std::string& fen, const char* what) {
    std::string mirror = MirrorFen(fen);
    int64 a = Eval(fen), b = Eval(mirror);
    if (a != b) {
        printf("  asymmetric %s\n    %-52s %6" PRId64 "\n    %-52s %6" PRId64 "\n", what, fen.c_str(), (int64_t)a,
               mirror.c_str(), (int64_t)b);
    }
    CHECK_EQ(a, b);
}

// Build a FEN from a board indexed the usual way: square = rank * 8 + file,
// with file 0 = 'a' and rank 0 = '1'.
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

static std::string SquareName(int square) {
    return std::string(1, (char)('a' + square % 8)) + (char)('1' + square / 8);
}

// White king on e1, black king on e8, plus whatever the caller places.
static const int kWhiteKing = 4, kBlackKing = 60;

static std::string WithKings(const std::vector<std::pair<int, char>>& pieces) {
    char squares[64] = {};
    squares[kWhiteKing] = 'K';
    squares[kBlackKing] = 'k';
    for (auto& piece : pieces) squares[piece.first] = piece.second;
    return MakeFen(squares, true);
}

// A white piece alone on every square it can stand on. Catches piece square
// tables and mobility terms that are read with the wrong square for one side.
static void SinglePieceSweep() {
    for (char piece : std::string("PNBRQ")) {
        for (int square = 0; square < 64; square++) {
            if (square == kWhiteKing || square == kBlackKing) continue;
            if (piece == 'P' && (square < 8 || square >= 56)) continue;
            std::string what = std::string(1, piece) + SquareName(square);
            CheckMirror(WithKings({ { square, piece } }), what.c_str());
        }
    }
}

// Both kings alone, walked over every legal pair of squares.
static void KingSweep() {
    for (int white = 0; white < 64; white++) {
        for (int black = 0; black < 64; black++) {
            int fileGap = std::abs(white % 8 - black % 8), rankGap = std::abs(white / 8 - black / 8);
            if (fileGap <= 1 && rankGap <= 1) continue; // same or adjacent square
            char squares[64] = {};
            squares[white] = 'K';
            squares[black] = 'k';
            std::string what = "K" + SquareName(white) + " k" + SquareName(black);
            CheckMirror(MakeFen(squares, true), what.c_str());
        }
    }
}

// Doubled pawns are detected with a forward-file mask, which is file sensitive.
static void DoubledPawnSweep() {
    for (int file = 0; file < 8; file++) {
        for (int rank = 2; rank < 7; rank++) {
            std::string what = "doubled " + SquareName(8 + file) + "+" + SquareName(rank * 8 + file);
            CheckMirror(WithKings({ { 8 + file, 'P' }, { rank * 8 + file, 'P' } }), what.c_str());
        }
    }
}

// Isolated pawns are detected with an adjacent-file mask, also file sensitive.
// Place a pawn on the fourth rank and a second pawn on every other file, so
// each case is isolated or not depending on the correct file being tested.
static void IsolatedPawnSweep() {
    for (int file = 0; file < 8; file++) {
        for (int other = 0; other < 8; other++) {
            if (other == file) continue;
            std::string what = "isolated " + SquareName(24 + file) + " with " + SquareName(8 + other);
            CheckMirror(WithKings({ { 24 + file, 'P' }, { 8 + other, 'P' } }), what.c_str());
        }
    }
}

// Passed pawn detection, including the defended-passer bonus.
static void PassedPawnSweep() {
    for (int file = 0; file < 8; file++) {
        for (int blocker = 0; blocker < 8; blocker++) {
            std::vector<std::pair<int, char>> pieces = { { 24 + file, 'P' }, { 40 + blocker, 'p' } };
            if (file > 0) pieces.push_back({ 16 + file - 1, 'P' }); // supporting pawn
            std::string what = "passer " + SquareName(24 + file) + " vs " + SquareName(40 + blocker);
            CheckMirror(WithKings(pieces), what.c_str());
        }
    }
}

// Pieces bearing down on the enemy king, which exercises the king safety masks.
// Each side gets the same piece on vertically mirrored squares, so the position
// is its own mirror and any difference has to come from an asymmetric term.
static void KingAttackSweep() {
    for (char piece : std::string("NBRQ")) {
        for (int square = 0; square < 64; square++) {
            int mirrored = square ^ 56;
            if (square == kWhiteKing || square == kBlackKing) continue;
            if (mirrored == kWhiteKing || mirrored == kBlackKing) continue;
            std::string what = std::string("attack ") + piece + SquareName(square);
            CheckMirror(WithKings({ { square, piece }, { mirrored, (char)std::tolower(piece) } }), what.c_str());
        }
    }
}

// Random material scatters, to cover interactions the targeted sweeps miss.
static void RandomSweep(int count) {
    std::mt19937 rng(20240608);
    const char* const white = "PPPPPPPPNNBBRRQ";
    const char* const black = "ppppppppnnbbrrq";

    for (int i = 0; i < count; i++) {
        int order[64];
        for (int j = 0; j < 64; j++) order[j] = j;
        std::shuffle(order, order + 64, rng);

        char squares[64] = {};
        int next = 0;
        squares[order[next++]] = 'K';
        squares[order[next++]] = 'k';
        int whiteCount = (int)(rng() % 8) + 1, blackCount = (int)(rng() % 8) + 1;
        for (int j = 0; j < whiteCount; j++) {
            int square = order[next++];
            char piece = white[rng() % 15];
            if (piece == 'P' && (square < 8 || square >= 56)) piece = 'N';
            squares[square] = piece;
        }
        for (int j = 0; j < blackCount; j++) {
            int square = order[next++];
            char piece = black[rng() % 15];
            if (piece == 'p' && (square < 8 || square >= 56)) piece = 'n';
            squares[square] = piece;
        }
        CheckMirror(MakeFen(squares, true), "random");
    }
}

int main() {
    // Small on purpose: every probe is preceded by a full clear, so a big table
    // would only make the test slower.
    g_PawnTable = std::make_unique<PawnTable>(4 * 1024);

    printf("-- mirror helper\n");
    CHECK_EQ_STR(MirrorFen(kStartPos), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    CHECK_EQ_STR(MirrorFen("4k3/8/8/8/4pP2/8/8/4K3 b - f3 0 1"), "4k3/8/8/4Pp2/8/8/8/4K3 w - f6 0 1");
    CHECK_EQ_STR(MirrorFen("r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1"), "r3k2r/8/8/8/8/8/8/R3K2R b Qk - 0 1");

    printf("-- named positions\n");
    const char* const fens[] = { kStartPos, kKiwipete, kEndgame, kPromo, kMidgame, kComplex };
    const char* const names[] = { "startpos", "kiwipete", "endgame", "promo", "midgame", "complex" };
    for (size_t i = 0; i < 6; i++) CheckMirror(fens[i], names[i]);

    printf("-- single piece sweep\n");
    SinglePieceSweep();

    printf("-- king sweep\n");
    KingSweep();

    printf("-- doubled pawns\n");
    DoubledPawnSweep();

    printf("-- isolated pawns\n");
    IsolatedPawnSweep();

    printf("-- passed pawns\n");
    PassedPawnSweep();

    printf("-- king attacks\n");
    KingAttackSweep();

    printf("-- random positions\n");
    RandomSweep(5000);

    return TestSummary("test_eval_symmetry");
}
