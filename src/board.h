#include <string>

using uint64 = unsigned long long;


static uint64 FenToMap(std::string FEN, char p);

class Board {
public:
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

    bool m_Whitetoplay;

    Board();

    void StartingPosition();
};