/*
  Copyright (c) 2011-2015 Ronald de Man
  This file may be redistributed and/or modified without restrictions.
*/

#include "TableBase.h"
namespace TableBase {

    static std::string s_Path;

    static char pchr[] = { 'K', 'Q', 'R', 'B', 'N', 'P' };

    

    static int TBnum_piece, TBnum_pawn;
    static TBEntry_piece TB_piece[TBMAX_PIECE];
    static TBEntry_pawn TB_pawn[TBMAX_PAWN];

    static TBHashEntry TB_hash[1 << TBHASHBITS][HSHMAX];


    static uint64 PCSHashKey(int* pcs, bool mirror) {
        int color = mirror ? 8 : 0;
        int pt;
        int i;
        uint64 key = 0;


        for (pt = (int)PieceType::PAWN; pt <= (int)PieceType::KING; pt++)
            for (i = 0; i < pcs[color + pt]; i++)
                key ^= Lookup::zobrist[i + (int)(pt - 1) * 128];
        color ^= 8;
        for (pt = (int)PieceType::PAWN; pt <= (int)PieceType::KING; pt++)
            for (i = 0; i < pcs[color + pt]; i++)
                key ^= Lookup::zobrist[64 + i + (int)(pt - 1) * 128];

        return key;
    }

    static void InsertTB(struct TBEntry* ptr, uint64 key) {
        int i, hshidx;

        hshidx = key >> (64 - TBHASHBITS);
        i = 0;
        while (i < HSHMAX && TB_hash[hshidx][i].ptr) {
            i++;
        }
        if (i == HSHMAX) {
            printf("HSHMAX too low!\n");
            exit(1);
        } else {
            TB_hash[hshidx][i].key = key;
            TB_hash[hshidx][i].ptr = ptr;
        }
    }

    static void InitTB(std::string tablebase) {
        int pcs[16];
        int i,j;

        // TODO: Check if file exists

        for (i = 0; i < 16; i++) {
            pcs[i] = 0;
        }
        int color = 0;
        for (char& s : tablebase) {
            switch (s) {
            case 'P':
                pcs[(int)PieceType::PAWN | color]++;
                break;
            case 'N':
                pcs[(int)PieceType::KNIGHT | color]++;
                break;
            case 'B':
                pcs[(int)PieceType::BISHOP | color]++;
                break;
            case 'R':
                pcs[(int)PieceType::ROOK | color]++;
                break;
            case 'Q':
                pcs[(int)PieceType::QUEEN | color]++;
                break;
            case 'K':
                pcs[(int)PieceType::KING | color]++;
                break;
            case 'v':
                color = 0x08;
                break;
            }
        }

        TBEntry* entry;

        uint64 key = PCSHashKey(pcs, false);
        uint64 key2 = PCSHashKey(pcs, true);
        if (pcs[(int)PieceType::PAWN] + pcs[(int)PieceType::PAWN | 8] == 0) {
            if (TBnum_piece == TBMAX_PIECE) {
                printf("TBMAX_PIECE limit too low!\n");
                exit(1);
            }
            entry = (TBEntry*)&TB_piece[TBnum_piece++];
        } else {
            if (TBnum_pawn == TBMAX_PAWN) {
                printf("TBMAX_PAWN limit too low!\n");
                exit(1);
            }
            entry = (TBEntry*)&TB_pawn[TBnum_pawn++];
        }

        entry->key = key;
        entry->ready = 0;
        entry->num = 0;
        for (i = 0; i < 16; i++)
            entry->num += pcs[i];
        entry->symmetric = (key == key2);
        entry->has_pawns = (pcs[(int)PieceType::PAWN] + pcs[(int)PieceType::PAWN | 8] > 0);

        if (entry->has_pawns) {
            TBEntry_pawn* ptr = (TBEntry_pawn*)entry;
            ptr->pawns[0] = pcs[(int)PieceType::PAWN];
            ptr->pawns[1] = pcs[(int)PieceType::PAWN | 8];
            if (pcs[(int)PieceType::PAWN | 8] > 0
                && (pcs[(int)PieceType::PAWN] == 0 || pcs[(int)PieceType::PAWN | 8] < pcs[(int)PieceType::PAWN])) {
                ptr->pawns[0] = pcs[(int)PieceType::PAWN | 8];
                ptr->pawns[1] = pcs[(int)PieceType::PAWN];
            }
        } else {
            TBEntry_piece* ptr = (TBEntry_piece*)entry;
            for (i = 0, j = 0; i < 16; i++)
                if (pcs[i] == 1) j++;
            if (j >= 3) ptr->enc_type = 0;
            else ptr->enc_type = 2;
        }
        InsertTB(entry, key);
        if (key2 != key) InsertTB(entry, key2);

    }

    static const signed char offdiag[] = {
      0,-1,-1,-1,-1,-1,-1,-1,
      1, 0,-1,-1,-1,-1,-1,-1,
      1, 1, 0,-1,-1,-1,-1,-1,
      1, 1, 1, 0,-1,-1,-1,-1,
      1, 1, 1, 1, 0,-1,-1,-1,
      1, 1, 1, 1, 1, 0,-1,-1,
      1, 1, 1, 1, 1, 1, 0,-1,
      1, 1, 1, 1, 1, 1, 1, 0
    };

    static const ubyte triangle[] = {
      6, 0, 1, 2, 2, 1, 0, 6,
      0, 7, 3, 4, 4, 3, 7, 0,
      1, 3, 8, 5, 5, 8, 3, 1,
      2, 4, 5, 9, 9, 5, 4, 2,
      2, 4, 5, 9, 9, 5, 4, 2,
      1, 3, 8, 5, 5, 8, 3, 1,
      0, 7, 3, 4, 4, 3, 7, 0,
      6, 0, 1, 2, 2, 1, 0, 6
    };

    static const ubyte invtriangle[] = {
      1, 2, 3, 10, 11, 19, 0, 9, 18, 27
    };

    static const ubyte invdiag[] = {
      0, 9, 18, 27, 36, 45, 54, 63,
      7, 14, 21, 28, 35, 42, 49, 56
    };

    static const ubyte flipdiag[] = {
       0,  8, 16, 24, 32, 40, 48, 56,
       1,  9, 17, 25, 33, 41, 49, 57,
       2, 10, 18, 26, 34, 42, 50, 58,
       3, 11, 19, 27, 35, 43, 51, 59,
       4, 12, 20, 28, 36, 44, 52, 60,
       5, 13, 21, 29, 37, 45, 53, 61,
       6, 14, 22, 30, 38, 46, 54, 62,
       7, 15, 23, 31, 39, 47, 55, 63
    };

    static const ubyte lower[] = {
      28,  0,  1,  2,  3,  4,  5,  6,
       0, 29,  7,  8,  9, 10, 11, 12,
       1,  7, 30, 13, 14, 15, 16, 17,
       2,  8, 13, 31, 18, 19, 20, 21,
       3,  9, 14, 18, 32, 22, 23, 24,
       4, 10, 15, 19, 22, 33, 25, 26,
       5, 11, 16, 20, 23, 25, 34, 27,
       6, 12, 17, 21, 24, 26, 27, 35
    };

    static const ubyte diag[] = {
       0,  0,  0,  0,  0,  0,  0,  8,
       0,  1,  0,  0,  0,  0,  9,  0,
       0,  0,  2,  0,  0, 10,  0,  0,
       0,  0,  0,  3, 11,  0,  0,  0,
       0,  0,  0, 12,  4,  0,  0,  0,
       0,  0, 13,  0,  0,  5,  0,  0,
       0, 14,  0,  0,  0,  0,  6,  0,
      15,  0,  0,  0,  0,  0,  0,  7
    };

    static const ubyte flap[] = {
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 6, 12, 18, 18, 12, 6, 0,
      1, 7, 13, 19, 19, 13, 7, 1,
      2, 8, 14, 20, 20, 14, 8, 2,
      3, 9, 15, 21, 21, 15, 9, 3,
      4, 10, 16, 22, 22, 16, 10, 4,
      5, 11, 17, 23, 23, 17, 11, 5,
      0, 0, 0, 0, 0, 0, 0, 0
    };

    static const ubyte ptwist[] = {
      0, 0, 0, 0, 0, 0, 0, 0,
      47, 35, 23, 11, 10, 22, 34, 46,
      45, 33, 21, 9, 8, 20, 32, 44,
      43, 31, 19, 7, 6, 18, 30, 42,
      41, 29, 17, 5, 4, 16, 28, 40,
      39, 27, 15, 3, 2, 14, 26, 38,
      37, 25, 13, 1, 0, 12, 24, 36,
      0, 0, 0, 0, 0, 0, 0, 0
    };

    static const ubyte invflap[] = {
      8, 16, 24, 32, 40, 48,
      9, 17, 25, 33, 41, 49,
      10, 18, 26, 34, 42, 50,
      11, 19, 27, 35, 43, 51
    };

    static const ubyte invptwist[] = {
      52, 51, 44, 43, 36, 35, 28, 27, 20, 19, 12, 11,
      53, 50, 45, 42, 37, 34, 29, 26, 21, 18, 13, 10,
      54, 49, 46, 41, 38, 33, 30, 25, 22, 17, 14, 9,
      55, 48, 47, 40, 39, 32, 31, 24, 23, 16, 15, 8
    };

    static const ubyte file_to_file[] = {
      0, 1, 2, 3, 3, 2, 1, 0
    };

    static const short KK_idx[10][64] = {
      { -1, -1, -1,  0,  1,  2,  3,  4,
        -1, -1, -1,  5,  6,  7,  8,  9,
        10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25,
        26, 27, 28, 29, 30, 31, 32, 33,
        34, 35, 36, 37, 38, 39, 40, 41,
        42, 43, 44, 45, 46, 47, 48, 49,
        50, 51, 52, 53, 54, 55, 56, 57 },
      { 58, -1, -1, -1, 59, 60, 61, 62,
        63, -1, -1, -1, 64, 65, 66, 67,
        68, 69, 70, 71, 72, 73, 74, 75,
        76, 77, 78, 79, 80, 81, 82, 83,
        84, 85, 86, 87, 88, 89, 90, 91,
        92, 93, 94, 95, 96, 97, 98, 99,
       100,101,102,103,104,105,106,107,
       108,109,110,111,112,113,114,115},
      {116,117, -1, -1, -1,118,119,120,
       121,122, -1, -1, -1,123,124,125,
       126,127,128,129,130,131,132,133,
       134,135,136,137,138,139,140,141,
       142,143,144,145,146,147,148,149,
       150,151,152,153,154,155,156,157,
       158,159,160,161,162,163,164,165,
       166,167,168,169,170,171,172,173 },
      {174, -1, -1, -1,175,176,177,178,
       179, -1, -1, -1,180,181,182,183,
       184, -1, -1, -1,185,186,187,188,
       189,190,191,192,193,194,195,196,
       197,198,199,200,201,202,203,204,
       205,206,207,208,209,210,211,212,
       213,214,215,216,217,218,219,220,
       221,222,223,224,225,226,227,228 },
      {229,230, -1, -1, -1,231,232,233,
       234,235, -1, -1, -1,236,237,238,
       239,240, -1, -1, -1,241,242,243,
       244,245,246,247,248,249,250,251,
       252,253,254,255,256,257,258,259,
       260,261,262,263,264,265,266,267,
       268,269,270,271,272,273,274,275,
       276,277,278,279,280,281,282,283 },
      {284,285,286,287,288,289,290,291,
       292,293, -1, -1, -1,294,295,296,
       297,298, -1, -1, -1,299,300,301,
       302,303, -1, -1, -1,304,305,306,
       307,308,309,310,311,312,313,314,
       315,316,317,318,319,320,321,322,
       323,324,325,326,327,328,329,330,
       331,332,333,334,335,336,337,338 },
      { -1, -1,339,340,341,342,343,344,
        -1, -1,345,346,347,348,349,350,
        -1, -1,441,351,352,353,354,355,
        -1, -1, -1,442,356,357,358,359,
        -1, -1, -1, -1,443,360,361,362,
        -1, -1, -1, -1, -1,444,363,364,
        -1, -1, -1, -1, -1, -1,445,365,
        -1, -1, -1, -1, -1, -1, -1,446 },
      { -1, -1, -1,366,367,368,369,370,
        -1, -1, -1,371,372,373,374,375,
        -1, -1, -1,376,377,378,379,380,
        -1, -1, -1,447,381,382,383,384,
        -1, -1, -1, -1,448,385,386,387,
        -1, -1, -1, -1, -1,449,388,389,
        -1, -1, -1, -1, -1, -1,450,390,
        -1, -1, -1, -1, -1, -1, -1,451 },
      {452,391,392,393,394,395,396,397,
        -1, -1, -1, -1,398,399,400,401,
        -1, -1, -1, -1,402,403,404,405,
        -1, -1, -1, -1,406,407,408,409,
        -1, -1, -1, -1,453,410,411,412,
        -1, -1, -1, -1, -1,454,413,414,
        -1, -1, -1, -1, -1, -1,455,415,
        -1, -1, -1, -1, -1, -1, -1,456 },
      {457,416,417,418,419,420,421,422,
        -1,458,423,424,425,426,427,428,
        -1, -1, -1, -1, -1,429,430,431,
        -1, -1, -1, -1, -1,432,433,434,
        -1, -1, -1, -1, -1,435,436,437,
        -1, -1, -1, -1, -1,459,438,439,
        -1, -1, -1, -1, -1, -1,460,440,
        -1, -1, -1, -1, -1, -1, -1,461 }
    };

    static int binomial[5][64];
    static int pawnidx[5][24];
    static int pfactor[5][4];

    void TableBase::Init(std::string path) {
        s_Path = path;
        char str[16];
        int i, j, k,l;

        // binomial[k-1][n] = Bin(n, k)
        for (i = 0; i < 5; i++) {
            for (j = 0; j < 64; j++) {
                int f = j;
                int l = 1;
                for (k = 1; k <= i; k++) {
                    f *= (j - k);
                    l *= (k + 1);
                }
                binomial[i][j] = f / l;
            }
        }

        for (i = 0; i < 5; i++) {
            int s = 0;
            for (j = 0; j < 6; j++) {
                pawnidx[i][j] = s;
                s += (i == 0) ? 1 : binomial[i - 1][ptwist[invflap[j]]];
            }
            pfactor[i][0] = s;
            s = 0;
            for (; j < 12; j++) {
                pawnidx[i][j] = s;
                s += (i == 0) ? 1 : binomial[i - 1][ptwist[invflap[j]]];
            }
            pfactor[i][1] = s;
            s = 0;
            for (; j < 18; j++) {
                pawnidx[i][j] = s;
                s += (i == 0) ? 1 : binomial[i - 1][ptwist[invflap[j]]];
            }
            pfactor[i][2] = s;
            s = 0;
            for (; j < 24; j++) {
                pawnidx[i][j] = s;
                s += (i == 0) ? 1 : binomial[i - 1][ptwist[invflap[j]]];
            }
            pfactor[i][3] = s;
        }

        TBnum_piece = TBnum_pawn = 0;

        for (i = 0; i < (1 << TBHASHBITS); i++) {
            for (j = 0; j < HSHMAX; j++) {
                TB_hash[i][j].key = 0ULL;
                TB_hash[i][j].ptr = NULL;
            }
        }

        for (i = 1; i < 6; i++) {
            sprintf(str, "K%cvK", pchr[i]); // Copy
            InitTB(str);
        }

        for (i = 1; i < 6; i++) {
            for (j = i; j < 6; j++) {
                sprintf(str, "K%cvK%c", pchr[i], pchr[j]);
                InitTB(str);
            }
        }

        for (i = 1; i < 6; i++) {
            for (j = i; j < 6; j++) {
                sprintf(str, "K%c%cvK", pchr[i], pchr[j]);
                InitTB(str);
            }
        }

        for (i = 1; i < 6; i++) {
            for (j = i; j < 6; j++) {
                for (k = 1; k < 6; k++) {
                    sprintf(str, "K%c%cvK%c", pchr[i], pchr[j], pchr[k]);
                    InitTB(str);
                }
            }
        }
        for (i = 1; i < 6; i++) {
            for (j = i; j < 6; j++) {
                for (k = j; k < 6; k++) {
                    sprintf(str, "K%c%c%cvK", pchr[i], pchr[j], pchr[k]);
                    InitTB(str);
                }
            }
        }

        printf("Found %d tablebases.\n", TBnum_piece + TBnum_pawn);
    }
}