#include <string.h>

struct Move {
    public:
        int m_Position;
        int m_Moveto;
}

enum Piece {
    NONE = 0,
    WHITE_KING = 1,
    WHITE_PAWN = 2,
    WHITE_KNIGHT = 3,
    WHITE_BISHOP = 4,
    WHITE_ROOK = 5,
    WHITE_QUEEN = 6,
    BLACK_KING = 7,
    BLACK_PAWN = 8,
    BLACK_KNIGHT = 9,
    BLACK_BISHOP = 10,
    BLACK_ROOK = 11,
    BLACK_QUEEN = 12
};

class Board {
    public:
        char m_Board[8][8] = { 0 }; // We start the index with a1 then b1 and so on

        Board();

        void StartingPosition();

}