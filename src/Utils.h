#pragma once
#include <bit>

using uint64 = unsigned long long;

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

static uint64 Slide(uint64 pos, uint64 line, uint64 block) {
    block = block & ~pos & line;

    uint64 bottom = pos - 1ull;

    uint64 mask_up = block & ~bottom;
    uint64 block_up = (mask_up ^ (mask_up - 1ull));

    uint64 mask_down = block & bottom;
    uint64 block_down = (0x7FFFFFFFFFFFFFFF) >> _lzcnt_u64(mask_down | 1);

    return (block_up ^ block_down) & line;
}

static uint64 SpecialSlide(uint64 pos, uint64 line, uint64 block) { // Allows us to block with the pos
    block = block & line;

    uint64 bottom = pos - 1ull;

    uint64 mask_up = block & ~bottom;
    uint64 block_up = (mask_up ^ (mask_up - 1ull));

    uint64 mask_down = block & bottom;
    uint64 block_down = (0x7FFFFFFFFFFFFFFF) >> _lzcnt_u64(mask_down | 1);

    return (block_up ^ block_down) & line;
}