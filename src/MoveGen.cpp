#include "MoveGen.h"


MoveGen::MoveGen(Position& position, Move hashmove)
    : m_Position(position), m_HashMove(hashmove), m_Current(&m_Moves[0]), m_End(&m_Moves[0])
{}

static bool movecomp(const ScoreMove& a, const ScoreMove& b) {
    return a.score > b.score;
}

Move MoveGen::Next() {
    while (true) {
        switch (m_Stage) {
        case TT_MOVE:
        case QUIESCENCE_TT:
            m_Stage++;
            // TODO: Check if hashmove is legal
            if (m_HashMove != 0) return m_HashMove;
            break;
        case CAPTURE_INIT:
            GenerateMoves<QUIESCENCE>();
            // sort
            if(m_Current != m_End) std::sort(m_Current, m_End - 1, movecomp);
            m_Stage++;
        case CAPTURE_MOVE:
            if (m_Current->move == m_HashMove) m_Current++;
            if (m_Current != m_End) {
                return (*(m_Current++)).move;
            }
            m_Stage++;
        case QUIET_INIT:
            GenerateMoves<SILENT>();
            // sort
            if (m_Current != m_End) std::sort(m_Current, m_End - 1, movecomp);
            m_Stage++;
        case QUIET_MOVE:
            if (m_Current->move == m_HashMove) m_Current++;
            if (m_Current != m_End) {
                return (*(m_Current++)).move;
            }
            return 0; // Finished
        case QUIESCENCE_INIT:
            GenerateMoves<QUIESCENCE>();
            // sort
            if (m_Current != m_End) std::sort(m_Current, m_End - 1, movecomp);
            m_Stage++;
        case QUIESCENCE_MOVE:
            if (m_Current->move == m_HashMove) m_Current++;
            if (m_Current != m_End) {
                return (*(m_Current++)).move;
            }
            return 0; // Finished
        }
    }
}