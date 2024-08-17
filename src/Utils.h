#pragma once
#include <bit>

using uint64 = unsigned long long;
using int64 = long long;

static uint64 PopBit(uint64& val) {
    uint64 result = (val & -val);
    val &= (val - 1);
    return result;
}

static int PopPos(uint64& val) {
    int index = int(_tzcnt_u64(val));
    val &= val - 1;
    return index;
}

static uint64 FenToMap(const std::string& FEN, char p) {
    int i = 0;
    char c = {};
    int pos = 63;

    uint64 result = 0;
    while ((c = FEN[i++]) != ' ') {
        uint64 P = 1ull << pos;
        switch (c) {
        case '/': pos += 1; break;
        case '1': break;
        case '2': pos -= 1; break;
        case '3': pos -= 2; break;
        case '4': pos -= 3; break;
        case '5': pos -= 4; break;
        case '6': pos -= 5; break;
        case '7': pos -= 6; break;
        case '8': pos -= 7; break;
        default:
            if (c == p) result |= P;
        }
        pos--;
    }
    return result;
}

static void PrintMap(uint64 map) {
    for (int i = 63; i >= 0; i--) {
        if ((map & (1ull << i)) > 0) {
            printf("X ");
        }
        else {
            printf("O ");
        }
        if (i % 8 == 0) printf("\n");
    }
}

#define GetLower(S) ((1ull << S) - 1)
#define GetUpper(S) (0xFFFFFFFFFFFFFFFF << (S))

struct lineEx {
    uint64_t lower;
    uint64_t upper;
    uint64_t uni;

    constexpr uint64_t init_low(int sq, uint64_t line) {
        uint64_t lineEx = line ^ (1ull << sq);
        return GetLower(sq) & lineEx;
    }

    constexpr uint64_t init_up(int sq, uint64_t line) {
        uint64_t lineEx = line ^ (1ull << sq);
        return GetUpper(sq) & lineEx;
    }

    constexpr lineEx() : lower(0), upper(0), uni(0) {

    }

    constexpr lineEx(int sq, uint64_t line) : lower(init_low(sq, line)), upper(init_up(sq, line)), uni(init_low(sq, line) | init_up(sq, line))
    {}
};

#undef GetLower
#undef GetUpper

/*
 * @author Michael Hoffmann, Daniel Infuehr
 * Obstruction difference slider algorithm
 */
static uint64 Slide(uint64 pos, lineEx line, uint64 block) {
    uint64_t lower = (line.lower & block) | 1;
    uint64_t upper = line.upper & block;
    uint64_t msb = (0x8000000000000000) >> std::countl_zero(lower);
    uint64_t lsb = upper & -upper;
    uint64_t oDif = lsb * 2 - msb;
    return line.uni & oDif;
}

