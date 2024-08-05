#include <string>

using llu = unsigned long long;

#define GET_SQUARE(X) _tzcnt_u64(X)

namespace Lookup {
    static llu InitFile(int p) {
        return (0x0101010101010101ull << (p % 8));
    }

    static llu InitRank(int p) {
        return (0xFFull << ((p>>3) << 3)); // Truncate the first 3 bits
    }

    static llu InitDiagonal(int p) {
        int s = 8*(p % 8) - ((p >> 3) << 3);
        return s > 0 ? 0x8040201008040201 >> (s) : 0x8040201008040201 << (-s);
    }

    static llu InitAntiDiagonal(int p) {
        int s = 56 - 8 * (p % 8) - ((p >> 3) << 3);
        return s > 0 ? 0x0102040810204080 >> (s) : 0x0102040810204080 << (-s);
    }

    static int CoordToPos(int x, int y) {
        if (x <= 0 || y <= 0 || x >= 8 || y >= 8) return -1;
        return (y - 1) * 8 + (8 - x);

    }

    static llu AddToMap(llu map, int x, int y) {
        int pos = CoordToPos(x, y);
        if (pos == -1) return map;
        else return map | (1ull << pos);
    }

    static llu lines[64 * 4] = {}; // Lookup table for all slider lines
    static llu knight_attacks[64] = {}; // Lookup table for all knight attack 

    static void Init() {
        for (int i = 0; i < 64; i++) {
            lines[i * 4] = InitFile(i);
            lines[i * 4 + 1] = InitRank(i);
            lines[i * 4 + 2] = InitDiagonal(i);
            lines[i * 4 + 3] = InitAntiDiagonal(i);
        }

        for (int x = 1; x <= 8; x++) {
            for (int y = 1; y <= 8; y++) {
                llu N_Attack = 0;
                N_Attack = AddToMap(N_Attack, x + 2, y + 1);
                N_Attack = AddToMap(N_Attack, x - 2, y + 1);
                N_Attack = AddToMap(N_Attack, x + 2, y - 1);
                N_Attack = AddToMap(N_Attack, x - 2, y - 1);
                N_Attack = AddToMap(N_Attack, x + 1, y + 2);
                N_Attack = AddToMap(N_Attack, x - 1, y + 2);
                N_Attack = AddToMap(N_Attack, x + 1, y - 2);
                N_Attack = AddToMap(N_Attack, x - 1, y - 2);
                knight_attacks[CoordToPos(x, y)] = N_Attack;
            }
        }
    }

    
}

using uint64 = unsigned long long;

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

struct BoardInfo {
    bool m_WhiteMove;
    uint64 m_EnPassant;

    bool m_WhiteCastleKing;
    bool m_WhiteCastleQueen;

    bool m_BlackCastleKing;
    bool m_BlackCastleQueen;

    uint64 m_HalfMoves;
    uint64 m_FullMoves;
};

static BoardInfo FenInfo(const std::string& FEN) {
    BoardInfo info = {};
    int i = 0;
    while (FEN[i++] != ' ') {
        // Skip piece placement
    }

    char ac = FEN[i++]; // Active color
    switch (ac) {
    case 'w':
        info.m_WhiteMove = true;
        break;
    case 'b':
        info.m_WhiteMove = false;
    default:
        printf("Invalid FEN at active color");
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

    char a = FEN[i++];
    
    if(a != '-') {
        if (info.m_WhiteMove) {
            info.m_EnPassant = 1ull << (32 + ('h' - a)); // TODO: need to be tested
        }
        else {
            info.m_EnPassant = 1ull << (24 + ('h' - a)); // TODO: need to be tested
        }
    }
    else {
        info.m_EnPassant = 0;
    }

    info.m_HalfMoves = 0;
    info.m_FullMoves = 0;
    int t = 1;

    while ((c = FEN[i++]) != ' ');
    while ((c = FEN[i++]) != ' ') info.m_HalfMoves = info.m_HalfMoves * 10 + (int)(c - '0');
    while (i < FEN.size()) {
        c = FEN[i++];
        info.m_FullMoves = info.m_FullMoves * 10 + (c - '0');
    }
    return info;
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

struct Move {
    uint64 m_From;
    uint64 m_To;
};

/*
    Boards start from bottom right and go right to left

    64 63 ... 57 56
    .
    .
    .
    8 7 ... 3 2 1

*/

class Board {
public: // TODO: make bitboard private and use constructors and move functions for changing them
    uint64 m_WhitePawn;
    uint64 m_WhiteKnight;
    uint64 m_WhiteBishop;
    uint64 m_WhiteRook;
    uint64 m_WhiteQueen;
    uint64 m_WhiteKing;

    uint64 m_BlackPawn;
    uint64 m_BlackKnight;
    uint64 m_BlackBishop;
    uint64 m_BlackRook;
    uint64 m_BlackQueen;
    uint64 m_BlackKing;

    uint64 m_White;
    uint64 m_Black;

    uint64 m_Board;

    BoardInfo m_BoardInfo;

    Board(const std::string& FEN);

    uint64 Slide(uint64 pos, uint64 line, uint64 blocked);

    uint64 Rook(int pos, uint64 occ);
    uint64 Bishop(int pos, uint64 occ);
    uint64 Queen(int pos, uint64 occ);

    bool Validate();

    void MoveWhitePawn(uint64 from, uint64 to);
    void MoveBlackPawn(uint64 from, uint64 to);
};

static std::string BoardtoFen(const Board& board) {
    std::string FEN;
    FEN.reserve(27); // Shortest FEN could be: k7/8/8/8/8/8/8/7K w - - 0 1
    // Board

    int skip = 0; // Count number of empty positions in each rank
    for (int i = 63; i >= 0; i--) {
        uint64 pos = 1ull << i;

        if (i % 8 == 7) {
            if (skip > 0) {
                FEN += (char)('0' + skip);
                skip = 0;
            }
            if(i != 63) FEN += '/';
        }

        if (!(board.m_Board & pos)) {
            skip++;
            continue;
        }

        if ((board.m_BlackPawn & pos) > 0) {
            FEN += "p";
            continue;
        }
        if ((board.m_BlackKnight & pos) > 0) {
            FEN += "n";
            continue;
        }
        if ((board.m_BlackBishop & pos) > 0) {
            FEN += "b";
            continue;
        }
        if ((board.m_BlackRook & pos) > 0) {
            FEN += "r";
            continue;
        }
        if ((board.m_BlackQueen & pos) > 0) {
            FEN += "q";
            continue;
        }
        if ((board.m_BlackKing & pos) > 0) {
            FEN += "k";
            continue;
        }
        if ((board.m_WhitePawn & pos) > 0) {
            FEN += "P";
            continue;
        }
        if ((board.m_WhiteKnight & pos) > 0) {
            FEN += "N";
            continue;
        }
        if ((board.m_WhiteBishop & pos) > 0) {
            FEN += "B";
            continue;
        }
        if ((board.m_WhiteRook & pos) > 0) {
            FEN += "R";
            continue;
        }
        if ((board.m_WhiteQueen & pos) > 0) {
            FEN += "Q";
            continue;
        }
        if ((board.m_WhiteKing & pos) > 0) {
            FEN += "K";
            continue;
        }
    }

    if (board.m_BoardInfo.m_WhiteMove) {
        FEN += " w ";
    } else {
        FEN += " b ";
    }

    // Castle rights
    if (board.m_BoardInfo.m_WhiteCastleKing) {
        FEN += "K";
    }
    if (board.m_BoardInfo.m_WhiteCastleQueen) {
        FEN += "Q";
    }
    if (board.m_BoardInfo.m_BlackCastleKing) {
        FEN += "k";
    }
    if (board.m_BoardInfo.m_BlackCastleQueen) {
        FEN += "q";
    }
    if (!(board.m_BoardInfo.m_WhiteCastleKing || board.m_BoardInfo.m_WhiteCastleQueen || board.m_BoardInfo.m_BlackCastleKing || board.m_BoardInfo.m_BlackCastleQueen)) {
        FEN += '-';
    }

    FEN += ' ';
    if (board.m_BoardInfo.m_EnPassant > 0) {
        char pos = 'h' - ((int)(log2(board.m_BoardInfo.m_EnPassant)) % 8);
        if (board.m_BoardInfo.m_WhiteMove) {
            FEN += pos + std::to_string(6);
        }
        else {
            FEN += pos + std::to_string(3);
        }
    } else {
        FEN += '-';
    }

    FEN += ' ' + std::to_string(board.m_BoardInfo.m_HalfMoves);
    FEN += ' ' + std::to_string(board.m_BoardInfo.m_FullMoves);

    return FEN;
}
