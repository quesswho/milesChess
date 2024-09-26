#pragma once

#include "Movelist.h"

// Transposition table entry
struct TTEntry {
    TTEntry()
        : m_Hash(0), m_Moves(0), m_Depth(0)
    {}

    TTEntry(uint64 hash, Move bestmove, int64 eval, int depth, int moves)
        : m_Hash(hash), m_BestMove(bestmove), m_Depth(depth), m_Moves(moves), m_Evaluation(eval)
    {}

    Move m_BestMove;
    short m_Depth;
    short m_Moves;
    uint64 m_Hash;
    int64 m_Evaluation;
};

struct TranspositionTable {
    // Size in bytes
    TranspositionTable(uint64 size) {
        m_Count = size / sizeof(TTEntry);
        m_Count = 1ull << ((uint64)floor(log2(m_Count)));
        m_Indexer = m_Count - 1;
        m_Table = new TTEntry[m_Count];

        Clear();
    }
    ~TranspositionTable() {
        delete[] m_Table;
    }

    // Will clear the table so make sure it's not being used
    void Resize(uint64 size) {
        m_Count = size / sizeof(TTEntry);
        m_Count = 1ull << ((uint64)floor(log2(m_Count)));
        m_Indexer = m_Count - 1;
        delete[] m_Table;
        m_Table = new TTEntry[m_Count];
        Clear();
    }

    void Enter(uint64 hash, TTEntry entry) {
        uint64 index = hash & m_Indexer;
        if (m_Table[index].m_Moves + m_Table[index].m_Depth < entry.m_Moves + entry.m_Depth) {
            m_Table[hash & m_Indexer] = entry;
        }
    }
    void Clear() {
        for (uint64 i = 0; i < m_Count; i++) {
            m_Table[i].m_Hash = 0;
            m_Table[i].m_Moves = 0;
        }
    }

    TTEntry* Get(uint64 hash) {
        uint64 index = hash & m_Indexer;
        if (m_Table[index].m_Hash == hash) {
            return &m_Table[index];
        }
        return nullptr;
    }

    // length in 2^(size)
    uint64 m_Count;
    TTEntry* m_Table;

private:
    uint64 m_Indexer;
};