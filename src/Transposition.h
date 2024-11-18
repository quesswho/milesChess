#pragma once

#include "Movelist.h"

enum Bound {
    NO_BOUND = 0,
    UPPER_BOUND = 1,
    LOWER_BOUND = 2,
    EXACT_BOUND = UPPER_BOUND | LOWER_BOUND
};

// Transposition table entry
struct TTEntry {
    TTEntry()
        : m_Hash(0), m_Moves(0), m_Ply(0), m_Score(0)
    {}

    TTEntry(uint64 hash, Move bestmove, int64 eval, Bound bound, int ply, int depth, int moves)
        : m_Hash(hash), m_BestMove(bestmove), m_Bound(bound), m_Ply(ply), m_Depth(depth), m_Moves(moves), m_Score(eval)
    {}

    Move m_BestMove;
    short m_Ply;
    short m_Depth;
    short m_Moves;
    uint64 m_Hash;
    int64 m_Score;
    Bound m_Bound;
};


struct PTEntry {
    PTEntry()
        : m_Hash(0)
    {}

    PTEntry(uint64 hash, Score eval)
        : m_Hash(hash), m_Score(eval)
    {}

    uint64 m_Hash;
    Score m_Score;
};

template<typename T>
struct HashTable {

    // Size in bytes
    HashTable(uint64 size) {
        m_Count = size / sizeof(T);
        m_Count = 1ull << ((uint64)floor(log2(m_Count)));
        m_Indexer = m_Count - 1;
        m_Table = new T[m_Count];

        Clear();
    }
    ~HashTable() {
        delete[] m_Table;
    }

    // Will clear the table so make sure it's not being used
    void Resize(uint64 size) {
        m_Count = size / sizeof(T);
        m_Count = 1ull << ((uint64)floor(log2(m_Count)));
        m_Indexer = m_Count - 1;
        delete[] m_Table;
        m_Table = new T[m_Count];
        Clear();
    }

    
    void Enter(uint64 hash, T entry) {
        uint64 index = hash & m_Indexer;
        if constexpr (std::is_same_v<TTEntry, T>) {
            if (m_Table[index].m_Moves + m_Table[index].m_Ply < entry.m_Moves + entry.m_Ply) {
                m_Table[hash & m_Indexer] = entry;
            }
        } else {
            m_Table[hash & m_Indexer] = entry;
        }
    }
    void Clear() {
        // TODO: Why are we not doing memset instead??
        for (uint64 i = 0; i < m_Count; i++) {
            m_Table[i].m_Hash = 0;
            //m_Table[i].m_Moves = 0;
        }
    }

    T* Probe(uint64 hash) {
        uint64 index = hash & m_Indexer;
        if (m_Table[index].m_Hash == hash) {
            return &m_Table[index];
        }
        return nullptr;
    }

    // length in 2^(size)
    uint64 m_Count;
    T* m_Table;
private:
    uint64 m_Indexer;
};

using TranspositionTable = HashTable<TTEntry>;
using PawnTable = HashTable<PTEntry>;