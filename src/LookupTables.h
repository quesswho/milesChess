#pragma once

using llu = unsigned long long;

namespace Lookup {
    constexpr llu InitFile(int p) {
        return (0x0101010101010101ull << (p % 8));
    }

    constexpr llu InitRank(int p) {
        return (0xFFull << ((p >> 3) << 3)); // Truncate the first 3 bits
    }

    constexpr llu InitDiagonal(int p) {
        int s = 8 * (p % 8) - ((p >> 3) << 3);
        return s > 0 ? 0x8040201008040201 >> (s) : 0x8040201008040201 << (-s);
    }

    constexpr llu InitAntiDiagonal(int p) {
        int s = 56 - 8 * (p % 8) - ((p >> 3) << 3);
        return s > 0 ? 0x0102040810204080 >> (s) : 0x0102040810204080 << (-s);
    }

    constexpr int CoordToPos(int x, int y) {
        return (x <= 0 || y <= 0 || x > 8 || y > 8) ? -1 : (y - 1) * 8 + (8 - x);

    }

    constexpr llu AddToMap(llu map, int x, int y) {
        int pos = CoordToPos(x,y);
        return (pos == -1) ? map : map | (1ull << pos);
    }

    // Table generator for lines;
    static std::array<llu, 64 * 4> InitSlider() {
        std::array<llu, 64 * 4> result{};
        for (int i = 0; i < 64; i++) {
            result[i * 4] = InitFile(i);
            result[i * 4 + 1] = InitRank(i);
            result[i * 4 + 2] = InitDiagonal(i);
            result[i * 4 + 3] = InitAntiDiagonal(i);
        }
        return result;
    }

    // Table generator for knight_attacks
    static std::array<llu, 64> InitKnight() {
        std::array<llu, 64> result{};
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

    static void PrintLineTable() {
        int i = 0;
        for (llu e : Lookup::InitSlider()) {
            printf("%#018llx, ", e);
            i++;
            if (i % 4 == 0) printf("\n");
        }
    }

    static void PrintKnightTable() {
        int i = 0;
        for (llu e : Lookup::InitKnight()) {
            printf("%#018llx, ", e);
            i++;
            if (i % 4 == 0) printf("\n");
        }
    }

    static std::array<llu, 64 * 4> lines = {
        0x0101010101010101, 0x00000000000000ff, 0x8040201008040201, 0x0000000000000001,
        0x0202020202020202, 0x00000000000000ff, 0x0080402010080402, 0x0000000000000102,
        0x0404040404040404, 0x00000000000000ff, 0x0000804020100804, 0x0000000000010204,
        0x0808080808080808, 0x00000000000000ff, 0x0000008040201008, 0x0000000001020408,
        0x1010101010101010, 0x00000000000000ff, 0x0000000080402010, 0x0000000102040810,
        0x2020202020202020, 0x00000000000000ff, 0x0000000000804020, 0x0000010204081020,
        0x4040404040404040, 0x00000000000000ff, 0x0000000000008040, 0x0001020408102040,
        0x8080808080808080, 0x00000000000000ff, 0x0000000000000080, 0x0102040810204080,
        0x0101010101010101, 0x000000000000ff00, 0x4020100804020100, 0x0000000000000102,
        0x0202020202020202, 0x000000000000ff00, 0x8040201008040201, 0x0000000000010204,
        0x0404040404040404, 0x000000000000ff00, 0x0080402010080402, 0x0000000001020408,
        0x0808080808080808, 0x000000000000ff00, 0x0000804020100804, 0x0000000102040810,
        0x1010101010101010, 0x000000000000ff00, 0x0000008040201008, 0x0000010204081020,
        0x2020202020202020, 0x000000000000ff00, 0x0000000080402010, 0x0001020408102040,
        0x4040404040404040, 0x000000000000ff00, 0x0000000000804020, 0x0102040810204080,
        0x8080808080808080, 0x000000000000ff00, 0x0000000000008040, 0x0204081020408000,
        0x0101010101010101, 0x0000000000ff0000, 0x2010080402010000, 0x0000000000010204,
        0x0202020202020202, 0x0000000000ff0000, 0x4020100804020100, 0x0000000001020408,
        0x0404040404040404, 0x0000000000ff0000, 0x8040201008040201, 0x0000000102040810,
        0x0808080808080808, 0x0000000000ff0000, 0x0080402010080402, 0x0000010204081020,
        0x1010101010101010, 0x0000000000ff0000, 0x0000804020100804, 0x0001020408102040,
        0x2020202020202020, 0x0000000000ff0000, 0x0000008040201008, 0x0102040810204080,
        0x4040404040404040, 0x0000000000ff0000, 0x0000000080402010, 0x0204081020408000,
        0x8080808080808080, 0x0000000000ff0000, 0x0000000000804020, 0x0408102040800000,
        0x0101010101010101, 0x00000000ff000000, 0x1008040201000000, 0x0000000001020408,
        0x0202020202020202, 0x00000000ff000000, 0x2010080402010000, 0x0000000102040810,
        0x0404040404040404, 0x00000000ff000000, 0x4020100804020100, 0x0000010204081020,
        0x0808080808080808, 0x00000000ff000000, 0x8040201008040201, 0x0001020408102040,
        0x1010101010101010, 0x00000000ff000000, 0x0080402010080402, 0x0102040810204080,
        0x2020202020202020, 0x00000000ff000000, 0x0000804020100804, 0x0204081020408000,
        0x4040404040404040, 0x00000000ff000000, 0x0000008040201008, 0x0408102040800000,
        0x8080808080808080, 0x00000000ff000000, 0x0000000080402010, 0x0810204080000000,
        0x0101010101010101, 0x000000ff00000000, 0x0804020100000000, 0x0000000102040810,
        0x0202020202020202, 0x000000ff00000000, 0x1008040201000000, 0x0000010204081020,
        0x0404040404040404, 0x000000ff00000000, 0x2010080402010000, 0x0001020408102040,
        0x0808080808080808, 0x000000ff00000000, 0x4020100804020100, 0x0102040810204080,
        0x1010101010101010, 0x000000ff00000000, 0x8040201008040201, 0x0204081020408000,
        0x2020202020202020, 0x000000ff00000000, 0x0080402010080402, 0x0408102040800000,
        0x4040404040404040, 0x000000ff00000000, 0x0000804020100804, 0x0810204080000000,
        0x8080808080808080, 0x000000ff00000000, 0x0000008040201008, 0x1020408000000000,
        0x0101010101010101, 0x0000ff0000000000, 0x0402010000000000, 0x0000010204081020,
        0x0202020202020202, 0x0000ff0000000000, 0x0804020100000000, 0x0001020408102040,
        0x0404040404040404, 0x0000ff0000000000, 0x1008040201000000, 0x0102040810204080,
        0x0808080808080808, 0x0000ff0000000000, 0x2010080402010000, 0x0204081020408000,
        0x1010101010101010, 0x0000ff0000000000, 0x4020100804020100, 0x0408102040800000,
        0x2020202020202020, 0x0000ff0000000000, 0x8040201008040201, 0x0810204080000000,
        0x4040404040404040, 0x0000ff0000000000, 0x0080402010080402, 0x1020408000000000,
        0x8080808080808080, 0x0000ff0000000000, 0x0000804020100804, 0x2040800000000000,
        0x0101010101010101, 0x00ff000000000000, 0x0201000000000000, 0x0001020408102040,
        0x0202020202020202, 0x00ff000000000000, 0x0402010000000000, 0x0102040810204080,
        0x0404040404040404, 0x00ff000000000000, 0x0804020100000000, 0x0204081020408000,
        0x0808080808080808, 0x00ff000000000000, 0x1008040201000000, 0x0408102040800000,
        0x1010101010101010, 0x00ff000000000000, 0x2010080402010000, 0x0810204080000000,
        0x2020202020202020, 0x00ff000000000000, 0x4020100804020100, 0x1020408000000000,
        0x4040404040404040, 0x00ff000000000000, 0x8040201008040201, 0x2040800000000000,
        0x8080808080808080, 0x00ff000000000000, 0x0080402010080402, 0x4080000000000000,
        0x0101010101010101, 0xff00000000000000, 0x0100000000000000, 0x0102040810204080,
        0x0202020202020202, 0xff00000000000000, 0x0201000000000000, 0x0204081020408000,
        0x0404040404040404, 0xff00000000000000, 0x0402010000000000, 0x0408102040800000,
        0x0808080808080808, 0xff00000000000000, 0x0804020100000000, 0x0810204080000000,
        0x1010101010101010, 0xff00000000000000, 0x1008040201000000, 0x1020408000000000,
        0x2020202020202020, 0xff00000000000000, 0x2010080402010000, 0x2040800000000000,
        0x4040404040404040, 0xff00000000000000, 0x4020100804020100, 0x4080000000000000,
        0x8080808080808080, 0xff00000000000000, 0x8040201008040201, 0x8000000000000000
    };

    static std::array<llu, 64> knight_attacks = {
        0x0000000000020400, 0x0000000000050800, 0x00000000000a1100, 0x0000000000142200,
        0x0000000000284400, 0x0000000000508800, 0x0000000000a01000, 0x0000000000402000,
        0x0000000002040004, 0x0000000005080008, 0x000000000a110011, 0x0000000014220022,
        0x0000000028440044, 0x0000000050880088, 0x00000000a0100010, 0x0000000040200020,
        0x0000000204000402, 0x0000000508000805, 0x0000000a1100110a, 0x0000001422002214,
        0x0000002844004428, 0x0000005088008850, 0x000000a0100010a0, 0x0000004020002040,
        0x0000020400040200, 0x0000050800080500, 0x00000a1100110a00, 0x0000142200221400,
        0x0000284400442800, 0x0000508800885000, 0x0000a0100010a000, 0x0000402000204000,
        0x0002040004020000, 0x0005080008050000, 0x000a1100110a0000, 0x0014220022140000,
        0x0028440044280000, 0x0050880088500000, 0x00a0100010a00000, 0x0040200020400000,
        0x0204000402000000, 0x0508000805000000, 0x0a1100110a000000, 0x1422002214000000,
        0x2844004428000000, 0x5088008850000000, 0xa0100010a0000000, 0x4020002040000000,
        0x0400040200000000, 0x0800080500000000, 0x1100110a00000000, 0x2200221400000000,
        0x4400442800000000, 0x8800885000000000, 0x100010a000000000, 0x2000204000000000,
        0x0004020000000000, 0x0008050000000000, 0x00110a0000000000, 0x0022140000000000,
        0x0044280000000000, 0x0088500000000000, 0x0010a00000000000, 0x0020400000000000
    };
}