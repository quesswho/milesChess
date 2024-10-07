#pragma once
#include "board.h"



namespace TableBase {

    // Change this for more tablebase pieces
#define TBPIECES 5

#define TBHASHBITS 10

#define TBMAX_PIECE 254
#define TBMAX_PAWN 256
#define HSHMAX 5

    struct TBEntry {
        char* data;
        uint64 key;
        uint64 mapping;
        ubyte ready;
        ubyte num;
        ubyte symmetric;
        ubyte has_pawns;
    };

    struct TBEntry_piece {
        char* data;
        uint64 key;
        uint64 mapping;
        ubyte ready;
        ubyte num;
        ubyte symmetric;
        ubyte has_pawns;
        ubyte enc_type;
        struct PairsData* precomp[2];
        int factor[2][TBPIECES];
        ubyte pieces[2][TBPIECES];
        ubyte norm[2][TBPIECES];
    };

    struct TBEntry_pawn {
        char* data;
        uint64 key;
        uint64 mapping;
        ubyte ready;
        ubyte num;
        ubyte symmetric;
        ubyte has_pawns;
        ubyte pawns[2];
        struct {
            struct PairsData* precomp[2];
            int factor[2][TBPIECES];
            ubyte pieces[2][TBPIECES];
            ubyte norm[2][TBPIECES];
        } file[4];
    };

    struct TBHashEntry {
        uint64 key;
        struct TBEntry* ptr;
    };

	void Init(std::string path);
}
