/*
  Copyright (c) 2011-2015 Ronald de Man
  This file may be redistributed and/or modified without restrictions.
*/

#include "TableBase.h"
#include "LookupTables.h"
#include "MoveGen.h"

#include <windows.h>

namespace TableBase {

#define Swap(a,b) {int tmp=a;a=b;b=tmp;}

    static std::string s_Path;

    static char pchr[] = { 'K', 'Q', 'R', 'B', 'N', 'P' };

    static int TBnum_piece, TBnum_pawn;
    static TBEntry_piece TB_piece[TBMAX_PIECE];
    static TBEntry_pawn TB_pawn[TBMAX_PAWN];

    static TBHashEntry TB_hash[1 << TBHASHBITS][HSHMAX];

#define DTZ_ENTRIES 64

    static struct DTZTableEntry DTZ_table[DTZ_ENTRIES];

    static int binomial[5][64];
    static int pawnidx[5][24];
    static int pfactor[5][4];

    // Given a position return the table base string such as KQPvKRP

    static void TB_Str(const Position& board, char* str, bool mirror) {
        bool color = mirror;
        int pt;
        int i;

        for (pt = (int)PieceType::KING; pt >= (int)PieceType::PAWN; pt--) {
            for (i = COUNT_BIT(board.m_Pieces[pt][color]); i > 0; i--) {
                *str++ = pchr[6 - pt];
            }
        }
        *str++ = 'v';
        color = !color;
        for (pt = (int)PieceType::KING; pt >= (int)PieceType::PAWN; pt--) {
            for (i = COUNT_BIT(board.m_Pieces[pt][color]); i > 0; i--) {
                *str++ = pchr[6 - pt];
            }
        }
        *str++ = 0;
    }

    static uint64 Material_Key(const Position& board) {
        const uint8_t wp = COUNT_BIT(board.m_WhitePawn), wkn = COUNT_BIT(board.m_WhiteKnight), wb = COUNT_BIT(board.m_WhiteBishop), wr = COUNT_BIT(board.m_WhiteRook), wq = COUNT_BIT(board.m_WhiteQueen), wk = COUNT_BIT(board.m_WhiteKing),
            bp = COUNT_BIT(board.m_BlackPawn), bkn = COUNT_BIT(board.m_BlackKnight), bb = COUNT_BIT(board.m_BlackBishop), br = COUNT_BIT(board.m_BlackRook), bq = COUNT_BIT(board.m_BlackQueen), bk = COUNT_BIT(board.m_BlackKing);
        
        uint64 key = 0;
        if (wp) {
            key ^= Lookup::zobrist[wp-1];
        }
        if (bp) {
            key ^= Lookup::zobrist[64+bp-1];
        }
        if (wkn) {
            key ^= Lookup::zobrist[128 + wkn - 1];
        }
        if (bkn) {
            key ^= Lookup::zobrist[64+128 + wkn - 1];
        }
        if (wb) {
            key ^= Lookup::zobrist[128*2 + wb - 1];
        }
        if (bb) {
            key ^= Lookup::zobrist[64+128 * 2 + bb - 1];
        }
        if (wr) {
            key ^= Lookup::zobrist[128 * 3 + wr - 1];
        }
        if (br) {
            key ^= Lookup::zobrist[64 + 128 * 3 + br - 1];
        }
        if (wq) {
            key ^= Lookup::zobrist[128 * 4 + wq - 1];
        }
        if (bq) {
            key ^= Lookup::zobrist[64 + 128 * 4 + bq - 1];
        }

        // White and black king are always present
        key ^= Lookup::zobrist[128 * 5];
        key ^= Lookup::zobrist[128 * 5 + 64];

        return key;
    }

    static uint64 Calc_Key(const Position& board, bool mirror) {
        bool color = mirror;
        int pt;
        int i;
        uint64 key = 0;

        for (pt = (int)PieceType::KING; pt >= (int)PieceType::PAWN; pt--) {
            if((i = COUNT_BIT(board.m_Pieces[pt][color])) > 0) {
                key ^= Lookup::zobrist[color*64 + (pt-1)*128 + i - 1];
            }
        }
        color = !color;
        for (pt = (int)PieceType::KING; pt >= (int)PieceType::PAWN; pt--) {
            if ((i = COUNT_BIT(board.m_Pieces[pt][color])) > 0) {
                key ^= Lookup::zobrist[color * 64 + (pt - 1) * 128 + i - 1];
            }
        }

        return key;
    }

    static void set_norm_piece(TBEntry_piece* ptr, ubyte* norm, ubyte* pieces) {
        int i, j;

        for (i = 0; i < ptr->num; i++)
            norm[i] = 0;

        switch (ptr->enc_type) {
        case 0:
            norm[0] = 3;
            break;
        default:
            norm[0] = 2;
            break;
        }

        for (i = norm[0]; i < ptr->num; i += norm[i])
            for (j = i; j < ptr->num && pieces[j] == pieces[i]; j++)
                norm[i]++;
    }

    // place k like pieces on n squares
    static int subfactor(int k, int n) {
        int i, f, l;

        f = n;
        l = 1;
        for (i = 1; i < k; i++) {
            f *= n - i;
            l *= i + 1;
        }

        return f / l;
    }

    static uint64 calc_factors_pawn(int* factor, int num, int order, int order2, ubyte* norm, int file) {
        int i, k, n;
        uint64 f;

        i = norm[0];
        if (order2 < 0x0f) i += norm[i];
        n = 64 - i;

        f = 1;
        for (k = 0; i < num || k == order || k == order2; k++) {
            if (k == order) {
                factor[0] = f;
                f *= pfactor[norm[0] - 1][file];
            } else if (k == order2) {
                factor[norm[0]] = f;
                f *= subfactor(norm[norm[0]], 48 - norm[0]);
            } else {
                factor[i] = f;
                f *= subfactor(norm[i], n);
                n -= norm[i];
                i += norm[i];
            }
        }

        return f;
    }

    static void set_norm_pawn(struct TBEntry_pawn* ptr, ubyte* norm, ubyte* pieces) {
        int i, j;

        for (i = 0; i < ptr->num; i++)
            norm[i] = 0;

        norm[0] = ptr->pawns[0];
        if (ptr->pawns[1]) norm[ptr->pawns[0]] = ptr->pawns[1];

        for (i = ptr->pawns[0] + ptr->pawns[1]; i < ptr->num; i += norm[i])
            for (j = i; j < ptr->num && pieces[j] == pieces[i]; j++)
                norm[i]++;
    }


    static uint64 calc_factors_piece(int* factor, int num, int order, ubyte* norm, ubyte enc_type) {
        int i, k, n;
        uint64 f;
        static int pivfac[] = { 31332, 28056, 462 };

        n = 64 - norm[0];

        f = 1;
        for (i = norm[0], k = 0; i < num || k == order; k++) {
            if (k == order) {
                factor[0] = f;
                f *= pivfac[enc_type];
            } else {
                factor[i] = f;
                f *= subfactor(norm[i], n);
                n -= norm[i];
                i += norm[i];
            }
        }

        return f;
    }

    static void setup_pieces_piece(struct TBEntry_piece* ptr, unsigned char* data, uint64* tb_size) {
        int i;
        int order;

        for (i = 0; i < ptr->num; i++)
            ptr->pieces[0][i] = data[i + 1] & 0x0f;
        order = data[0] & 0x0f;
        set_norm_piece(ptr, ptr->norm[0], ptr->pieces[0]);
        tb_size[0] = calc_factors_piece(ptr->factor[0], ptr->num, order, ptr->norm[0], ptr->enc_type);

        for (i = 0; i < ptr->num; i++)
            ptr->pieces[1][i] = data[i + 1] >> 4;
        order = data[0] >> 4;
        set_norm_piece(ptr, ptr->norm[1], ptr->pieces[1]);
        tb_size[1] = calc_factors_piece(ptr->factor[1], ptr->num, order, ptr->norm[1], ptr->enc_type);
    }

    static void setup_pieces_piece_dtz(struct DTZEntry_piece* ptr, unsigned char* data, uint64* tb_size) {
        int i;
        int order;

        for (i = 0; i < ptr->num; i++)
            ptr->pieces[i] = data[i + 1] & 0x0f;
        order = data[0] & 0x0f;
        set_norm_piece((struct TBEntry_piece*)ptr, ptr->norm, ptr->pieces);
        tb_size[0] = calc_factors_piece(ptr->factor, ptr->num, order, ptr->norm, ptr->enc_type);
    }

    static void setup_pieces_pawn(struct TBEntry_pawn* ptr, unsigned char* data, uint64* tb_size, int f) {
        int i, j;
        int order, order2;

        j = 1 + (ptr->pawns[1] > 0);
        order = data[0] & 0x0f;
        order2 = ptr->pawns[1] ? (data[1] & 0x0f) : 0x0f;
        for (i = 0; i < ptr->num; i++)
            ptr->file[f].pieces[0][i] = data[i + j] & 0x0f;
        set_norm_pawn(ptr, ptr->file[f].norm[0], ptr->file[f].pieces[0]);
        tb_size[0] = calc_factors_pawn(ptr->file[f].factor[0], ptr->num, order, order2, ptr->file[f].norm[0], f);

        order = data[0] >> 4;
        order2 = ptr->pawns[1] ? (data[1] >> 4) : 0x0f;
        for (i = 0; i < ptr->num; i++)
            ptr->file[f].pieces[1][i] = data[i + j] >> 4;
        set_norm_pawn(ptr, ptr->file[f].norm[1], ptr->file[f].pieces[1]);
        tb_size[1] = calc_factors_pawn(ptr->file[f].factor[1], ptr->num, order, order2, ptr->file[f].norm[1], f);
    }

    static void setup_pieces_pawn_dtz(struct DTZEntry_pawn* ptr, unsigned char* data, uint64* tb_size, int f) {
        int i, j;
        int order, order2;

        j = 1 + (ptr->pawns[1] > 0);
        order = data[0] & 0x0f;
        order2 = ptr->pawns[1] ? (data[1] & 0x0f) : 0x0f;
        for (i = 0; i < ptr->num; i++)
            ptr->file[f].pieces[i] = data[i + j] & 0x0f;
        set_norm_pawn((TBEntry_pawn*)ptr, ptr->file[f].norm, ptr->file[f].pieces);
        tb_size[0] = calc_factors_pawn(ptr->file[f].factor, ptr->num, order, order2, ptr->file[f].norm, f);
    }

    static void calc_symlen(struct PairsData* d, int s, char* tmp) {
        int s1, s2;

        int w = *(int*)(d->sympat + 3 * s);
        s2 = (w >> 12) & 0x0fff;
        if (s2 == 0x0fff)
            d->symlen[s] = 0;
        else {
            s1 = w & 0x0fff;
            if (!tmp[s1]) calc_symlen(d, s1, tmp);
            if (!tmp[s2]) calc_symlen(d, s2, tmp);
            d->symlen[s] = d->symlen[s1] + d->symlen[s2] + 1;
        }
        tmp[s] = 1;
    }

    static PairsData* setup_pairs(unsigned char* data, uint64 tb_size, uint64* size, unsigned char** next, ubyte* flags, int wdl) {
        PairsData* d;
        int i;

        *flags = data[0];
        if (data[0] & 0x80) {
            d = (PairsData*)malloc(sizeof(PairsData));
            d->idxbits = 0;
            if (wdl)
                d->min_len = data[1];
            else
                d->min_len = 0;
            *next = data + 2;
            size[0] = size[1] = size[2] = 0;
            return d;
        }

        int blocksize = data[1];
        int idxbits = data[2];
        int real_num_blocks = *(uint32*)(&data[4]);
        int num_blocks = real_num_blocks + *(ubyte*)(&data[3]);
        int max_len = data[8];
        int min_len = data[9];
        int h = max_len - min_len + 1;
        int num_syms = *(ushort*)(&data[10 + 2 * h]);
        d = (struct PairsData*)malloc(sizeof(struct PairsData) + (h - 1) * sizeof(uint32) + num_syms);
        d->blocksize = blocksize;
        d->idxbits = idxbits;
        d->offset = (ushort*)(&data[10]);
        d->symlen = ((ubyte*)d) + sizeof(struct PairsData) + (h - 1) * sizeof(uint32);
        d->sympat = &data[12 + 2 * h];
        d->min_len = min_len;
        *next = &data[12 + 2 * h + 3 * num_syms + (num_syms & 1)];

        int num_indices = (tb_size + (1ULL << idxbits) - 1) >> idxbits;
        size[0] = 6ULL * num_indices;
        size[1] = 2ULL * num_blocks;
        size[2] = (1ULL << blocksize) * real_num_blocks;

        // char tmp[num_syms];
        char tmp[4096];
        for (i = 0; i < num_syms; i++)
            tmp[i] = 0;
        for (i = 0; i < num_syms; i++)
            if (!tmp[i])
                calc_symlen(d, i, tmp);

        d->base[h - 1] = 0;
        for (i = h - 2; i >= 0; i--)
            d->base[i] = (d->base[i + 1] + d->offset[i] - d->offset[i + 1]) / 2;

        for (i = 0; i < h; i++)
            d->base[i] <<= 32 - (min_len + i);

        d->offset -= d->min_len;

        return d;
    }

    static FD Open_TB(const char* str, const char* suffix) {

        char file[256];
        strcpy(file, s_Path.c_str());
        strcat(file, "/");
        strcat(file, str);
        strcat(file, suffix);
        HANDLE fd = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fd != FD_ERR) return fd;

        return FD_ERR;
    }

    static void Close_TB(FD fd) {
        CloseHandle(fd);
    }

    static char* Map_TB(const char* name, const char* suffix, uint64* mapping) {
        FD fd = Open_TB(name, suffix);
        if (fd == FD_ERR)
            return NULL;
        DWORD size_low, size_high;
        size_low = GetFileSize(fd, &size_high);
        HANDLE map = CreateFileMapping(fd, NULL, PAGE_READONLY, size_high, size_low,
            NULL);
        if (map == NULL) {
            printf("CreateFileMapping() failed.\n");
            exit(1);
        }
        *mapping = (uint64)map;
        char* data = (char*)MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
        if (data == NULL) {
            printf("MapViewOfFile() failed, name = %s%s, error = %lu.\n", name, suffix, GetLastError());
            exit(1);
        }

        Close_TB(fd);
        return data;
    }

    static void Unmap_TB(char* data, uint64 mapping) {
        if (!data) return;
        UnmapViewOfFile(data);
        CloseHandle((HANDLE)mapping);
    }

    static int Load_Wdl(TBEntry* entry, char* str) {
        ubyte* next;
        int f, s;
        uint64 tb_size[8];
        uint64 size[8 * 3];
        ubyte flags;

        entry->data = Map_TB(str, WDLSUFFIX, &entry->mapping);
        if (!entry->data) {
            printf("Could not find %s" WDLSUFFIX "\n", str);
            return 0;
        }

        ubyte* data = (ubyte*)entry->data;
        if (((uint32*)data)[0] != WDL_MAGIC) {
            printf("Corrupted table.\n");
            Unmap_TB(entry->data, entry->mapping);
            entry->data = 0;
            return 0;
        }

        int split = data[4] & 0x01;
        int files = data[4] & 0x02 ? 4 : 1;

        data += 5;

        if (!entry->has_pawns) {
            TBEntry_piece* ptr = (TBEntry_piece*)entry;
            setup_pieces_piece(ptr, data, &tb_size[0]);
            data += ptr->num + 1;
            data += ((uintptr_t)data) & 0x01;

            ptr->precomp[0] = setup_pairs(data, tb_size[0], &size[0], &next, &flags, 1);
            data = next;
            if (split) {
                ptr->precomp[1] = setup_pairs(data, tb_size[1], &size[3], &next, &flags, 1);
                data = next;
            } else
                ptr->precomp[1] = NULL;

            ptr->precomp[0]->indextable = (char*)data;
            data += size[0];
            if (split) {
                ptr->precomp[1]->indextable = (char*)data;
                data += size[3];
            }

            ptr->precomp[0]->sizetable = (ushort*)data;
            data += size[1];
            if (split) {
                ptr->precomp[1]->sizetable = (ushort*)data;
                data += size[4];
            }

            data = (ubyte*)((((uintptr_t)data) + 0x3f) & ~0x3f);
            ptr->precomp[0]->data = data;
            data += size[2];
            if (split) {
                data = (ubyte*)((((uintptr_t)data) + 0x3f) & ~0x3f);
                ptr->precomp[1]->data = data;
            }
        } else {
            struct TBEntry_pawn* ptr = (struct TBEntry_pawn*)entry;
            s = 1 + (ptr->pawns[1] > 0);
            for (f = 0; f < 4; f++) {
                setup_pieces_pawn((struct TBEntry_pawn*)ptr, data, &tb_size[2 * f], f);
                data += ptr->num + s;
            }
            data += ((uintptr_t)data) & 0x01;

            for (f = 0; f < files; f++) {
                ptr->file[f].precomp[0] = setup_pairs(data, tb_size[2 * f], &size[6 * f], &next, &flags, 1);
                data = next;
                if (split) {
                    ptr->file[f].precomp[1] = setup_pairs(data, tb_size[2 * f + 1], &size[6 * f + 3], &next, &flags, 1);
                    data = next;
                } else
                    ptr->file[f].precomp[1] = NULL;
            }

            for (f = 0; f < files; f++) {
                ptr->file[f].precomp[0]->indextable = (char*)data;
                data += size[6 * f];
                if (split) {
                    ptr->file[f].precomp[1]->indextable = (char*)data;
                    data += size[6 * f + 3];
                }
            }

            for (f = 0; f < files; f++) {
                ptr->file[f].precomp[0]->sizetable = (ushort*)data;
                data += size[6 * f + 1];
                if (split) {
                    ptr->file[f].precomp[1]->sizetable = (ushort*)data;
                    data += size[6 * f + 4];
                }
            }

            for (f = 0; f < files; f++) {
                data = (ubyte*)((((uintptr_t)data) + 0x3f) & ~0x3f);
                ptr->file[f].precomp[0]->data = data;
                data += size[6 * f + 2];
                if (split) {
                    data = (ubyte*)((((uintptr_t)data) + 0x3f) & ~0x3f);
                    ptr->file[f].precomp[1]->data = data;
                    data += size[6 * f + 5];
                }
            }
        }

        return 1;
    }

    static int Load_DTZ(struct TBEntry* entry) {
        ubyte* data = (ubyte*)entry->data;
        ubyte* next;
        int f, s;
        uint64 tb_size[4];
        uint64 size[4 * 3];

        if (!data)
            return 0;

        if (((uint32*)data)[0] != DTZ_MAGIC) {
            printf("Corrupted table.\n");
            return 0;
        }

        int files = data[4] & 0x02 ? 4 : 1;

        data += 5;

        if (!entry->has_pawns) {
            struct DTZEntry_piece* ptr = (struct DTZEntry_piece*)entry;
            setup_pieces_piece_dtz(ptr, data, &tb_size[0]);
            data += ptr->num + 1;
            data += ((uintptr_t)data) & 0x01;

            ptr->precomp = setup_pairs(data, tb_size[0], &size[0], &next, &(ptr->flags), 0);
            data = next;

            ptr->map = data;
            if (ptr->flags & 2) {
                int i;
                for (i = 0; i < 4; i++) {
                    ptr->map_idx[i] = (data + 1 - ptr->map);
                    data += 1 + data[0];
                }
                data += ((uintptr_t)data) & 0x01;
            }

            ptr->precomp->indextable = (char*)data;
            data += size[0];

            ptr->precomp->sizetable = (ushort*)data;
            data += size[1];

            data = (ubyte*)((((uintptr_t)data) + 0x3f) & ~0x3f);
            ptr->precomp->data = data;
            data += size[2];
        } else {
            struct DTZEntry_pawn* ptr = (struct DTZEntry_pawn*)entry;
            s = 1 + (ptr->pawns[1] > 0);
            for (f = 0; f < 4; f++) {
                setup_pieces_pawn_dtz(ptr, data, &tb_size[f], f);
                data += ptr->num + s;
            }
            data += ((uintptr_t)data) & 0x01;

            for (f = 0; f < files; f++) {
                ptr->file[f].precomp = setup_pairs(data, tb_size[f], &size[3 * f], &next, &(ptr->flags[f]), 0);
                data = next;
            }

            ptr->map = data;
            for (f = 0; f < files; f++) {
                if (ptr->flags[f] & 2) {
                    int i;
                    for (i = 0; i < 4; i++) {
                        ptr->map_idx[f][i] = (data + 1 - ptr->map);
                        data += 1 + data[0];
                    }
                }
            }
            data += ((uintptr_t)data) & 0x01;

            for (f = 0; f < files; f++) {
                ptr->file[f].precomp->indextable = (char*)data;
                data += size[3 * f];
            }

            for (f = 0; f < files; f++) {
                ptr->file[f].precomp->sizetable = (ushort*)data;
                data += size[3 * f + 1];
            }

            for (f = 0; f < files; f++) {
                data = (ubyte*)((((uintptr_t)data) + 0x3f) & ~0x3f);
                ptr->file[f].precomp->data = data;
                data += size[3 * f + 2];
            }
        }

        return 1;
    }

    void load_dtz_table(char* str, uint64 key1, uint64 key2)
    {
        int i;
        struct TBEntry* ptr, * ptr3;
        struct TBHashEntry* ptr2;

        DTZ_table[0].key1 = key1;
        DTZ_table[0].key2 = key2;
        DTZ_table[0].entry = NULL;

        // find corresponding WDL entry
        ptr2 = TB_hash[key1 >> (64 - TBHASHBITS)];
        for (i = 0; i < HSHMAX; i++)
            if (ptr2[i].key == key1) break;
        if (i == HSHMAX) return;
        ptr = ptr2[i].ptr;

        ptr3 = (struct TBEntry*)malloc(ptr->has_pawns
            ? sizeof(struct DTZEntry_pawn)
            : sizeof(struct DTZEntry_piece));

        ptr3->data = Map_TB(str, DTZSUFFIX, &ptr3->mapping);
        ptr3->key = ptr->key;
        ptr3->num = ptr->num;
        ptr3->symmetric = ptr->symmetric;
        ptr3->has_pawns = ptr->has_pawns;
        if (ptr3->has_pawns) {
            struct DTZEntry_pawn* entry = (struct DTZEntry_pawn*)ptr3;
            entry->pawns[0] = ((struct TBEntry_pawn*)ptr)->pawns[0];
            entry->pawns[1] = ((struct TBEntry_pawn*)ptr)->pawns[1];
        } else {
            struct DTZEntry_piece* entry = (struct DTZEntry_piece*)ptr3;
            entry->enc_type = ((struct TBEntry_piece*)ptr)->enc_type;
        }
        if (!Load_DTZ(ptr3))
            free(ptr3);
        else
            DTZ_table[0].entry = ptr3;
    }

    static void free_wdl_entry(struct TBEntry* entry)
    {
        Unmap_TB(entry->data, entry->mapping);
        if (!entry->has_pawns) {
            struct TBEntry_piece* ptr = (struct TBEntry_piece*)entry;
            free(ptr->precomp[0]);
            if (ptr->precomp[1])
                free(ptr->precomp[1]);
        } else {
            struct TBEntry_pawn* ptr = (struct TBEntry_pawn*)entry;
            int f;
            for (f = 0; f < 4; f++) {
                free(ptr->file[f].precomp[0]);
                if (ptr->file[f].precomp[1])
                    free(ptr->file[f].precomp[1]);
            }
        }
    }

    static void free_dtz_entry(struct TBEntry* entry)
    {
        Unmap_TB(entry->data, entry->mapping);
        if (!entry->has_pawns) {
            struct DTZEntry_piece* ptr = (struct DTZEntry_piece*)entry;
            free(ptr->precomp);
        } else {
            struct DTZEntry_pawn* ptr = (struct DTZEntry_pawn*)entry;
            int f;
            for (f = 0; f < 4; f++)
                free(ptr->file[f].precomp);
        }
        free(entry);
    }

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

        // Check if file exists
        FD fd = Open_TB(tablebase.c_str(), WDLSUFFIX);
        if (fd == FD_ERR) return;
        Close_TB(fd);

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

    static int wdl_to_map[5] = { 1, 3, 0, 2, 0 };
    static ubyte pa_flags[5] = { 8, 0, 0, 0, 4 };

    static uint64 encode_piece(struct TBEntry_piece* ptr, ubyte* norm, int* pos, int* factor) {
        uint64 idx;
        int i, j, k, m, l, p;
        int n = ptr->num;

        if (pos[0] & 0x04) {
            for (i = 0; i < n; i++)
                pos[i] ^= 0x07;
        }
        if (pos[0] & 0x20) {
            for (i = 0; i < n; i++)
                pos[i] ^= 0x38;
        }

        for (i = 0; i < n; i++)
            if (offdiag[pos[i]]) break;
        if (i < (ptr->enc_type == 0 ? 3 : 2) && offdiag[pos[i]] > 0)
            for (i = 0; i < n; i++)
                pos[i] = flipdiag[pos[i]];

        switch (ptr->enc_type) {

        case 0: /* 111 */
            i = (pos[1] > pos[0]);
            j = (pos[2] > pos[0]) + (pos[2] > pos[1]);

            if (offdiag[pos[0]])
                idx = triangle[pos[0]] * 63 * 62 + (pos[1] - i) * 62 + (pos[2] - j);
            else if (offdiag[pos[1]])
                idx = 6 * 63 * 62 + diag[pos[0]] * 28 * 62 + lower[pos[1]] * 62 + pos[2] - j;
            else if (offdiag[pos[2]])
                idx = 6 * 63 * 62 + 4 * 28 * 62 + (diag[pos[0]]) * 7 * 28 + (diag[pos[1]] - i) * 28 + lower[pos[2]];
            else
                idx = 6 * 63 * 62 + 4 * 28 * 62 + 4 * 7 * 28 + (diag[pos[0]] * 7 * 6) + (diag[pos[1]] - i) * 6 + (diag[pos[2]] - j);
            i = 3;
            break;

        default: /* K2 */
            idx = KK_idx[triangle[pos[0]]][pos[1]];
            i = 2;
            break;
        }
        idx *= factor[0];

        for (; i < n;) {
            int t = norm[i];
            for (j = i; j < i + t; j++)
                for (k = j + 1; k < i + t; k++)
                    if (pos[j] > pos[k]) Swap(pos[j], pos[k]);
            int s = 0;
            for (m = i; m < i + t; m++) {
                p = pos[m];
                for (l = 0, j = 0; l < i; l++)
                    j += (p > pos[l]);
                s += binomial[m - i][p - j];
            }
            idx += ((uint64)s) * ((uint64)factor[i]);
            i += t;
        }

        return idx;
    }

    // determine file of leftmost pawn and sort pawns
    static int pawn_file(struct TBEntry_pawn* ptr, int* pos) {
        int i;

        for (i = 1; i < ptr->pawns[0]; i++)
            if (flap[pos[0]] > flap[pos[i]])
                Swap(pos[0], pos[i]);

        return file_to_file[pos[0] & 0x07];
    }

    static uint64 encode_pawn(struct TBEntry_pawn* ptr, ubyte* norm, int* pos, int* factor) {
        uint64 idx;
        int i, j, k, m, s, t;
        int n = ptr->num;

        if (pos[0] & 0x04)
            for (i = 0; i < n; i++)
                pos[i] ^= 0x07;

        for (i = 1; i < ptr->pawns[0]; i++)
            for (j = i + 1; j < ptr->pawns[0]; j++)
                if (ptwist[pos[i]] < ptwist[pos[j]])
                    Swap(pos[i], pos[j]);

        t = ptr->pawns[0] - 1;
        idx = pawnidx[t][flap[pos[0]]];
        for (i = t; i > 0; i--)
            idx += binomial[t - i][ptwist[pos[i]]];
        idx *= factor[0];

        // remaining pawns
        i = ptr->pawns[0];
        t = i + ptr->pawns[1];
        if (t > i) {
            for (j = i; j < t; j++)
                for (k = j + 1; k < t; k++)
                    if (pos[j] > pos[k]) Swap(pos[j], pos[k]);
            s = 0;
            for (m = i; m < t; m++) {
                int p = pos[m];
                for (k = 0, j = 0; k < i; k++)
                    j += (p > pos[k]);
                s += binomial[m - i][p - j - 8];
            }
            idx += ((uint64)s) * ((uint64)factor[i]);
            i = t;
        }

        for (; i < n;) {
            t = norm[i];
            for (j = i; j < i + t; j++)
                for (k = j + 1; k < i + t; k++)
                    if (pos[j] > pos[k]) Swap(pos[j], pos[k]);
            s = 0;
            for (m = i; m < i + t; m++) {
                int p = pos[m];
                for (k = 0, j = 0; k < i; k++)
                    j += (p > pos[k]);
                s += binomial[m - i][p - j];
            }
            idx += ((uint64)s) * ((uint64)factor[i]);
            i += t;
        }

        return idx;
    }

    static ubyte decompress_pairs(struct PairsData* d, uint64 idx) {
        if (!d->idxbits)
            return d->min_len;

        uint32 mainidx = idx >> d->idxbits;
        int litidx = (idx & ((1 << d->idxbits) - 1)) - (1 << (d->idxbits - 1));
        uint32 block = *(uint32*)(d->indextable + 6 * mainidx);
        litidx += *(ushort*)(d->indextable + 6 * mainidx + 4);
        if (litidx < 0) {
            do {
                litidx += d->sizetable[--block] + 1;
            } while (litidx < 0);
        } else {
            while (litidx > d->sizetable[block])
                litidx -= d->sizetable[block++] + 1;
        }

        uint32* ptr = (uint32*)(d->data + (block << d->blocksize));

        int m = d->min_len;
        ushort* offset = d->offset;
        uint32* base = d->base - m;
        ubyte* symlen = d->symlen;
        int sym, bitcnt;

        uint32 next = 0;
        uint32 code = _byteswap_ulong(*ptr++);
        bitcnt = 0; // number of bits in next
        for (;;) {
            int l = m;
            while (code < base[l]) l++;
            sym = offset[l] + ((code - base[l]) >> (32 - l));
            if (litidx < (int)symlen[sym] + 1) break;
            litidx -= (int)symlen[sym] + 1;
            code <<= l;
            if (bitcnt < l) {
                if (bitcnt) {
                    code |= (next >> (32 - l));
                    l -= bitcnt;
                }
                next = _byteswap_ulong(*ptr++);
                bitcnt = 32;
            }
            code |= (next >> (32 - l));
            next <<= l;
            bitcnt -= l;
        }

        ubyte* sympat = d->sympat;
        while (symlen[sym] != 0) {
            int w = *(int*)(sympat + 3 * sym);
            int s1 = w & 0x0fff;
            if (litidx < (int)symlen[s1] + 1)
                sym = s1;
            else {
                litidx -= (int)symlen[s1] + 1;
                sym = (w >> 12) & 0x0fff;
            }
        }

        return *(sympat + 3 * sym);
    }

    static int Probe_WDL_Table(const Position& board, int* success) {
        *success = 1;

        TBEntry* ptr;
        TBHashEntry* ptr2;
        uint64 idx;
        int i;
        ubyte res;
        int p[TBPIECES];

        uint64 key = Material_Key(board);

        // Check if we have this particular tablebase
        ptr2 = TB_hash[key >> (64 - TBHASHBITS)];
        for (i = 0; i < HSHMAX; i++)
            if (ptr2[i].key == key) break;
        if (i == HSHMAX) {
            *success = 0;
            return 0;
        }

        // Load the tablebase if we haven't already
        ptr = ptr2[i].ptr;
        if (!ptr->ready) {
            char str[16];
            TB_Str(board, str, ptr->key != key);
            if (!Load_Wdl(ptr, str)) {
                ptr2[i].key = 0ULL;
                *success = 0;
                return 0;
            }
            ptr->ready = 1;
        }

        int bside, mirror, cmirror;
        if (!ptr->symmetric) {
            if (key != ptr->key) {
                cmirror = 8;
                mirror = 0x38;
                bside = (board.m_WhiteMove);
            } else {
                cmirror = mirror = 0;
                bside = !(board.m_WhiteMove);
            }
        } else {
            cmirror = board.m_WhiteMove ? 0 : 8;
            mirror = board.m_WhiteMove ? 0 : 0x38;
            bside = 0;
        }

        // p[i] is to contain the square 0-63 (A1-H8) for a piece of type
        // pc[i] ^ cmirror, where 1 = white pawn, ..., 14 = black king.
        // Pieces of the same type are guaranteed to be consecutive.
        if (!ptr->has_pawns) {
            TBEntry_piece* entry = (TBEntry_piece*)ptr;
            ubyte* pc = entry->pieces[bside];
            for (i = 0; i < entry->num;) {
                BitBoard bb = board.m_Pieces[pc[i] & 0x07][((pc[i] ^ cmirror) >> 3) & 0x1];
                do {
                    p[i++] = PopPos(bb);
                } while (bb);
            }
            idx = encode_piece(entry, entry->norm[bside], p, entry->factor[bside]);
            res = decompress_pairs(entry->precomp[bside], idx);
        } else {
            TBEntry_pawn* entry = (TBEntry_pawn*)ptr;
            int k = entry->file[0].pieces[0][0] ^ cmirror;
            BitBoard bb = board.m_Pieces[k & 0x07][(k >> 3) & 0x1];
            i = 0;
            do {
                p[i++] = PopPos(bb) ^ mirror;
            } while (bb);
            int f = pawn_file(entry, p);
            ubyte* pc = entry->file[f].pieces[bside];
            for (; i < entry->num;) {
                bb = board.m_Pieces[pc[i] & 0x07][((pc[i] ^ cmirror) >> 3) & 0x1];
                do {
                    p[i++] = PopPos(bb) ^ mirror;
                } while (bb);
            }
            idx = encode_pawn(entry, entry->file[f].norm[bside], p, entry->file[f].factor[bside]);
            res = decompress_pairs(entry->file[f].precomp[bside], idx);
        }

        return ((int)res) - 2;
    }

    // Essentially do a quiesence search until a silent position is reached which will then be probed.
    static int Probe_Q(Position& board, int alpha, int beta, int* success) {
        int v;

        int man = COUNT_BIT(board.m_Board);

        if (man == 2) { // Two kings is automatic draw
            return 0;
        }

        MoveGen gen(board, 0, true); // Generates all captures, or evasions if in check
        Move move;
        while ((move = gen.Next()) != 0) {
            board.MovePiece(move);
            v = -Probe_Q(board, -beta, -alpha, success);
            board.UndoMove(move);
            if (*success == 0) return 0;
            if (v > alpha) {
                if (v >= beta)
                    return v;
                alpha = v;
            }
        }

        v = Probe_WDL_Table(board, success);

        return alpha >= v ? alpha : v;
    }

    // -2: loss
    // -1: loss but 50-move rule draw
    // 0: draw
    // 1: win but 50-move rule draw
    // 2: win
    int TableBase::Probe_WDL(Position& board, int* success) {

        *success = 1;

        int man = COUNT_BIT(board.m_Board);
        if (man > TBPIECES) {
            *success = 0;
            return 0;
        }

        if (man == 2) { // Two kings is automatic draw
            return 0;
        }

        int best_cap = -3, best_ep = -3;

        MoveGen gen(board, 0, true); // Generates all captures, or evasions if in check
        Move move;
        while ((move = gen.Next()) != 0) {
            board.MovePiece(move);
            int v = -Probe_Q(board, -2, -best_cap, success);
            board.UndoMove(move);
            if (*success == 0) return 0;
            if (v > best_cap) {
                if (v == 2) {
                    *success = 2;
                    return 2;
                }
                if (!EnPassant(move))
                    best_cap = v;
                else if (v > best_ep)
                    best_ep = v;
            }
        }

        int v = Probe_WDL_Table(board, success);
        if (*success == 0) return 0;

        if (best_ep > best_cap) {
            if (best_ep > v) {
                *success = 2;
                return best_ep;
            }
            best_cap = best_ep;
        }

        if (best_cap >= v) {
            *success = 1 + (best_cap > 0);
            return best_cap;
        }

        return v;
    }

    static int wdl_to_dtz[] = {
        -1, -101, 0, 101, 1
    };

    int Probe_DTZ_Table(Position& board, int wdl, int* success) {
        *success = 1;

        struct TBEntry* ptr;
        uint64 idx;
        int i, res;
        int p[TBPIECES];

        // Obtain the position's material signature key.
        uint64 key = Material_Key(board);

        if (DTZ_table[0].key1 != key && DTZ_table[0].key2 != key) {
            for (i = 1; i < DTZ_ENTRIES; i++)
                if (DTZ_table[i].key1 == key || DTZ_table[i].key2 == key) break;
            if (i < DTZ_ENTRIES) {
                struct DTZTableEntry table_entry = DTZ_table[i];
                for (; i > 0; i--)
                    DTZ_table[i] = DTZ_table[i - 1];
                DTZ_table[0] = table_entry;
            } else {
                struct TBHashEntry* ptr2 = TB_hash[key >> (64 - TBHASHBITS)];
                for (i = 0; i < HSHMAX; i++)
                    if (ptr2[i].key == key) break;
                if (i == HSHMAX) {
                    *success = 0;
                    return 0;
                }
                ptr = ptr2[i].ptr;
                char str[16];
                int mirror = (ptr->key != key);
                TB_Str(board, str, mirror);
                if (DTZ_table[DTZ_ENTRIES - 1].entry)
                    free_dtz_entry(DTZ_table[DTZ_ENTRIES - 1].entry);
                for (i = DTZ_ENTRIES - 1; i > 0; i--)
                    DTZ_table[i] = DTZ_table[i - 1];
                load_dtz_table(str, Calc_Key(board, mirror), Calc_Key(board, !mirror));
            }
        }

        ptr = DTZ_table[0].entry;
        if (!ptr) {
            *success = 0;
            return 0;
        }

        int bside, mirror, cmirror;
        if (!ptr->symmetric) {
            if (key != ptr->key) {
                cmirror = 8;
                mirror = 0x38;
                bside = (board.m_WhiteMove);
            } else {
                cmirror = mirror = 0;
                bside = !(board.m_WhiteMove);
            }
        } else {
            cmirror = board.m_WhiteMove ? 0 : 8;
            mirror = board.m_WhiteMove ? 0 : 0x38;
            bside = 0;
        }

        if (!ptr->has_pawns) {
            DTZEntry_piece* entry = (DTZEntry_piece*)ptr;
            if ((entry->flags & 1) != bside && !entry->symmetric) {
                *success = -1;
                return 0;
            }
            ubyte* pc = entry->pieces;
            for (i = 0; i < entry->num;) {
                BitBoard bb = board.m_Pieces[(pc[i] & 0x07)][((pc[i] ^ cmirror) >> 3) & 0x1];
                do {
                    p[i++] = PopPos(bb);
                } while (bb);
            }
            idx = encode_piece((TBEntry_piece*)entry, entry->norm, p, entry->factor);
            res = decompress_pairs(entry->precomp, idx);

            if (entry->flags & 2)
                res = entry->map[entry->map_idx[wdl_to_map[wdl + 2]] + res];

            if (!(entry->flags & pa_flags[wdl + 2]) || (wdl & 1))
                res *= 2;
        } else {
            DTZEntry_pawn* entry = (DTZEntry_pawn*)ptr;
            int k = entry->file[0].pieces[0] ^ cmirror;
            BitBoard bb = board.m_Pieces[k & 0x07][(k >> 3) & 0x1];
            i = 0;
            do {
                p[i++] = PopPos(bb) ^ mirror;
            } while (bb);
            int f = pawn_file((TBEntry_pawn*)entry, p);
            if ((entry->flags[f] & 1) != bside) {
                *success = -1;
                return 0;
            }
            ubyte* pc = entry->file[f].pieces;
            for (; i < entry->num;) {
                bb = board.m_Pieces[pc[i] & 0x07][((pc[i] ^ cmirror) >> 3) & 0x1];
                do {
                    p[i++] = PopPos(bb) ^ mirror;
                } while (bb);
            }
            idx = encode_pawn((TBEntry_pawn*)entry, entry->file[f].norm, p, entry->file[f].factor);
            res = decompress_pairs(entry->file[f].precomp, idx);

            if (entry->flags[f] & 2)
                res = entry->map[entry->map_idx[f][wdl_to_map[wdl + 2]] + res];

            if (!(entry->flags[f] & pa_flags[wdl + 2]) || (wdl & 1))
                res *= 2;
        }

        return res;
    }

    int TableBase::Probe_DTZ(Position& board, int* success) {
        // Probe WDL first
        int wdl = Probe_WDL(board, success);
        if (*success == 0) return 0;

        if (wdl == 0) return 0;

        // Winning capture
        if (*success == 2)
            return wdl_to_dtz[wdl + 2];

        // Check for winning pawn move
        if (wdl > 0) {
            MoveGen gen(board, 0, false); // Generates all legal moves
            Move move;
            while ((move = gen.Next()) != 0) {
                if (!(MovePieceType(move) == WPAWN || MovePieceType(move) == WPAWN) || (CaptureType(move) > 0)) continue;
                board.MovePiece(move);
                int v = -Probe_WDL(board, success);
                board.UndoMove(move);
                if (*success == 0) return 0;
                if (v == wdl)
                    return wdl_to_dtz[wdl + 2];
            }
        }

        int dtz = Probe_DTZ_Table(board, wdl, success);
        if (*success >= 0)
            return wdl_to_dtz[wdl + 2] + ((wdl > 0) ? dtz : -dtz);

        // if success < 0 then we need to probe DTZ for the other side to move.

        int best = wdl > 0 ? INT16_MAX : wdl_to_dtz[wdl + 2];
        MoveGen gen(board, 0, false);
        Move move;
        while ((move = gen.Next()) != 0) {
            // Skip pawn moves and captures
            if (MovePieceType(move) == WPAWN || MovePieceType(move) == WPAWN || (CaptureType(move) > 0)) continue;
            board.MovePiece(move);
            int v = -Probe_DTZ(board, success);
            board.UndoMove(move);
            if (*success == 0) return 0;
            if (wdl > 0) {
                if (v > 0 && v + 1 < best)
                    best = v + 1;
            } else {
                if (v - 1 < best)
                    best = v - 1;
            }
        }
        return best;
    }

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