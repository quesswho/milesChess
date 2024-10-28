#pragma once
#include <bit>
#include <cassert>
#include <chrono>
#include <stdarg.h>

#include "Types.h"

#define GET_SQUARE(X) _tzcnt_u64(X)
#define COUNT_BIT(X) _mm_popcnt_u64(X)

struct Score {
    Score()
        : mg(0), eg(0)
    {}

    Score(int mg, int eg)
        : mg(mg), eg(eg)
    {}

    int mg;
    int eg;

    Score& operator+=(const Score& other) {
        mg += other.mg;
        eg += other.eg;
        return *this;
    }

    Score& operator-=(const Score& other) {
        mg -= other.mg;
        eg -= other.eg;
        return *this;
    }
};

static inline uint64 PopBit(uint64& val) {
    uint64 result = (val & -val);
    val &= (val - 1);
    return result;
}

static inline int LSB(uint64& val) {
    unsigned long r;
    _BitScanForward64(&r, val);
    return r;
}

static inline int PopPos(BitBoard& val) {
    int index = int(LSB(val)); // 0 returns 0
    //int index = int(_tzcnt_u64(val));
    val &= val - 1;
    return index;
}

static inline int Signum(int val) {
    return (0 < val) - (val < 0);
}

static BitBoard FenToMap(const std::string& FEN, char p) {
    int i = 0;
    char c = {};
    int pos = 63;

    BitBoard result = 0;
    while ((c = FEN[i++]) != ' ') {
        BitBoard P = 1ull << pos;
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

static BoardInfo FenToBoardInfo(const std::string& FEN) {
    BoardInfo info = {};
    int i = 0;
    while (FEN[i++] != ' ') {
        // Skip piece placement
    }

    char ac = FEN[i++]; // Active color
    switch (ac) {
    case 'w':
        info.m_WhiteMove = WHITE;
        break;
    case 'b':
        info.m_WhiteMove = BLACK;
        break;
    default:
        printf("Invalid FEN for active color\n");
        return info;
    }

    i++;
    info.m_BlackCastleKing = false;
    info.m_BlackCastleQueen = false;
    info.m_WhiteCastleKing = false;
    info.m_WhiteCastleQueen = false;
    char c = {};
    while ((c = FEN[i++]) != ' ') {
        if (c == '-') {
            break;
        }

        switch (c) {
        case 'K':
            info.m_WhiteCastleKing = true;
            break;
        case 'Q':
            info.m_WhiteCastleQueen = true;
            break;
        case 'k':
            info.m_BlackCastleKing = true;
            break;
        case 'q':
            info.m_BlackCastleQueen = true;
            break;
        }
    }

    while ((c = FEN[i]) == ' ') i++;
    char a = FEN[i++];

    if (a != '-') {
        if (info.m_WhiteMove) {
            info.m_EnPassant = 1ull << (40 + ('h' - a)); // TODO: need to be tested
        } else {
            info.m_EnPassant = 1ull << (16 + ('h' - a)); // TODO: need to be tested
        }
    } else {
        info.m_EnPassant = 0;
    }

    info.m_HalfMoves = 0;
    info.m_FullMoves = 0;
    int t = 1;

    while ((c = FEN[i++]) != ' ');
    while ((c = FEN[i++]) != ' ') info.m_HalfMoves = info.m_HalfMoves * 10 + (int)(c - '0');
    while (i < FEN.size() && FEN[i] >= '0' && FEN[i] <= '9') {
        c = FEN[i++];
        info.m_FullMoves = info.m_FullMoves * 10 + (c - '0');
    }
    return info;
}

static void PrintBitBoard(BitBoard map) {
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

static int sync_printf(const char* format, ...) {
    static std::mutex lock;
    std::lock_guard guard{ lock };
    va_list args;
    va_start(args, format);
    setbuf(stdout, NULL);
    return vprintf(format, args);
}

// Remove double whitespaces and \t characters from command
static std::string trim_str(std::string str) {
    for (int i = 0; i < str.length() - 1; i++) {
        if ((str.at(i) == ' ' && str.at(i + 1) == ' ') || str.at(i) == '\t') {
            str.erase(str.begin() + i);
            i--;
        }
    }
    return str;
}

class Timer {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;

    float m_LastElapsed;
public:
    Timer()
        : m_LastElapsed(0.0f)
    {}

    void Start() { m_Start = std::chrono::high_resolution_clock::now(); }
    float End() { return std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - m_Start).count(); } // Returns x seconds after Start() was called
    float EndMs() { return std::chrono::duration_cast<std::chrono::milliseconds > (std::chrono::high_resolution_clock::now() - m_Start).count(); }
};

#define GetLower(S) ((1ull << S) - 1)
#define GetUpper(S) (0xFFFFFFFFFFFFFFFF << (S))

struct lineEx {
    BitBoard lower;
    BitBoard upper;
    BitBoard uni;

    constexpr BitBoard init_low(int sq, BitBoard line) {
        BitBoard lineEx = line ^ (1ull << sq);
        return GetLower(sq) & lineEx;
    }

    constexpr BitBoard init_up(int sq, BitBoard line) {
        BitBoard lineEx = line ^ (1ull << sq);
        return GetUpper(sq) & lineEx;
    }

    constexpr lineEx() : lower(0), upper(0), uni(0) {

    }

    constexpr lineEx(int sq, BitBoard line) : lower(init_low(sq, line)), upper(init_up(sq, line)), uni(init_low(sq, line) | init_up(sq, line))
    {}
};

#undef GetLower
#undef GetUpper