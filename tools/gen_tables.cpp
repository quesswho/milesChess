// Offline generator for the constexpr tables committed in src/LookupTables.h.
//
// This is not part of the engine build. Each subcommand prints one table as C++
// literals on stdout, formatted to be pasted back into the corresponding array
// in LookupTables.h:
//
//     cmake --build build --target gen_tables
//     ./build/gen_tables knight
//
// `make check-tables` regenerates every table and diffs it against what is
// currently committed, so the pasted data stays verifiable.

#include <array>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <random>

#include "LookupTables.h"
#include "Utils.h"

namespace Lookup {

    constexpr uint64 InitFile(int p) {
        return (0x0101010101010101ull << (p % 8));
    }

    constexpr uint64 InitRank(int p) {
        return (0xFFull << ((p >> 3) << 3)); // Truncate the first 3 bits
    }

    constexpr uint64 InitDiagonal(int p) {
        int s = 8 * (p % 8) - ((p >> 3) << 3);
        return s > 0 ? 0x8040201008040201 >> (s) : 0x8040201008040201 << (-s);
    }

    constexpr uint64 InitAntiDiagonal(int p) {
        int s = 56 - 8 * (p % 8) - ((p >> 3) << 3);
        return s > 0 ? 0x0102040810204080 >> (s) : 0x0102040810204080 << (-s);
    }

    constexpr int CoordToPos(int x, int y) {
        return (x <= 0 || y <= 0 || x > 8 || y > 8) ? -1 : (y - 1) * 8 + (8 - x);
    }

    constexpr uint64 AddToMap(uint64 map, int x, int y) {
        int pos = CoordToPos(x, y);
        return (pos == -1) ? map : map | (1ull << pos);
    }

    // Table generator for lines;
    std::array<uint64, 64 * 4> InitSlider() {
        std::array<uint64, 64 * 4> result{};
        for (int i = 0; i < 64; i++) {
            result[i * 4] = InitFile(i);
            result[i * 4 + 1] = InitRank(i);
            result[i * 4 + 2] = InitDiagonal(i);
            result[i * 4 + 3] = InitAntiDiagonal(i);
        }
        return result;
    }

    // Table generator for knight_attacks
    std::array<uint64, 64> InitKnight() {
        std::array<uint64, 64> result{};
        for (int i = 0; i < 64; i++) {
            int x = i % 8 + 1;
            int y = i / 8 + 1;
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x + 2, y + 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x - 2, y + 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x + 2, y - 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x - 2, y - 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x + 1, y + 2);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x - 1, y + 2);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x + 1, y - 2);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x - 1, y - 2);
        }
        return result;
    }

    std::array<uint64, 64> InitKing() {
        std::array<uint64, 64> result{};
        for (int i = 0; i < 64; i++) {
            int x = i % 8 + 1;
            int y = i / 8 + 1;
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x + 1, y + 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x - 1, y + 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x + 1, y - 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x - 1, y - 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x, y + 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x, y - 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x + 1, y);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x - 1, y);
        }
        return result;
    }

    void PrintKingSafety(bool white) {
        std::array<uint64, 64> result{};
        for (int i = 0; i < 64; i++) {
            int x = i % 8 + 1;
            int y = i / 8 + 1;
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x + 1, y + 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x - 1, y + 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x + 1, y - 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x - 1, y - 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x, y + 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x, y - 1);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x + 1, y);
            result[CoordToPos(x, y)] = AddToMap(result[CoordToPos(x, y)], x - 1, y);
            if (white) {
                result[CoordToPos(x, y)] = result[CoordToPos(x, y)] | result[CoordToPos(x, y)] << 8;
            } else {
                result[CoordToPos(x, y)] = result[CoordToPos(x, y)] | result[CoordToPos(x, y)] >> 8;
            }
        }

        int i = 0;
        for (uint64 e : result) {
            printf("%#018" PRIx64 ", ", e);
            i++;
            if (i % 4 == 0) printf("\n");
        }
    }

    std::array<uint64, 64> InitRook() {
        std::array<uint64, 64> result{};
        for (int i = 0; i < 64; i++) {
            result[i] = InitFile(i) | InitRank(i);
        }
        return result;
    }

    std::array<uint64, 64> InitBishop() {
        std::array<uint64, 64> result{};
        for (int i = 0; i < 64; i++) {
            result[i] = InitDiagonal(i) | InitAntiDiagonal(i);
        }
        return result;
    }

    void PrintLineTable() {
        int i = 0;
        for (uint64 e : Lookup::InitSlider()) {
            printf("%#018" PRIx64 ", ", e);
            i++;
            if (i % 4 == 0) printf("\n");
        }
    }


    void PrintKnightTable() {
        int i = 0;
        for (uint64 e : Lookup::InitKnight()) {
            printf("%#018" PRIx64 ", ", e);
            i++;
            if (i % 4 == 0) printf("\n");
        }
    }

    void PrintKingTable() {
        int i = 0;
        for (uint64 e : Lookup::InitKing()) {
            printf("%#018" PRIx64 ", ", e);
            i++;
            if (i % 4 == 0) printf("\n");
        }
    }

    void PrintTable(std::array<uint64, 64> table) {
        int i = 0;
        for (uint64 e : table) {
            printf("%#018" PRIx64 ", ", e);
            i++;
            if (i % 4 == 0) printf("\n");
        }
    }
    std::array<uint64, 64 * 64> InitActiveMoves() {
        std::array<uint64, 64 * 64> result;
        for (int ksq = 0; ksq < 64; ksq++) {
            for (int enemysq = 0; enemysq < 64; enemysq++) {
                if (ksq == enemysq) {
                    result[ksq * 64 + enemysq] = 0;
                    continue;
                }
                uint64 kingb = 1ull << ksq;
                uint64 enemyb = 1ull << enemysq;
                if ((lines[ksq * 4 + 2] & enemyb) > 0) { // There is a diagonal attack
                    if (ksq > enemysq) {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb <<= 9;
                        }
                        result[ksq * 64 + enemysq] = active;
                    } else {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb >>= 9;
                        }
                        result[ksq * 64 + enemysq] = active;
                    }
                } else if ((lines[ksq * 4 + 3] & enemyb) > 0) { // There is a anti diagonal attack
                    if (ksq > enemysq) {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb <<= 7;
                        }
                        result[ksq * 64 + enemysq] = active;
                    } else {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb >>= 7;
                        }
                        result[ksq * 64 + enemysq] = active;
                    }
                } else if ((lines[ksq * 4] & enemyb) > 0) { // There is a file attack
                    if (ksq > enemysq) {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb <<= 8;
                        }
                        result[ksq * 64 + enemysq] = active;
                    } else {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb >>= 8;
                        }
                        result[ksq * 64 + enemysq] = active;
                    }
                } else if ((lines[ksq * 4 + 1] & enemyb) > 0) { // There is a file attack
                    if (ksq > enemysq) {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb <<= 1;
                        }
                        result[ksq * 64 + enemysq] = active;
                    } else {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb >>= 1;
                        }
                        result[ksq * 64 + enemysq] = active;
                    }
                } else {
                    result[ksq * 64 + enemysq] = 0;
                }
            }
        }
        return result;
    }

    std::array<uint64, 64 * 64> InitCheckMoves() {
        std::array<uint64, 64 * 64> result;
        for (int ksq = 0; ksq < 64; ksq++) {
            for (int enemysq = 0; enemysq < 64; enemysq++) {
                if (ksq == enemysq) {
                    result[ksq * 64 + enemysq] = 0;
                    continue;
                }
                if (ksq * 64 + enemysq == 664) {
                    //printf("");
                }
                uint64 kingb = 1ull << ksq;
                uint64 enemyb = 1ull << enemysq;
                if ((lines[ksq * 4 + 2] & enemyb) > 0) { // There is a diagonal attack
                    if (ksq > enemysq) {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb <<= 9;
                        }
                        if (((kingb & lines[7 * 4]) | (kingb & lines[8 * 7 * 4 + 1])) == 0) {
                            enemyb <<= 9;
                            active |= enemyb; // One square behind the king
                        }

                        result[ksq * 64 + enemysq] = active;
                    } else {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb >>= 9;
                        }
                        if (((kingb & lines[0]) | (kingb & lines[1])) == 0) {
                            enemyb >>= 9;
                            active |= enemyb; // One square behind the king
                        }
                        result[ksq * 64 + enemysq] = active;
                    }
                } else if ((lines[ksq * 4 + 3] & enemyb) > 0) { // There is a anti diagonal attack
                    if (ksq > enemysq) {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb <<= 7;
                        }
                        if (((kingb & lines[0]) | (kingb & lines[8 * 7 * 4 + 1])) == 0) {
                            enemyb <<= 7;
                            active |= enemyb; // One square behind the king
                        }
                        result[ksq * 64 + enemysq] = active;
                    } else {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb >>= 7;
                        }
                        if (((kingb & lines[7 * 4]) | (kingb & lines[1])) == 0) {
                            enemyb >>= 7;
                            active |= enemyb; // One square behind the king
                        }
                        result[ksq * 64 + enemysq] = active;
                    }
                } else if ((lines[ksq * 4] & enemyb) > 0) { // There is a file attack
                    if (ksq > enemysq) {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb <<= 8;
                        }
                        if ((kingb & lines[8 * 7 * 4 + 1]) == 0) {
                            enemyb <<= 8;
                            active |= enemyb; // One square behind the king
                        }
                        result[ksq * 64 + enemysq] = active;
                    } else {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb >>= 8;
                        }
                        if ((kingb & lines[1]) == 0) {
                            enemyb >>= 8;
                            active |= enemyb; // One square behind the king
                        }
                        result[ksq * 64 + enemysq] = active;
                    }
                } else if ((lines[ksq * 4 + 1] & enemyb) > 0) { // There is a rank attack
                    if (ksq > enemysq) {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb <<= 1;
                        }
                        if ((kingb & lines[7 * 4]) == 0) {
                            enemyb <<= 1;
                            active |= enemyb; // One square behind the king
                        }
                        result[ksq * 64 + enemysq] = active;
                    } else {
                        uint64 active = 0;
                        while (enemyb != kingb) {
                            active |= enemyb;
                            enemyb >>= 1;
                        }
                        if ((kingb & lines[0]) == 0) {
                            enemyb >>= 1;
                            active |= enemyb; // One square behind the king
                        }
                        result[ksq * 64 + enemysq] = active;
                    }
                } else {
                    result[ksq * 64 + enemysq] = 0;
                }
            }
        }
        return result;
    }

    void PrintActiveMoves() {
        int i = 0;
        for (uint64 e : InitActiveMoves()) {
            printf("%#018" PRIx64 ", ", e);
            i++;
            if (i % 4 == 0) printf("\n");
        }
    }

    void PrintCheckMoves() {
        int i = 0;
        for (uint64 e : InitCheckMoves()) {
            printf("%#018" PRIx64 ", ", e);
            i++;
            if (i % 4 == 0) printf("\n");
        }
    }

    void PrintWhitePassedPawnTable() {
        for (int i = 0; i < 64; i++) {
            uint64 map = 0;
            for (int j = i + 8; j < 64; j += 8) map |= 1ull << j;
            if (i % 8 != 0) {
                for (int j = i + 7; j < 64; j += 8) map |= 1ull << j;
            }
            if (i % 8 != 7) {
                for (int j = i + 9; j < 64; j += 8) map |= 1ull << j;
            }
            printf("%#018" PRIx64 ", ", map);
            if ((i + 1) % 4 == 0) printf("\n");
        }
    }

    void PrintBlackPassedPawnTable() {
        for (int i = 0; i < 64; i++) {
            uint64 map = 0;
            for (int j = i - 8; j > 0; j -= 8) map |= 1ull << j;
            if (i % 8 != 0) {
                for (int j = i - 9; j > 0; j -= 8) map |= 1ull << j;
            }
            if (i % 8 != 7) {
                for (int j = i - 7; j > 0; j -= 8) map |= 1ull << j;
            }
            printf("%#018" PRIx64 ", ", map);
            if ((i + 1) % 4 == 0) printf("\n");
        }
    }

    void PrintWhiteForwardPawnTable() {
        for (int i = 0; i < 64; i++) {
            uint64 map = 0;
            for (int j = i + 8; j < 64; j += 8) map |= 1ull << j;
            printf("%#018" PRIx64 ", ", map);
            if ((i + 1) % 4 == 0) printf("\n");
        }
    }

    void PrintBlackForwardPawnTable() {
        for (int i = 0; i < 64; i++) {
            uint64 map = 0;
            for (int j = i - 8; j > 0; j -= 8) map |= 1ull << j;
            printf("%#018" PRIx64 ", ", map);
            if ((i + 1) % 4 == 0) printf("\n");
        }
    }

    void PrintIsolatedPawnMask() {
        for (int i = 0; i < 64; i++) {
            uint64 map = 0;
            if ((i % 8) != 0)
                for (int j = (i - 1) % 8; j < 64; j += 8) map |= 1ull << j;
            if ((i % 8) != 7)
                for (int j = (i + 1) % 8; j < 64; j += 8) map |= 1ull << j;
            printf("%#018" PRIx64 ", ", map);
            if ((i + 1) % 4 == 0) printf("\n");
        }
    }
    void PrintZobristConstants() {
        uint64 seed = 9966442212445; // just random number

        std::mt19937_64 mt(seed);

        for (int i = 1; i <= 781; i++) {
            printf("%#018" PRIx64 ", ", mt());
            if (i % 4 == 0) printf("\n");
        }
    }


} // namespace Lookup

namespace {

    struct Generator {
        const char* name;
        void (*run)();
    };

    constexpr Generator generators[] = {
        { "line", Lookup::PrintLineTable },
        { "knight", Lookup::PrintKnightTable },
        { "king", Lookup::PrintKingTable },
        { "rook", [] { Lookup::PrintTable(Lookup::InitRook()); } },
        { "bishop", [] { Lookup::PrintTable(Lookup::InitBishop()); } },
        { "king-safety-white", [] { Lookup::PrintKingSafety(true); } },
        { "king-safety-black", [] { Lookup::PrintKingSafety(false); } },
        { "active-moves", Lookup::PrintActiveMoves },
        { "check-moves", Lookup::PrintCheckMoves },
        { "passed-pawn-white", Lookup::PrintWhitePassedPawnTable },
        { "passed-pawn-black", Lookup::PrintBlackPassedPawnTable },
        { "forward-pawn-white", Lookup::PrintWhiteForwardPawnTable },
        { "forward-pawn-black", Lookup::PrintBlackForwardPawnTable },
        { "isolated-pawn", Lookup::PrintIsolatedPawnMask },
        { "zobrist", Lookup::PrintZobristConstants },
    };

    void Usage() {
        printf("usage: gen_tables <table>\n\ntables:\n");
        for (const Generator& g : generators) printf("  %s\n", g.name);
    }

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        Usage();
        return 1;
    }
    if (strcmp(argv[1], "all") == 0) {
        for (const Generator& g : generators) {
            printf("// ---- %s ----\n", g.name);
            g.run();
            printf("\n");
        }
        return 0;
    }
    for (const Generator& g : generators) {
        if (strcmp(argv[1], g.name) == 0) {
            g.run();
            return 0;
        }
    }
    fprintf(stderr, "unknown table: %s\n", argv[1]);
    Usage();
    return 1;
}
