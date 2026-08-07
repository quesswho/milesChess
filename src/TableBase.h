#pragma once
#include "Position.h"


namespace TableBase {

    // Change this for more tablebase pieces
#define TBPIECES 5

#define TBHASHBITS 10

#define TBMAX_PIECE 254
#define TBMAX_PAWN  256
#define HSHMAX      5

#define WDL_MAGIC 0x5d23e871
#define DTZ_MAGIC 0xa50c66d7

#define WDLSUFFIX ".rtbw"
#define DTZSUFFIX ".rtbz"

    using FD = int;
    inline constexpr FD FD_ERR = -1;

    struct PairsData {
        char* indextable;
        ushort* sizetable;
        uint8* data;
        ushort* offset;
        uint8* symlen;
        uint8* sympat;
        int blocksize;
        int idxbits;
        int min_len;
        uint32 base[1];
    };

    struct TBEntry {
        char* data;
        uint64 key;
        uint64 mapping;
        uint8 ready;
        uint8 num;
        uint8 symmetric;
        uint8 has_pawns;
    };

    struct TBEntry_piece {
        char* data;
        uint64 key;
        uint64 mapping;
        uint8 ready;
        uint8 num;
        uint8 symmetric;
        uint8 has_pawns;
        uint8 enc_type;
        struct PairsData* precomp[2];
        int factor[2][TBPIECES];
        uint8 pieces[2][TBPIECES];
        uint8 norm[2][TBPIECES];
    };

    struct TBEntry_pawn {
        char* data;
        uint64 key;
        uint64 mapping;
        uint8 ready;
        uint8 num;
        uint8 symmetric;
        uint8 has_pawns;
        uint8 pawns[2];
        struct {
            struct PairsData* precomp[2];
            int factor[2][TBPIECES];
            uint8 pieces[2][TBPIECES];
            uint8 norm[2][TBPIECES];
        } file[4];
    };

    struct TBHashEntry {
        uint64 key;
        struct TBEntry* ptr;
    };

    struct DTZEntry_piece {
        char* data;
        uint64 key;
        uint64 mapping;
        uint8 ready;
        uint8 num;
        uint8 symmetric;
        uint8 has_pawns;
        uint8 enc_type;
        PairsData* precomp;
        int factor[TBPIECES];
        uint8 pieces[TBPIECES];
        uint8 norm[TBPIECES];
        uint8 flags; // accurate, mapped, side
        ushort map_idx[4];
        uint8* map;
    };

    struct DTZEntry_pawn {
        char* data;
        uint64 key;
        uint64 mapping;
        uint8 ready;
        uint8 num;
        uint8 symmetric;
        uint8 has_pawns;
        uint8 pawns[2];
        struct {
            PairsData* precomp;
            int factor[TBPIECES];
            uint8 pieces[TBPIECES];
            uint8 norm[TBPIECES];
        } file[4];
        uint8 flags[4];
        ushort map_idx[4][4];
        uint8* map;
    };

    struct DTZTableEntry {
        uint64 key1;
        uint64 key2;
        struct TBEntry* entry;
    };

    void Init(std::string path);

    int Probe_WDL(Position& board, int* success);
    int Probe_DTZ(Position& board, int* success);
}
