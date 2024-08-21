#include <string>
#include <array>
#include <vector>


#include "LookupTables.h"


#define GET_SQUARE(X) _tzcnt_u64(X)

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
    while (i < FEN.size()) {
        c = FEN[i++];
        info.m_FullMoves = info.m_FullMoves * 10 + (c - '0');
    }
    return info;
}

enum class MoveType {
    NONE, PAWN, PAWN2, KNIGHT, BISHOP, ROOK, QUEEN, KING, EPASSANT,
    KCASTLE, QCASTLE, P_KNIGHT, P_BISHOP, P_ROOK, P_QUEEN
};

struct Move {

    Move()
        : m_From(0), m_To(0), m_Type(MoveType::NONE)
    {}

    Move(uint64 from, uint64 to, MoveType movetype)
        : m_From(from), m_To(to), m_Type(movetype)
    {}
    uint64 m_From;
    uint64 m_To;
    MoveType m_Type;

    inline std::string toString() const {
        std::string result = "";
        int f = GET_SQUARE(m_From);
        result += 'h' - (f % 8);
        result += '1' + (f / 8);
        int t = GET_SQUARE(m_To);
        result += 'h' - (t % 8);
        result += '1' + (t / 8);
        return result;
    }
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
    const uint64 m_WhitePawn;
    const uint64 m_WhiteKnight;
    const uint64 m_WhiteBishop;
    const uint64 m_WhiteRook;
    const uint64 m_WhiteQueen;
    const uint64 m_WhiteKing;

    const uint64 m_BlackPawn;
    const uint64 m_BlackKnight;
    const uint64 m_BlackBishop;
    const uint64 m_BlackRook;
    const uint64 m_BlackQueen;
    const uint64 m_BlackKing;

    const uint64 m_White;
    const uint64 m_Black;
    const uint64 m_Board;

    BoardInfo m_BoardInfo; // TODO: Move this somewhere else so Board can be constant

    Board(const std::string& FEN);

    Board(uint64 wp, uint64 wkn, uint64 wb, uint64 wr, uint64 wq, uint64 wk, uint64 bp, uint64 bkn, uint64 bb, uint64 br, uint64 bq, uint64 bk, BoardInfo info)
        : m_WhitePawn(wp), m_WhiteKnight(wkn), m_WhiteBishop(wb), m_WhiteRook(wr), m_WhiteQueen(wq), m_WhiteKing(wk), 
          m_BlackPawn(bp), m_BlackKnight(bkn), m_BlackBishop(bb), m_BlackRook(br), m_BlackQueen(bq), m_BlackKing(bk),
          m_White(wp | wkn | wb | wr | wq | wk), m_Black(bp | bkn | bb | br | bq | bk), m_Board(m_White | m_Black),
          m_BoardInfo(info)
    {}

    Board(const Board& other)
        : m_WhitePawn(other.m_WhitePawn),m_WhiteKnight(other.m_WhiteKnight),m_WhiteBishop(other.m_WhiteBishop),m_WhiteRook(other.m_WhiteRook),
        m_WhiteQueen(other.m_WhiteQueen),m_WhiteKing(other.m_WhiteKing),m_BlackPawn(other.m_BlackPawn),m_BlackKnight(other.m_BlackKnight),
        m_BlackBishop(other.m_BlackBishop),m_BlackRook(other.m_BlackRook),m_BlackQueen(other.m_BlackQueen),m_BlackKing(other.m_BlackKing),
        m_White(other.m_White),m_Black(other.m_Black), m_Board(other.m_Board), m_BoardInfo(other.m_BoardInfo)
    {}

    uint64 RookAttack(int pos, uint64 occ) const;
    uint64 BishopAttack(int pos, uint64 occ) const;
    uint64 QueenAttack(int pos, uint64 occ) const;

    uint64_t RookXray(int pos, uint64_t occ) const;
    uint64_t BishopXray(int pos, uint64_t occ) const;

    uint64 PawnForward(uint64 pawns, const bool white) const;
    uint64 Pawn2Forward(uint64 pawns, const bool white) const;
    uint64 PawnRight(const bool white) const;
    uint64 PawnLeft(const bool white) const;
    uint64 PawnAttack(const bool white) const;
    uint64 PawnAttackRight(uint64 pawns, const bool white) const;
    uint64 PawnAttackLeft(uint64 pawns, const bool white) const;

    Board MovePiece(const Move& move) const;

    uint64 Check(uint64& danger, uint64& check, uint64& rookPin, uint64& bishopPin, uint64& enPassant) const;

    inline uint64 Player(const bool white) const {
        return white ? m_White : m_Black;
    }

    inline uint64 Enemy(const bool white) const {
        return white ? m_Black : m_White;
    }

    inline uint64 Pawn(const bool white) const {
        return white ? m_WhitePawn : m_BlackPawn;
    }

    inline uint64 Knight(const bool white) const {
        return white ? m_WhiteKnight : m_BlackKnight;
    }

    inline uint64 Bishop(const bool white) const {
        return white ? m_WhiteBishop : m_BlackBishop;
    }

    inline uint64 Rook(const bool white) const {
        return white ? m_WhiteRook : m_BlackRook;
    }

    inline uint64 Queen(const bool white) const {
        return white ? m_WhiteQueen : m_BlackQueen;
    }

    inline uint64 King(const bool white) const {
        return white ? m_WhiteKing : m_BlackKing;
    }

    inline uint64 CastleKing(uint64 danger, const bool white) const {
        return white ? ((m_BoardInfo.m_WhiteCastleKing && (m_Board & 0b110) == 0 && (danger & 0b1110) == 0) && (m_WhiteRook & 0b1) > 0) * (1ull << 1) : (m_BoardInfo.m_BlackCastleKing && ((m_Board & (0b110ull << 56)) == 0) && ((danger & (0b1110ull << 56)) == 0) && (m_BlackRook & 0b1ull << 56) > 0) * (1ull << 57);
    }

    inline uint64 CastleQueen(uint64 danger, const bool white) const {
        return white ? ((m_BoardInfo.m_WhiteCastleQueen && (m_Board & 0b01110000) == 0 && (danger & 0b00111000) == 0) && (m_WhiteRook & 0b10000000) > 0) * (1ull << 5) : (m_BoardInfo.m_BlackCastleQueen && ((m_Board & (0b01110000ull << 56)) == 0) && ((danger & (0b00111000ull << 56)) == 0) && (m_BlackRook & (0b10000000ull << 56)) > 0) * (1ull << 61);
    }
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

static Move GetMove(std::string str, const Board& board) {
    const bool white = board.m_BoardInfo.m_WhiteMove;
    const uint64 from = 1ull << ('h' - str[0] + (str[1] - '1') * 8);
    const int posTo = ('h' - str[2] + (str[3] - '1') * 8);
    const uint64 to = 1ull << posTo;

    MoveType type = MoveType::NONE;
    if (str.size() > 4) {
        switch (str[4]) {
        case 'k':
            type = MoveType::P_KNIGHT;
            break;
        case 'b':
            type = MoveType::P_BISHOP;
            break;
        case 'r':
            type = MoveType::P_ROOK;
            break;
        case 'q':
            type = MoveType::P_ROOK;
            break;
        }
    }
    else {
        if (board.Pawn(white) & from) {
            if (board.Pawn2Forward(board.Pawn(white) & from, white) == to) {
                type = MoveType::PAWN2;
            }
            else if (board.m_BoardInfo.m_EnPassant & to) {
                type = MoveType::EPASSANT;
            }
            else {
                type = MoveType::PAWN;
            }
        }
        else if (board.Knight(white) & from) {
            type = MoveType::KNIGHT;
        }
        else if (board.Bishop(white) & from) {
            type = MoveType::BISHOP;
        }
        else if (board.Rook(white) & from) {
            type = MoveType::ROOK;
        }
        else if (board.Queen(white) & from) {
            type = MoveType::QUEEN;
        }
        else if (board.King(white) & from) {
            uint64 danger, a, b, c,d;
            board.Check(danger,a,b,c,d);
            if (board.CastleKing(danger, white) && !(Lookup::king_attacks[posTo] & from)) {
                type = MoveType::KCASTLE;
            } else if (board.CastleQueen(danger, white) && !(Lookup::king_attacks[posTo] & from)) {
                type = MoveType::QCASTLE;
            } else {
                type = MoveType::KING;
            }
        }
    }

    assert("Could not read move", type == MoveType::NONE);
    return Move(from, to, type);
}
