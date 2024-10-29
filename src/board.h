#pragma once

#include <string>
#include <array>
#include <vector>


#include "LookupTables.h"

static BoardInfo FenInfo(const std::string& FEN) {
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
    
    if(a != '-') {
        if (info.m_WhiteMove) {
            info.m_EnPassant = 1ull << (40 + ('h' - a)); // TODO: need to be tested
        }
        else {
            info.m_EnPassant = 1ull << (16 + ('h' - a)); // TODO: need to be tested
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
    while (i < FEN.size() && FEN[i] >= '0' && FEN[i] <= '9') {
        c = FEN[i++];
        info.m_FullMoves = info.m_FullMoves * 10 + (c - '0');
    }
    return info;
}

enum PieceType : int {
    NONE = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6
};

enum class MoveType{ // Ordering is based on capture value
    NONE, PAWN, PAWN2, EPASSANT, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    KCASTLE, QCASTLE, P_KNIGHT, P_BISHOP, P_ROOK, P_QUEEN
};



struct Move {

    Move()
        : m_From(0), m_To(0), m_Type(MoveType::NONE), m_Capture(MoveType::NONE)
    {}

    Move(uint64 from, uint64 to, MoveType movetype)
        : m_From(from), m_To(to), m_Type(movetype), m_Capture(MoveType::NONE)
    {}

    Move(uint64 from, uint64 to, MoveType movetype, MoveType capture)
        : m_From(from), m_To(to), m_Type(movetype), m_Capture(capture)
    {}

    uint64 m_From;
    uint64 m_To;
    MoveType m_Type;
    MoveType m_Capture;

    bool operator==(const Move& other) const {
        return m_From == other.m_From && m_To == other.m_To && m_Type == other.m_Type; // Capture should not be necessary
    }

    inline std::string toString() const {
        std::string result = "";
        int f = GET_SQUARE(m_From);
        result += 'h' - (f % 8);
        result += '1' + (f / 8);
        int t = GET_SQUARE(m_To);
        result += 'h' - (t % 8);
        result += '1' + (t / 8);
        if (m_Type == MoveType::P_BISHOP) {
            result += 'N';
        } else if (m_Type == MoveType::P_KNIGHT) {
            result += "N";
        } else if (m_Type == MoveType::P_ROOK) {
            result += "R";
        } else if (m_Type == MoveType::P_QUEEN) {
            result += "Q";
        }
        return result;
    }
};


class Board {
public:
    
    union {
        struct {
            BitBoard m_White;
            BitBoard m_Black;

            const BitBoard m_WhitePawn;
            const BitBoard m_BlackPawn;

            const BitBoard m_WhiteKnight;
            const BitBoard m_BlackKnight;

            const BitBoard m_WhiteBishop;
            const BitBoard m_BlackBishop;

            const BitBoard m_WhiteRook;
            const BitBoard m_BlackRook;

            const BitBoard m_WhiteQueen;
            const BitBoard m_BlackQueen;

            const BitBoard m_WhiteKing;
            const BitBoard m_BlackKing;
        };
        const BitBoard m_Pieces[7][2]; // 6 pieces for each color: 0 for white and 1 for black
        
    };
    BitBoard m_Board;

    Board(const std::string& FEN);

    Board(BitBoard wp, BitBoard wkn, BitBoard wb, BitBoard wr, BitBoard wq, BitBoard wk, BitBoard bp, BitBoard bkn, BitBoard bb, BitBoard br, BitBoard bq, BitBoard bk)
        : m_WhitePawn(wp), m_WhiteKnight(wkn), m_WhiteBishop(wb), m_WhiteRook(wr), m_WhiteQueen(wq), m_WhiteKing(wk), 
          m_BlackPawn(bp), m_BlackKnight(bkn), m_BlackBishop(bb), m_BlackRook(br), m_BlackQueen(bq), m_BlackKing(bk),
          m_White(wp | wkn | wb | wr | wq | wk), m_Black(bp | bkn | bb | br | bq | bk), m_Board(m_White | m_Black)
    {}

    Board(const Board& other)
        : m_WhitePawn(other.m_WhitePawn),m_WhiteKnight(other.m_WhiteKnight),m_WhiteBishop(other.m_WhiteBishop),m_WhiteRook(other.m_WhiteRook),
        m_WhiteQueen(other.m_WhiteQueen),m_WhiteKing(other.m_WhiteKing),m_BlackPawn(other.m_BlackPawn),m_BlackKnight(other.m_BlackKnight),
        m_BlackBishop(other.m_BlackBishop),m_BlackRook(other.m_BlackRook),m_BlackQueen(other.m_BlackQueen),m_BlackKing(other.m_BlackKing),
        m_White(other.m_White),m_Black(other.m_Black), m_Board(other.m_Board)
    {}

    BitBoard RookAttack(int pos, BitBoard occ) const;
    BitBoard BishopAttack(int pos, BitBoard occ) const;
    BitBoard QueenAttack(int pos, BitBoard occ) const;

    BitBoard RookXray(int pos, BitBoard occ) const;
    BitBoard BishopXray(int pos, BitBoard occ) const;
};

static std::string BoardtoFen(const Board& board, const BoardInfo& info) {
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
        else if (skip > 0) {
            FEN += (char)('0' + skip);
            skip = 0;
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

    if (skip > 0) {
        FEN += (char)('0' + skip);
        skip = 0;
    }

    if (info.m_WhiteMove) {
        FEN += " w ";
    } else {
        FEN += " b ";
    }

    // Castle rights
    if (info.m_WhiteCastleKing) {
        FEN += "K";
    }
    if (info.m_WhiteCastleQueen) {
        FEN += "Q";
    }
    if (info.m_BlackCastleKing) {
        FEN += "k";
    }
    if (info.m_BlackCastleQueen) {
        FEN += "q";
    }
    if (!(info.m_WhiteCastleKing || info.m_WhiteCastleQueen || info.m_BlackCastleKing || info.m_BlackCastleQueen)) {
        FEN += '-';
    }

    FEN += ' ';
    if (info.m_EnPassant > 0) {
        char pos = 'h' - ((int)(log2(info.m_EnPassant)) % 8);
        if (info.m_WhiteMove) {
            FEN += pos + std::to_string(6);
        }
        else {
            FEN += pos + std::to_string(3);
        }
    } else {
        FEN += '-';
    }

    FEN += ' ' + std::to_string(info.m_HalfMoves);
    FEN += ' ' + std::to_string(info.m_FullMoves);

    return FEN;
}

static uint64 Zobrist_Hash(const Board& board, const BoardInfo& info) {
    uint64 result = 0;

    BitBoard wp = board.m_WhitePawn, wkn = board.m_WhiteKnight, wb = board.m_WhiteBishop, wr = board.m_WhiteRook, wq = board.m_WhiteQueen, wk = board.m_WhiteKing,
        bp = board.m_BlackPawn, bkn = board.m_BlackKnight, bb = board.m_BlackBishop, br = board.m_BlackRook, bq = board.m_BlackQueen, bk = board.m_BlackKing;

    while (wp > 0) {
        int pos = PopPos(wp);
        result ^= Lookup::zobrist[pos];
    }

    while (bp > 0) {
        int pos = PopPos(bp);
        result ^= Lookup::zobrist[pos+64];
    }

    // Knights
    while (wkn > 0) {
        int pos = PopPos(wkn);
        result ^= Lookup::zobrist[pos + 64 * 2];
    }

    while (bkn > 0) {
        int pos = PopPos(bkn);
        result ^= Lookup::zobrist[pos + 64 * 3];
    }

    // Bishops
    while (wb > 0) {
        int pos = PopPos(wb);
        result ^= Lookup::zobrist[pos + 64 * 4];
    }

    while (bb > 0) {
        int pos = PopPos(bb);
        result ^= Lookup::zobrist[pos + 64 * 5];
    }

    // Rooks
    while (wr > 0) {
        int pos = PopPos(wr);
        result ^= Lookup::zobrist[pos + 64 * 6];
    }

    while (br > 0) {
        int pos = PopPos(br);
        result ^= Lookup::zobrist[pos + 64 * 7];
    }

    while (wq > 0) {
        int pos = PopPos(wq);
        result ^= Lookup::zobrist[pos + 64 * 8];
    }

    while (bq > 0) {
        int pos = PopPos(bq);
        result ^= Lookup::zobrist[pos + 64 * 9];
    }

    while (wk > 0) {
        int pos = PopPos(wk);
        result ^= Lookup::zobrist[pos + 64 * 10];
    }

    while (bk > 0) {
        int pos = PopPos(bk);
        result ^= Lookup::zobrist[pos + 64 * 11];
    }

    if(info.m_WhiteMove) result ^= Lookup::zobrist[64 * 12];
    if (info.m_WhiteCastleKing) result ^= Lookup::zobrist[64 * 12 + 1];
    if (info.m_WhiteCastleQueen) result ^= Lookup::zobrist[64 * 12 + 2];
    if (info.m_BlackCastleKing) result ^= Lookup::zobrist[64 * 12 + 3];
    if (info.m_BlackCastleQueen) result ^= Lookup::zobrist[64 * 12 + 4];
    if (info.m_EnPassant) result ^= Lookup::zobrist[64 * 12 + 5 + (GET_SQUARE(info.m_EnPassant) % 8)];

    return result;
}

static uint64 Zobrist_PawnHash(const Board& board) {
    uint64 result = 0;

    BitBoard wp = board.m_WhitePawn, bp = board.m_BlackPawn;

    while (wp > 0) {
        int pos = PopPos(wp);
        result ^= Lookup::zobrist[pos];
    }

    while (bp > 0) {
        int pos = PopPos(bp);
        result ^= Lookup::zobrist[pos + 64];
    } 

    return result;
}