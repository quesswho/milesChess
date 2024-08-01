#include "board.h"

#include <stdio.h>

Board::Board() {
	StartingPosition();
}

static uint64 FenToMap(std::string FEN, char p) {
    uint64_t i = 0;
    char c;
    int pos = 63;

    uint64_t result = 0;
    while ((c = FEN[i++]) != ' ')
    {
        uint64_t P = 1ull << pos;
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