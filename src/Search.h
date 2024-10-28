#pragma once

#include "Transposition.h"
#include "Evaluate.h"
#include "TableBase.h"

#include <unordered_map>
#include <algorithm>
#include <vector>
#include <thread>
#include <cmath>

#define MAX_DEPTH 64 // Maximum depth that the engine will go
#define MATE_SCORE 32767/2
#define MIN_ALPHA -32767ll
#define MAX_BETA 32767ll

// Quadratic https://www.chessprogramming.org/Triangular_PV-Table
struct MoveStack {
    Move m_PV[MAX_DEPTH] = {};
    int m_Ply;
};

struct RootMove {
    int score;
    Move move;
    bool operator<(const RootMove& m) const {
        return m.score < score;
    }
};

class Search {
public:
	BoardInfo m_Info[MAX_DEPTH];
	std::unique_ptr<Board> m_RootBoard;
    int64 m_MaxTime;
private:
	uint64 m_Hash[MAX_DEPTH];
    uint64 m_PawnHash[MAX_DEPTH];
    uint64 m_History[256];
    std::vector<std::unique_ptr<std::thread>> m_Threads;
    bool m_Running;
    Timer m_Timer;
	int m_Maxdepth; // Maximum depth currently set
    uint64 m_NodeCnt;
    TranspositionTable* m_Table;
    PawnTable* m_PawnTable;
    std::vector<RootMove> m_RootMoves;
    int m_RootDelta;
public:

	Search() 
        : m_Maxdepth(0), m_Running(false), m_MaxTime(999999999999999), m_NodeCnt(0)
	{
        m_Table = new TranspositionTable(1024 * 1024 * 1024);
        m_PawnTable = new PawnTable(1024 * 1024);
        LoadPosition(Lookup::starting_pos);
    }

    ~Search() {
        delete m_Table;
    }

    void Stop() {
        m_Running = false;
        for (std::unique_ptr<std::thread>& t : m_Threads) {
            if (t->joinable()) {
                t->join();
            }
        }
        m_Threads.clear();
    }

	void LoadPosition(std::string fen) {
        m_Info[0] = FenInfo(fen);
		m_RootBoard = std::make_unique<Board>(Board(fen));
        m_Hash[0] = Zobrist_Hash(*m_RootBoard, m_Info[0]);
        m_PawnHash[0] = Zobrist_PawnHash(*m_RootBoard);
        m_Table->Clear();
        m_History[0] = m_Hash[0];
	}



    // Simple Most Valuable Victim - Least Valuable Aggressor sorting function
    static bool Move_Order(const Move& a, const Move& b) {
        int64 score_a = 0;
        int64 score_b = 0;
        if ((int)a.m_Capture) score_a += 9 + Lookup::pieceValue[(int)a.m_Capture] - Lookup::pieceValue[(int)a.m_Type];
        if ((int)b.m_Capture) score_b += 9 + Lookup::pieceValue[(int)b.m_Capture] - Lookup::pieceValue[(int)b.m_Type];

        return score_a > score_b;
    }

    template<Color white>
	uint64 Perft_r(const Board& board, int depth) {
        if (!m_Running) return 0;
        assert("Hash is not matched!", Zobrist_Hash(board, m_Info[depth]) != m_Hash[depth]);
		const std::vector<Move> moves = TGenerateMoves<white>(board, depth);
		if (depth == m_Maxdepth-1) return moves.size();
		int64 result = 0;
		for (const Move& move : moves) {
			int64 count = Perft_r<!white>(MovePiece(board, move, depth + 1), depth + 1);
			result += count;
			if (depth == 0) {
                if(m_Running) sync_printf("%s: %llu: %s\n", move.toString().c_str(), count, BoardtoFen(MovePiece(board, move, depth + 1), m_Info[depth+1]).c_str());
			}
		}
		return result;
	}

	uint64 Perft(int maxdepth) {
        m_Running = true;
        m_Maxdepth = maxdepth;
		Timer time;
		time.Start();
        uint64 result = m_Info[0].m_WhiteMove ? Perft_r<WHITE>(*m_RootBoard, 0) : Perft_r<BLACK>(*m_RootBoard, 0);
		float t = time.End();
        if(m_Running) sync_printf("%llu\n%.3fMNodes/s\n", result, (result / 1000000.0f) / t);
        m_Running = false;
		return result;
	}

    static void Update_PV(Move* pv, Move move, Move* target) {
        for (*pv++ = move; target && *target != Move();)
            *pv++ = *target++;
        *pv = Move();
    }

    int64 Quiesce(const Board& board, int64 alpha, int64 beta, int ply) {
        m_NodeCnt++;
        int64 bestScore = Evaluate(board, m_PawnTable, m_PawnHash[ply], m_Info[ply].m_WhiteMove);
        if (bestScore >= beta) { // fail soft
            return bestScore;
        }
        if (alpha < bestScore) {
            alpha = bestScore;
        }

        std::vector<Move> moves = GenerateCaptureMoves(board, ply);
        if (moves.size() == 0) {
            uint64 danger = 0, active = 0, rookPin = 0, bishopPin = 0, enPassant = m_Info[ply].m_EnPassant;
            Check(m_Info[ply].m_WhiteMove, board, danger, active, rookPin, bishopPin, enPassant);
            if (active == 0xFFFFFFFFFFFFFFFFull) return 0; // Draw
            return -MATE_SCORE + ply; // Mate in 0 is 10000 centipawns worth
        }

        TTEntry* entry = m_Table->Probe(m_Hash[ply]);
        if (entry != nullptr) {
            const Move hashMove = entry->m_BestMove;
            std::sort(moves.begin(), moves.end(), [hashMove](const Move& a, const Move& b) {
                if (a == hashMove) {
                    return true;
                } else if (b == hashMove) {
                    return false;
                }
                return Move_Order(a, b);
                });
        }
        else {
            std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
                return Move_Order(a, b);
            });
        }

        Move bestMove;
        for (const Move& move : moves) {
            if (move.m_Capture == MoveType::NONE) continue; // Only analyze capturing moves
            int64 score = -Quiesce(MovePiece(board, move, ply + 1), -beta, -alpha, ply + 1);
            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
                if (score > alpha) {
                    alpha = score;
                }
            }
            if (alpha >= beta) {
                break;
            }
        }
        if (bestMove.m_Type != MoveType::NONE) {
            m_Table->Enter(m_Hash[ply], TTEntry(m_Hash[ply], bestMove, bestScore, ply, m_Info[ply].m_FullMoves));
        }

        return bestScore;
    }

	int64 AlphaBeta(const Board& board, MoveStack* stack, int64 alpha, int64 beta, int ply, int depth) {
        m_NodeCnt++;
        if (!m_Running || m_Timer.EndMs() >= m_MaxTime) return 0;

        // Prevent explosions
        assert("Ply is more than MAX_DEPTH!\n", ply >= MAX_DEPTH);
        depth = std::min(depth, MAX_DEPTH - 1);


        // Probe the tablebase
        int success;
        int v = TableBase::Probe_DTZ(board, m_Info[ply], &success);
        if (success) {
            return Signum(v) * (MATE_SCORE-v);
        }


        // Quiesce search if we reached the bottom
        if (depth <= 1) {
			return Quiesce(board, alpha, beta, ply);
		}

        uint64 danger = 0, active = 0, rookPin = 0, bishopPin = 0, enPassant = m_Info[ply].m_EnPassant;
        Check(m_Info[ply].m_WhiteMove, board, danger, active, rookPin, bishopPin, enPassant);
        bool inCheck = false;
        if (active != 0xFFFFFFFFFFFFFFFFull) inCheck = true;

		std::vector<Move> moves = GenerateMoves(board, ply);
		if (moves.size() == 0) {
            if (!inCheck) return 0; // Draw
			return -MATE_SCORE + ply; // Mate in 0 is 10000 centipawns worth
		}

        // We count any repetion as draw
        // TODO: Maybe employ hashing to make this O(1) instead of O(n)
        for (int i = 0; i < m_Info[ply].m_HalfMoves; i++) {
            if (m_History[i] == m_Hash[ply]) {
                return 0;
            }

        }
        
		int64 bestScore = -MATE_SCORE;
        if (*stack->m_PV != Move()) {
            const Move pvMove = *stack->m_PV;
            std::sort(moves.begin(), moves.end(), [pvMove](const Move& a, const Move& b) {
                if (a == pvMove) {
                    return true;
                } else if (b == pvMove) {
                    return false;
                }
                return Move_Order(a, b);
            });
        } else {
            std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
                return Move_Order(a, b);
            });
        }
        
        Move hashMove = Move();
        TTEntry* entry = m_Table->Probe(m_Hash[ply]);
        if (entry != nullptr) {
            hashMove = entry->m_BestMove;
        }

        Move bestMove = moves[0];
		for (const Move& move : moves) {
            int newdepth = depth - 1;
            int extension = 0;
            int delta = beta - alpha;
            if (inCheck && ply < MAX_DEPTH) extension++;

            if (ply >= 4 && move.m_Capture == MoveType::NONE && !inCheck) {
                int reduction = (1500 - delta * 800 / m_RootDelta) / 1024 * std::log(ply);
                // If we are on pv node then decrease reduction
                if (*stack->m_PV != Move()) reduction -= 1;

                // If hash move is not found then increase reduction
                if (hashMove == Move() && depth <= ply * 2) reduction += 1;

                reduction = std::max(0, reduction);
                newdepth = std::max(1, newdepth - reduction);
            }

            newdepth += extension;
            
			int64 score = -AlphaBeta(MovePiece(board, move, ply + 1), stack+1, -beta, -alpha, ply + 1, newdepth);
			if (score > bestScore) {
				bestScore = score;
                bestMove = move;
				if (score > alpha) {
                    Update_PV(stack->m_PV, bestMove, (stack + 1)->m_PV);
					alpha = score;
				}
			}
			if (alpha >= beta) { // Exit out early
				break;
			}
		}
        m_Table->Enter(m_Hash[ply], TTEntry(m_Hash[ply], bestMove, bestScore, ply, m_Info[ply].m_FullMoves));

		return bestScore;
	}

    

    // Calculate time to allocate for a move
    void MoveTimed(int64 wtime, int64 btime, int64 winc, int64 binc) {
        int64 timediff = llabs(wtime - btime);

        bool moreTime = m_Info[0].m_WhiteMove ? wtime > btime : wtime < btime;
        int64 timeleft = m_Info[0].m_WhiteMove ? wtime : btime;
        int64 timeic = m_Info[0].m_WhiteMove ? winc : binc;
        int64 target = (timeleft+ (moreTime ? timediff : 0)) / 40 + 200 + timeic;
        float x = (m_Info[0].m_FullMoves - 20.0f)/30.0f; 
        float factor = exp(-x*x);   // Bell curve
        sync_printf("info movetime %lli\n", (int64)(target * factor));
        UCIMove(target * factor);
    }

    void UCIMove(int64 time) {
        m_MaxTime = time;
        if (m_MaxTime == -1) {
            m_MaxTime = 999999999999999;
        }
        m_Threads.push_back(std::make_unique<std::thread>(&Search::UCIMove_async, &*this));
    }

    void UCIMove_async() {
        m_Running = true;
        m_Maxdepth = 0;

        m_NodeCnt = 1;

        //MoveStack movestack[64] = {};
        MoveStack* stack = new MoveStack[MAX_DEPTH];

        for (int i = 0; i < MAX_DEPTH; i++) {
            (stack + i)->m_Ply = i;
        }

        // Move the moves into rootmoves
        std::vector<Move> generatedMoves = GenerateMoves(*m_RootBoard, 0);
        m_RootMoves.clear();
        m_RootMoves.reserve(generatedMoves.size());  // Reserve space for efficiency

        std::transform(std::make_move_iterator(generatedMoves.begin()),
            std::make_move_iterator(generatedMoves.end()),
            std::back_inserter(m_RootMoves),
            [](Move&& move) {
                return RootMove{ 0, std::move(move) };
            });

        if (m_RootMoves.size() == 0) return;
        
        // Start timer
        m_Timer.Start();
        Move bestMove = m_RootMoves[0].move;
        Move finalMove = bestMove;
        int64 bestScore = -MATE_SCORE;
        int64 rootAlpha = MIN_ALPHA;
        int64 rootBeta = MAX_BETA;
        m_RootDelta = 10;
        while (m_Running && m_Timer.EndMs() < m_MaxTime && MAX_DEPTH > m_Maxdepth) {
            bestScore = -MATE_SCORE;

            m_RootDelta = 10;
            rootAlpha = MIN_ALPHA;
            rootBeta = MAX_BETA;

            int64 alpha, beta;

            m_Maxdepth++;
            if (m_RootMoves.size() == 1) break;

            TTEntry* entry = m_Table->Probe(m_Hash[0]);
            if (entry != nullptr) {
                rootAlpha = entry->m_Evaluation - m_RootDelta;
                rootBeta = entry->m_Evaluation + m_RootDelta;
            }
            int failHigh = 0;
            while (true) {
                alpha = rootAlpha;
                beta = rootBeta;
                bestScore = -MATE_SCORE;
                std::stable_sort(m_RootMoves.begin(), m_RootMoves.end());
                for (RootMove& move : m_RootMoves) {
                    int64 score = -AlphaBeta(MovePiece(*m_RootBoard, move.move, 1), stack+1, -beta, -alpha, 1, std::max(1, m_Maxdepth - failHigh));
                    assert("Evaluation is unreasonably big\n", score >= 72057594);
                    if (score > bestScore) {
                        bestScore = score;
                        bestMove = move.move;
                        move.score = score;
                        if (score > alpha) {
                            Update_PV(stack->m_PV, bestMove, (stack + 1)->m_PV);
                            alpha = score;
                        }
                    }
                    if (alpha >= beta) { // Exit out early
                        break;
                    }
                }

                if (bestScore <= rootAlpha) { // Failed low
                    rootBeta = (rootAlpha + rootBeta) / 2;
                    rootAlpha = std::max(bestScore - m_RootDelta, MIN_ALPHA);
                    failHigh = 0;
                }
                else if (bestScore >= rootBeta) { // Failed high
                    rootBeta = std::min(bestScore + m_RootDelta, MAX_BETA);
                    failHigh++;
                } else {
                    break;
                }

                m_RootDelta *= 1.5;
            }
            if (!m_Running || m_Timer.EndMs() >= m_MaxTime) break;

            // Print pv and search info
            sync_printf("info depth %i score cp %lli time %lli nodes %llu tps %llu\n", m_Maxdepth, (m_Info[0].m_WhiteMove ? 1 : -1) * bestScore, (int64)m_Timer.EndMs(), m_NodeCnt, (uint64)(m_NodeCnt / m_Timer.End()));
            sync_printf("info pv");
            for (int i = 0; i < MAX_DEPTH && stack->m_PV[i] != Move(); i++) {
                sync_printf(" %s", stack->m_PV[i].toString().c_str());
            }
            sync_printf("\n");

            finalMove = bestMove;
            

            m_Table->Enter(m_Hash[0], TTEntry(m_Hash[0], bestMove, bestScore, 0, m_Info[0].m_FullMoves));

            if (m_Timer.EndMs() * 2 >= m_MaxTime) break; // We won't have enough time to calculate more depth anyway
            if (bestScore >= MATE_SCORE - MAX_DEPTH || bestScore <= -MATE_SCORE + MAX_DEPTH) break; // Position is solved so exit out
        }
        sync_printf("bestmove %s\n", finalMove.toString().c_str());
        delete[] stack;
    }

	Board MovePiece(const Board& board, const Move& move, int ply) {
		const BitBoard re = ~move.m_To;
		const BitBoard swp = move.m_From | move.m_To;
		const Color white = m_Info[ply-1].m_WhiteMove;
		const BitBoard wp = board.m_WhitePawn, wkn = board.m_WhiteKnight, wb = board.m_WhiteBishop, wr = board.m_WhiteRook, wq = board.m_WhiteQueen, wk = board.m_WhiteKing,
			bp = board.m_BlackPawn, bkn = board.m_BlackKnight, bb = board.m_BlackBishop, br = board.m_BlackRook, bq = board.m_BlackQueen, bk = board.m_BlackKing;

        m_Info[ply] = m_Info[ply - 1]; // Maybe expensive copy, consider making an unmoving function
        m_Info[ply].m_HalfMoves = m_Info[ply-1].m_HalfMoves+1;
        if (!white) m_Info[ply].m_FullMoves = m_Info[ply - 1].m_FullMoves + 1;
        m_Info[ply].m_EnPassant = 0;
        m_Info[ply].m_WhiteMove = !white;


        int fpos = GET_SQUARE(move.m_From);
        int tpos = GET_SQUARE(move.m_To);

        m_Hash[ply] = m_Hash[ply - 1] ^ Lookup::zobrist[64 * 12];
        m_PawnHash[ply] = m_PawnHash[ply - 1];

        if (m_Info[ply - 1].m_EnPassant) {
            m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 5 + (GET_SQUARE(m_Info[ply - 1].m_EnPassant) % 8)];
        }

        if(move.m_Capture != MoveType::NONE) m_Info[ply].m_HalfMoves = 0;

		if (white) {
            switch (move.m_Capture) {
            case MoveType::PAWN:
                m_Hash[ply] ^= Lookup::zobrist[64 + tpos];
                m_PawnHash[ply] ^= Lookup::zobrist[64 + tpos];
                break;
            case MoveType::EPASSANT:
                m_Hash[ply] ^= Lookup::zobrist[56 + tpos];
                m_PawnHash[ply] ^= Lookup::zobrist[56 + tpos];
                break;
            case MoveType::KNIGHT:
                m_Hash[ply] ^= Lookup::zobrist[64 * 3 + tpos];
                break;
            case MoveType::BISHOP:
                m_Hash[ply] ^= Lookup::zobrist[64 * 5 + tpos];
                break;
            case MoveType::ROOK:
                m_Hash[ply] ^= Lookup::zobrist[64 * 7 + tpos];
                break;
            case MoveType::QUEEN:
                m_Hash[ply] ^= Lookup::zobrist[64 * 9 + tpos];
                break;
            case MoveType::PAWN2: // Should never happen
                assert("Pawn can not take when moving forward twice!");
            case MoveType::NONE:
                // No capture
                break;
            }

			assert("Cant move to same color piece", move.m_To & m_White);
			switch (move.m_Type) {
			case MoveType::PAWN:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[tpos] ^ Lookup::zobrist[fpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[tpos] ^ Lookup::zobrist[fpos];
                m_Info[ply].m_HalfMoves = 0;
				return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::PAWN2:
                m_Info[ply].m_EnPassant = move.m_To >> 8;
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[tpos] ^ Lookup::zobrist[fpos] ^ Lookup::zobrist[64 * 12 + 5 + (tpos % 8)];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[tpos] ^ Lookup::zobrist[fpos] ^ Lookup::zobrist[64 * 12 + 5 + (tpos % 8)];
                m_Info[ply].m_HalfMoves = 0;
				return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::KNIGHT:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 2 + tpos] ^ Lookup::zobrist[64 * 2 + fpos];
				return Board(wp, wkn ^ swp, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::BISHOP:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 4 + tpos] ^ Lookup::zobrist[64 * 4 + fpos];
				return Board(wp, wkn, wb ^ swp, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::ROOK:
                if (move.m_From == 0b1ull) {
                    m_Info[ply].m_WhiteCastleKing = false;
                    if (m_Info[ply - 1].m_WhiteCastleKing) {
                        m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 1];
                    }
                }
                else if (move.m_From == 0b10000000ull) {
                    m_Info[ply].m_WhiteCastleQueen = false;
                    if (m_Info[ply - 1].m_WhiteCastleQueen) {
                        m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 2];
                    }
                }
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 6 + tpos] ^ Lookup::zobrist[64 * 6 + fpos];
				return Board(wp, wkn, wb, wr ^ swp, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::QUEEN:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 8 + tpos] ^ Lookup::zobrist[64 * 8 + fpos];
				return Board(wp, wkn, wb, wr, wq ^ swp, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::KING:
                m_Info[ply].m_WhiteCastleQueen = false;
                m_Info[ply].m_WhiteCastleKing = false;
                if (m_Info[ply - 1].m_WhiteCastleQueen) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 2];
                }
                if (m_Info[ply - 1].m_WhiteCastleKing) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 1];
                }
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 10 + tpos] ^ Lookup::zobrist[64 * 10 + fpos];
				return Board(wp, wkn, wb, wr, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::EPASSANT:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[tpos] ^ Lookup::zobrist[fpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[tpos] ^ Lookup::zobrist[fpos];
				return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & ~(move.m_To >> 8), bkn, bb, br, bq, bk);
			case MoveType::KCASTLE:
                m_Info[ply].m_WhiteCastleQueen = false;
                m_Info[ply].m_WhiteCastleKing = false;
                if (m_Info[ply - 1].m_WhiteCastleQueen) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 2];
                }
                if (m_Info[ply - 1].m_WhiteCastleKing) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 1];
                }
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 10 + 1] ^ Lookup::zobrist[64 * 10 + 3] ^ Lookup::zobrist[64 * 6] ^ Lookup::zobrist[64 * 6 + 2];
				return Board(wp, wkn, wb, wr ^ 0b101ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::QCASTLE:
                m_Info[ply].m_WhiteCastleQueen = false;
                m_Info[ply].m_WhiteCastleKing = false;
                if (m_Info[ply - 1].m_WhiteCastleQueen) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 2];
                }
                if (m_Info[ply - 1].m_WhiteCastleKing) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 1];
                }
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 10 + 5] ^ Lookup::zobrist[64 * 10 + 3] ^ Lookup::zobrist[64 * 6 + 7] ^ Lookup::zobrist[64 * 6 + 4];
				return Board(wp, wkn, wb, wr ^ 0b10010000ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_KNIGHT:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[fpos] ^ Lookup::zobrist[64 * 2 + tpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[fpos];
				return Board(wp & (~move.m_From), wkn | move.m_To, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_BISHOP:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[fpos] ^ Lookup::zobrist[64 * 4 + tpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[fpos];
				return Board(wp & (~move.m_From), wkn, wb | move.m_To, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_ROOK:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[fpos] ^ Lookup::zobrist[64 * 6 + tpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[fpos];
				return Board(wp & (~move.m_From), wkn, wb, wr | move.m_To, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_QUEEN:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[fpos] ^ Lookup::zobrist[64 * 8 + tpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[fpos];
				return Board(wp & (~move.m_From), wkn, wb, wr, wq | move.m_To, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::NONE:
				assert("Invalid move!");
				break;
			}
		}
		else {
            switch (move.m_Capture) {
            case MoveType::PAWN:
            case MoveType::PAWN2:
                m_Hash[ply] ^= Lookup::zobrist[tpos];
                m_PawnHash[ply] ^= Lookup::zobrist[tpos];
                break;
            case MoveType::EPASSANT:
                m_Hash[ply] ^= Lookup::zobrist[8 + tpos];
                break;
            case MoveType::KNIGHT:
                m_Hash[ply] ^= Lookup::zobrist[64 * 2 + tpos];
                break;
            case MoveType::BISHOP:
                m_Hash[ply] ^= Lookup::zobrist[64 * 4 + tpos];
                break;
            case MoveType::ROOK:
                m_Hash[ply] ^= Lookup::zobrist[64 * 6 + tpos];
                break;
            case MoveType::QUEEN:
                m_Hash[ply] ^= Lookup::zobrist[64 * 8 + tpos];
                break;
            case MoveType::NONE:
                // No capture
                break;
            }

			assert("Cant move to same color piece", move.m_To & m_Black);
			switch (move.m_Type) {
			case MoveType::PAWN:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 + tpos] ^ Lookup::zobrist[64 + fpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[64 + tpos] ^ Lookup::zobrist[64 + fpos];
                m_Info[ply].m_HalfMoves = 0;
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk);
			case MoveType::PAWN2:
                m_Info[ply].m_EnPassant = move.m_To << 8;
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 + tpos] ^ Lookup::zobrist[64 + fpos] ^ Lookup::zobrist[64 * 12 + 5 + (tpos % 8)];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[64 + tpos] ^ Lookup::zobrist[64 + fpos] ^ Lookup::zobrist[64 * 12 + 5 + (tpos % 8)];
                m_Info[ply].m_HalfMoves = 0;
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk);
			case MoveType::KNIGHT:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 3 + tpos] ^ Lookup::zobrist[64 * 3 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn ^ swp, bb, br, bq, bk);
			case MoveType::BISHOP:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 5 + tpos] ^ Lookup::zobrist[64 * 5 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb ^ swp, br, bq, bk);
			case MoveType::ROOK:
                if (move.m_From == (1ull << 56)) {
                    m_Info[ply].m_BlackCastleKing = false;
                    if (m_Info[ply - 1].m_BlackCastleKing) {
                        m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 3];
                    }
                }
                else if (move.m_From == (0b10000000ull << 56)) {
                    m_Info[ply].m_BlackCastleQueen = false;
                    if (m_Info[ply - 1].m_BlackCastleQueen) {
                        m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 4];
                    }
                }
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 7 + tpos] ^ Lookup::zobrist[64 * 7 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ swp, bq, bk);
			case MoveType::QUEEN:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 9 + tpos] ^ Lookup::zobrist[64 * 9 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq ^ swp, bk);
			case MoveType::KING:
                m_Info[ply].m_BlackCastleKing = false;
                m_Info[ply].m_BlackCastleQueen = false;
                if (m_Info[ply - 1].m_BlackCastleKing) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 3];
                }
                if (m_Info[ply - 1].m_BlackCastleQueen) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 4];
                }
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 11 + tpos] ^ Lookup::zobrist[64 * 11 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq, bk ^ swp);
			case MoveType::EPASSANT:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 + tpos] ^ Lookup::zobrist[64 + fpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[64 + tpos] ^ Lookup::zobrist[64 + fpos];
				return Board(wp & ~(move.m_To << 8), wkn, wb, wr, wq, wk, bp ^ swp, bkn, bb, br, bq, bk);
			case MoveType::KCASTLE:
                m_Info[ply].m_BlackCastleKing = false;
                m_Info[ply].m_BlackCastleQueen = false;
                if (m_Info[ply - 1].m_BlackCastleKing) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 3];
                }
                if (m_Info[ply - 1].m_BlackCastleQueen) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 4];
                }
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 11 + 57] ^ Lookup::zobrist[64 * 11 + 59] ^ Lookup::zobrist[64 * 7 + 56] ^ Lookup::zobrist[64 * 7 + 58];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ (0b101ull << 56), bq, bk ^ swp);
			case MoveType::QCASTLE:
                m_Info[ply].m_BlackCastleKing = false;
                m_Info[ply].m_BlackCastleQueen = false;
                if (m_Info[ply - 1].m_BlackCastleKing) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 3];
                }
                if (m_Info[ply - 1].m_BlackCastleQueen) {
                    m_Hash[ply] ^= Lookup::zobrist[64 * 12 + 4];
                }
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 * 11 + 61] ^ Lookup::zobrist[64 * 11 + 59] ^ Lookup::zobrist[64 * 7 + 60] ^ Lookup::zobrist[64 * 7 + 63];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ (0b10010000ull << 56), bq, bk ^ swp);
			case MoveType::P_KNIGHT:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 + fpos] ^ Lookup::zobrist[3 * 64 + tpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[64 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn | move.m_To, bb, br, bq, bk);
			case MoveType::P_BISHOP:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 + fpos] ^ Lookup::zobrist[5 * 64 + tpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[64 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb | move.m_To, br, bq, bk);
			case MoveType::P_ROOK:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 + fpos] ^ Lookup::zobrist[7 * 64 + tpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[64 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb, br | move.m_To, bq, bk);
			case MoveType::P_QUEEN:
                m_Hash[ply] = m_Hash[ply] ^ Lookup::zobrist[64 + fpos] ^ Lookup::zobrist[9 * 64 + tpos];
                m_PawnHash[ply] = m_PawnHash[ply] ^ Lookup::zobrist[64 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb, br, bq | move.m_To, bk);
			case MoveType::NONE:
				assert("Invalid move!");
				break;
			}
		}
        m_History[m_Info[ply].m_HalfMoves] = m_Hash[ply];
	}

    void MoveRootPiece(const Move& move) {
        const BitBoard re = ~move.m_To;
        const BitBoard swp = move.m_From | move.m_To;
        const Color white = m_Info[0].m_WhiteMove;
        const BitBoard wp = m_RootBoard->m_WhitePawn, wkn = m_RootBoard->m_WhiteKnight, wb = m_RootBoard->m_WhiteBishop, wr = m_RootBoard->m_WhiteRook, wq = m_RootBoard->m_WhiteQueen, wk = m_RootBoard->m_WhiteKing,
            bp = m_RootBoard->m_BlackPawn, bkn = m_RootBoard->m_BlackKnight, bb = m_RootBoard->m_BlackBishop, br = m_RootBoard->m_BlackRook, bq = m_RootBoard->m_BlackQueen, bk = m_RootBoard->m_BlackKing;

        m_Info[0].m_HalfMoves++;
        if (!white) m_Info[0].m_FullMoves++;
        m_Info[0].m_EnPassant = 0;
        m_Info[0].m_WhiteMove = !white;
        if(move.m_Capture != MoveType::NONE) m_Info[0].m_HalfMoves = 0;

        if (white) {
            assert("Cant move to same color piece", move.m_To & m_White);
            switch (move.m_Type) {
            case MoveType::PAWN:
                m_RootBoard = std::make_unique<Board>(Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                m_Info[0].m_HalfMoves = 0;
                break;
            case MoveType::PAWN2:
                m_Info[0].m_EnPassant = move.m_To >> 8;
                m_RootBoard = std::make_unique<Board>(Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                m_Info[0].m_HalfMoves = 0;
                break;
            case MoveType::KNIGHT:
                m_RootBoard = std::make_unique<Board>(Board(wp, wkn ^ swp, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::BISHOP:
                m_RootBoard = std::make_unique<Board>(Board(wp, wkn, wb ^ swp, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::ROOK:
                if (move.m_From == 0b1ull) {
                    m_Info[0].m_WhiteCastleKing = false;
                }
                else if (move.m_From == 0b10000000ull) {
                    m_Info[0].m_WhiteCastleQueen = false;
                }
                m_RootBoard = std::make_unique<Board>(Board(wp, wkn, wb, wr ^ swp, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::QUEEN:
                m_RootBoard = std::make_unique<Board>(Board(wp, wkn, wb, wr, wq ^ swp, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::KING:
                m_Info[0].m_WhiteCastleQueen = false;
                m_Info[0].m_WhiteCastleKing = false;
                m_RootBoard = std::make_unique<Board>(Board(wp, wkn, wb, wr, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::EPASSANT:
                m_RootBoard = std::make_unique<Board>(Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & ~(move.m_To >> 8), bkn, bb, br, bq, bk));
                break;
            case MoveType::KCASTLE:
                m_Info[0].m_WhiteCastleQueen = false;
                m_Info[0].m_WhiteCastleKing = false;
                m_RootBoard = std::make_unique<Board>(Board(wp, wkn, wb, wr ^ 0b101ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::QCASTLE:
                m_Info[0].m_WhiteCastleQueen = false;
                m_Info[0].m_WhiteCastleKing = false;
                m_RootBoard = std::make_unique<Board>(Board(wp, wkn, wb, wr ^ 0b10010000ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::P_KNIGHT:
                m_RootBoard = std::make_unique<Board>(Board(wp & (~move.m_From), wkn | move.m_To, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::P_BISHOP:
                m_RootBoard = std::make_unique<Board>(Board(wp & (~move.m_From), wkn, wb | move.m_To, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::P_ROOK:
                m_RootBoard = std::make_unique<Board>(Board(wp & (~move.m_From), wkn, wb, wr | move.m_To, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::P_QUEEN:
                m_RootBoard = std::make_unique<Board>(Board(wp & (~move.m_From), wkn, wb, wr, wq | move.m_To, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::NONE:
                assert("Invalid move!");
                break;
            }
        }
        else {
            assert("Cant move to same color piece", move.m_To & m_Black);
            switch (move.m_Type) {
            case MoveType::PAWN:
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk));
                m_Info[0].m_HalfMoves = 0;
                break;
            case MoveType::PAWN2:
                m_Info[0].m_EnPassant = move.m_To << 8;
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk));
                m_Info[0].m_HalfMoves = 0;
                break;
            case MoveType::KNIGHT:
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn ^ swp, bb, br, bq, bk));
                break;
            case MoveType::BISHOP:
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb ^ swp, br, bq, bk));
                break;
            case MoveType::ROOK:
                if (move.m_From == (1ull << 56)) {
                    m_Info[0].m_BlackCastleKing = false;
                }
                else if (move.m_From == (0b10000000ull << 56)) {
                    m_Info[0].m_BlackCastleQueen = false;
                }
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ swp, bq, bk));
                break;
            case MoveType::QUEEN:
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq ^ swp, bk));
                break;
            case MoveType::KING:
                m_Info[0].m_BlackCastleKing = false;
                m_Info[0].m_BlackCastleQueen = false;
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq, bk ^ swp));
                break;
            case MoveType::EPASSANT:
                m_RootBoard = std::make_unique<Board>(Board(wp & ~(move.m_To << 8), wkn, wb, wr, wq, wk, bp ^ swp, bkn, bb, br, bq, bk));
                break;
            case MoveType::KCASTLE:
                m_Info[0].m_BlackCastleKing = false;
                m_Info[0].m_BlackCastleQueen = false;
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ (0b101ull << 56), bq, bk ^ swp));
                break;
            case MoveType::QCASTLE:
                m_Info[0].m_BlackCastleKing = false;
                m_Info[0].m_BlackCastleQueen = false;
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ (0b10010000ull << 56), bq, bk ^ swp));
                break;
            case MoveType::P_KNIGHT:
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn | move.m_To, bb, br, bq, bk));
                break;
            case MoveType::P_BISHOP:
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb | move.m_To, br, bq, bk));
                break;
            case MoveType::P_ROOK:
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb, br | move.m_To, bq, bk));
                break;
            case MoveType::P_QUEEN:
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb, br, bq | move.m_To, bk));
                break;
            case MoveType::NONE:
                assert("Invalid move!");
                break;
            }
        }
        m_Hash[0] = Zobrist_Hash(*m_RootBoard, m_Info[0]);
        m_History[m_Info[0].m_HalfMoves] = m_Hash[0];
    }

    BitBoard Check(Color white, const Board& board, BitBoard& danger, BitBoard& active, BitBoard& rookPin, BitBoard& bishopPin, BitBoard& enPassant) const {
        return white ? TCheck<WHITE>(board, danger, active, rookPin, bishopPin, enPassant) : TCheck <BLACK>(board, danger, active, rookPin, bishopPin, enPassant);
    }

    template<Color white>
    BitBoard TCheck(const Board& board, BitBoard& danger, BitBoard& active, BitBoard& rookPin, BitBoard& bishopPin, BitBoard& enPassant) const {
        constexpr Color enemy = !white;

        int kingsq = GET_SQUARE(King<white>(board));

        BitBoard knightPawnCheck = Lookup::knight_attacks[kingsq] & Knight(board, enemy);


        BitBoard pr = PawnRight(board, enemy) & King<white>(board);
        BitBoard pl = PawnLeft(board, enemy) & King<white>(board);

        knightPawnCheck |= PawnAttackLeft(pr, white); // Reverse the pawn attack to find the attacking pawn
        knightPawnCheck |= PawnAttackRight(pl, white);

        BitBoard enPassantCheck = PawnForward((PawnAttackLeft(pr, white) | PawnAttackRight(pl, white)), white) & enPassant;

        danger = PawnAttack(board, enemy);

        // Active moves, mask for checks
        active = 0xFFFFFFFFFFFFFFFFull;
        BitBoard rookcheck = board.RookAttack(kingsq, board.m_Board) & (Rook(board, enemy) | Queen(board, enemy));
        if (rookcheck > 0) { // If a rook piece is attacking the king
            active = Lookup::active_moves[kingsq * 64 + GET_SQUARE(rookcheck)];
            danger |= Lookup::check_mask[kingsq * 64 + GET_SQUARE(rookcheck)] & ~rookcheck;
        }
        rookPin = 0;
        BitBoard rookpinner = board.RookXray(kingsq, board.m_Board) & (Rook(board, enemy) | Queen(board, enemy));
        while (rookpinner > 0) {
            int pos = PopPos(rookpinner);
            BitBoard mask = Lookup::active_moves[kingsq * 64 + pos];
            if (mask & Player<white>(board)) rookPin |= mask;
        }

        BitBoard bishopcheck = board.BishopAttack(kingsq, board.m_Board) & (Bishop(board, enemy) | Queen(board, enemy));
        if (bishopcheck > 0) { // If a bishop piece is attacking the king
            if (active != 0xFFFFFFFFFFFFFFFFull) { // case where rook and bishop checks king
                active = 0;
            }
            else {
                active = Lookup::active_moves[kingsq * 64 + GET_SQUARE(bishopcheck)];
            }
            danger |= Lookup::check_mask[kingsq * 64 + GET_SQUARE(bishopcheck)] & ~bishopcheck;
        }

        bishopPin = 0;
        BitBoard bishoppinner = board.BishopXray(kingsq, board.m_Board) & (Bishop(board, enemy) | Queen(board, enemy));
        while (bishoppinner > 0) {
            int pos = PopPos(bishoppinner);
            BitBoard mask = Lookup::active_moves[kingsq * 64 + pos];
            if (mask & Player<white>(board)) {
                bishopPin |= mask;
            }
        }

        if (enPassant) {
            if ((King<white>(board) & Lookup::EnPassantRank(white)) && (Pawn<white>(board) & Lookup::EnPassantRank(white)) && ((Rook(board, enemy) | Queen(board, enemy)) & Lookup::EnPassantRank(white))) {
                BitBoard REPawn = PawnRight(board, white) & enPassant;
                BitBoard LEPawn = PawnLeft(board, white) & enPassant;
                if (REPawn) {
                    BitBoard noEPPawn = board.m_Board & ~(PawnForward(enPassant, enemy) | PawnAttackLeft(REPawn, enemy)); // Remove en passanter and en passant target
                    if (board.RookAttack(kingsq, noEPPawn) & (Rook(board, enemy) | Queen(board, enemy))) enPassant = 0; // If there is a rook or queen attacking king after removing pawns
                }
                if (LEPawn) {
                    BitBoard noEPPawn = board.m_Board & ~(PawnForward(enPassant, enemy) | PawnAttackRight(LEPawn, enemy)); // Remove en passanter and en passant target
                    if (board.RookAttack(kingsq, noEPPawn) & (Rook(board, enemy) | Queen(board, enemy))) enPassant = 0; // If there is a rook or queen attacking king after removing pawns
                }
            }
        }

        if (knightPawnCheck && (active != 0xFFFFFFFFFFFFFFFFull)) { // If double checked we have to move the king
            active = 0;
        }
        else if (knightPawnCheck && active == 0xFFFFFFFFFFFFFFFFull) {
            active = knightPawnCheck;
        }



        BitBoard knights = Knight(board, enemy);

        while (knights > 0) { // Loop each bit
            danger |= Lookup::knight_attacks[PopPos(knights)];
        }

        BitBoard bishops = Bishop(board, enemy) | Queen(board, enemy);
        while (bishops > 0) { // Loop each bit
            int pos = PopPos(bishops);
            danger |= (board.BishopAttack(pos, board.m_Board) & ~(1ull << pos));
        }
        BitBoard rooks = Rook(board, enemy) | Queen(board, enemy);

        while (rooks > 0) { // Loop each bit
            int pos = PopPos(rooks);
            danger |= (board.RookAttack(pos, board.m_Board) & ~(1ull << pos));
        }

        danger |= Lookup::king_attacks[GET_SQUARE(King(board, enemy))];

        return enPassantCheck;
   }

   inline std::vector<Move> GenerateMoves(const Board& board, int depth) {
       return m_Info[depth].m_WhiteMove ? TGenerateMoves<WHITE>(board, depth) : TGenerateMoves<BLACK>(board, depth);
   }

    template<Color white>
    std::vector<Move> TGenerateMoves(const Board& board, int depth) {
        std::vector<Move> result;
        result.reserve(64); // Pre allocation increases perft by almost 3x

        constexpr Color enemy = !white;
        BitBoard danger = 0, active = 0, rookPin = 0, bishopPin = 0, enPassant = m_Info[depth].m_EnPassant;
        BitBoard enPassantCheck = TCheck<white>(board, danger, active, rookPin, bishopPin, enPassant);
        BitBoard moveable = ~Player<white>(board) & active;

        // Pawns

        BitBoard pawns = Pawn<white>(board);
        BitBoard nonBishopPawn = pawns & ~bishopPin;

        BitBoard FPawns = PawnForward<white>(nonBishopPawn) & (~board.m_Board) & active; // No diagonally pinned pawn can move forward

        BitBoard F2Pawns = PawnForward<white>(nonBishopPawn & Lookup::StartingPawnRank(white)) & ~board.m_Board;
        F2Pawns = PawnForward<white>(F2Pawns) & (~board.m_Board) & active; // TODO: Use Fpawns to save calculation

        BitBoard pinnedFPawns = FPawns & PawnForward<white>(rookPin);
        BitBoard pinnedF2Pawns = F2Pawns & Pawn2Forward<white>(rookPin);
        BitBoard movePinFPawns = FPawns & rookPin; // Pinned after moving
        BitBoard movePinF2Pawns = F2Pawns & rookPin;
        FPawns = FPawns & (~pinnedFPawns | movePinFPawns);
        F2Pawns = F2Pawns & (~pinnedF2Pawns | movePinF2Pawns);

        for (; const BitBoard bit = PopBit(F2Pawns); F2Pawns > 0) { // Loop each bit
            result.push_back({ Pawn2Forward<enemy>(bit), bit, MoveType::PAWN2 });
        }

        const BitBoard nonRookPawns = pawns & ~rookPin;
        BitBoard RPawns = PawnAttackRight<white>(nonRookPawns) & Enemy<white>(board) & active;
        BitBoard LPawns = PawnAttackLeft<white>(nonRookPawns) & Enemy<white>(board) & active;

        BitBoard pinnedRPawns = RPawns & PawnAttackRight<white>(bishopPin);
        BitBoard pinnedLPawns = LPawns & PawnAttackLeft<white>(bishopPin);
        BitBoard movePinRPawns = RPawns & bishopPin; // is it pinned after move
        BitBoard movePinLPawns = LPawns & bishopPin;
        RPawns = RPawns & (~pinnedRPawns | movePinRPawns);
        LPawns = LPawns & (~pinnedLPawns | movePinLPawns);
        if ((FPawns | RPawns | LPawns) & FirstRank<enemy>()) {
            uint64 FPromote = FPawns & FirstRank<enemy>();
            uint64 RPromote = RPawns & FirstRank<enemy>();
            uint64 LPromote = LPawns & FirstRank<enemy>();

            FPawns = FPawns & ~FirstRank<enemy>();
            RPawns = RPawns & ~FirstRank<enemy>();
            LPawns = LPawns & ~FirstRank<enemy>();

            // Add Forward pawns
            for (; const uint64 bit = PopBit(FPromote); FPromote > 0) {
                result.push_back({ PawnForward<enemy>(bit), bit, MoveType::P_KNIGHT });
                result.push_back({ PawnForward<enemy>(bit), bit, MoveType::P_BISHOP });
                result.push_back({ PawnForward<enemy>(bit), bit, MoveType::P_ROOK });
                result.push_back({ PawnForward<enemy>(bit), bit, MoveType::P_QUEEN });
            }

            for (; BitBoard bit = PopBit(RPromote); RPromote > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::P_KNIGHT, capture });
                result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::P_BISHOP, capture });
                result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::P_ROOK, capture });
                result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::P_QUEEN, capture });
            }

            for (; BitBoard bit = PopBit(LPromote); LPromote > 0) {
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::P_KNIGHT, capture });
                result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::P_BISHOP, capture });
                result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::P_ROOK, capture });
                result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::P_QUEEN, capture });
            }
        }

        for (; const uint64 bit = PopBit(FPawns); FPawns > 0) {
            result.push_back({ PawnForward<enemy>(bit), bit, MoveType::PAWN });
        }

        for (; BitBoard bit = PopBit(RPawns); RPawns > 0) { // Loop each bit
            const MoveType capture = GetCaptureType<enemy>(board, bit);
            result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::PAWN, capture });
        }

        for (; BitBoard bit = PopBit(LPawns); LPawns > 0) {
            const MoveType capture = GetCaptureType<enemy>(board, bit);
            result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::PAWN, capture });
        }

        // En passant

        BitBoard REPawns = PawnAttackRight<white>(nonRookPawns) & enPassant & (active | enPassantCheck);
        BitBoard LEPawns = PawnAttackLeft<white>(nonRookPawns) & enPassant & (active | enPassantCheck);
        BitBoard pinnedREPawns = REPawns & PawnAttackRight<white>(bishopPin);
        BitBoard stillPinREPawns = REPawns & bishopPin;
        REPawns = REPawns & (~pinnedREPawns | stillPinREPawns);
        if (REPawns > 0) {
            BitBoard bit = PopBit(REPawns);
            result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::EPASSANT, MoveType::EPASSANT });
        }

        BitBoard pinnedLEPawns = LEPawns & PawnAttackLeft<white>(bishopPin);
        BitBoard stillPinLEPawns = LEPawns & bishopPin;
        LEPawns = LEPawns & (~pinnedLEPawns | stillPinLEPawns);
        if (LEPawns > 0) {
            BitBoard bit = PopBit(LEPawns);
            result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::EPASSANT, MoveType::EPASSANT });
        }

        // Kinghts

        BitBoard knights = Knight<white>(board) & ~(rookPin | bishopPin); // Pinned knights can not move

        while (knights > 0) { // Loop each bit
            int pos = PopPos(knights);
            BitBoard moves = Lookup::knight_attacks[pos] & moveable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::KNIGHT, capture });
            }
        }

        // Bishops

        BitBoard bishops = Bishop<white>(board) & ~rookPin; // A rook pinned bishop can not move

        BitBoard pinnedBishop = bishopPin & bishops;
        BitBoard notPinnedBishop = bishops ^ pinnedBishop;

        while (pinnedBishop > 0) {
            int pos = PopPos(pinnedBishop);
            BitBoard moves = board.BishopAttack(pos, board.m_Board) & moveable & bishopPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::BISHOP, capture });
            }
        }

        while (notPinnedBishop > 0) {
            int pos = PopPos(notPinnedBishop);
            BitBoard moves = board.BishopAttack(pos, board.m_Board) & moveable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::BISHOP, capture });
            }
        }

        // Rooks
        BitBoard rooks = Rook<white>(board) & ~bishopPin; // A bishop pinned rook can not move

        // Castles
        BitBoard CK = CastleKing<white>(danger, board.m_Board, rooks, depth);
        BitBoard CQ = CastleQueen<white>(danger, board.m_Board, rooks, depth);
        if (CK) {
            result.push_back({ King<white>(board), CK, MoveType::KCASTLE });
        }
        if (CQ) {
            result.push_back({ King<white>(board), CQ, MoveType::QCASTLE });
        }

        BitBoard pinnedRook = rookPin & (rooks);
        BitBoard notPinnedRook = rooks ^ pinnedRook;

        while (pinnedRook > 0) {
            int pos = PopPos(pinnedRook);
            BitBoard moves = board.RookAttack(pos, board.m_Board) & moveable & rookPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::ROOK, capture });
            }
        }

        while (notPinnedRook > 0) {
            int pos = PopPos(notPinnedRook);
            BitBoard moves = board.RookAttack(pos, board.m_Board) & moveable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::ROOK, capture });
            }
        }

        // Queens
        BitBoard queens = Queen<white>(board);

        BitBoard pinnedStraightQueen = rookPin & queens;
        BitBoard pinnedDiagonalQueen = bishopPin & queens;

        BitBoard notPinnedQueen = queens ^ (pinnedStraightQueen | pinnedDiagonalQueen);


        while (pinnedStraightQueen > 0) {
            int pos = PopPos(pinnedStraightQueen);
            BitBoard moves = board.RookAttack(pos, board.m_Board) & moveable & rookPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
            }
        }

        // TODO: Put this in bishop and rook loops instead perhaps
        while (pinnedDiagonalQueen > 0) {
            int pos = PopPos(pinnedDiagonalQueen);
            BitBoard moves = board.BishopAttack(pos, board.m_Board) & moveable & bishopPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
            }
        }

        while (notPinnedQueen > 0) {
            int pos = PopPos(notPinnedQueen);
            BitBoard moves = board.QueenAttack(pos, board.m_Board) & moveable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
            }
        }

        // King
        int kingpos = GET_SQUARE(King<white>(board));
        BitBoard moves = Lookup::king_attacks[kingpos] & ~Player<white>(board);
        moves &= ~danger;

        for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
            const MoveType capture = GetCaptureType<enemy>(board, bit);
            result.push_back({ 1ull << kingpos, bit, MoveType::KING, capture });
        }


        return result;
    }

    inline std::vector<Move> GenerateCaptureMoves(const Board& board, int depth) {
        return m_Info[depth].m_WhiteMove ? TGenerateCaptureMoves<WHITE>(board, depth) : TGenerateCaptureMoves<BLACK>(board, depth);
    }

    template<Color white>
    std::vector<Move> TGenerateCaptureMoves(const Board& board, int depth) {
        std::vector<Move> result;
        result.reserve(64); // Pre allocation

        constexpr Color enemy = !white;
        BitBoard danger = 0, active = 0, rookPin = 0, bishopPin = 0, enPassant = m_Info[depth].m_EnPassant;
        BitBoard enPassantCheck = TCheck<white>(board, danger, active, rookPin, bishopPin, enPassant);
        BitBoard capturable = ~Player<white>(board) & active & Enemy<white>(board);

        // Pawns

        BitBoard pawns = Pawn<white>(board);

        const BitBoard nonRookPawns = pawns & ~rookPin;
        BitBoard RPawns = PawnAttackRight<white>(nonRookPawns) & Enemy<white>(board) & active;
        BitBoard LPawns = PawnAttackLeft<white>(nonRookPawns) & Enemy<white>(board) & active;

        BitBoard pinnedRPawns = RPawns & PawnAttackRight<white>(bishopPin);
        BitBoard pinnedLPawns = LPawns & PawnAttackLeft<white>(bishopPin);
        BitBoard movePinRPawns = RPawns & bishopPin; // is it pinned after move
        BitBoard movePinLPawns = LPawns & bishopPin;
        RPawns = RPawns & (~pinnedRPawns | movePinRPawns);
        LPawns = LPawns & (~pinnedLPawns | movePinLPawns);

        for (; BitBoard bit = PopBit(RPawns); RPawns > 0) { // Loop each bit
            const MoveType capture = GetCaptureType<enemy>(board, bit);
            if ((bit & FirstRank<enemy>()) > 0) { // If promote
                result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::P_KNIGHT, capture });
                result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::P_BISHOP, capture });
                result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::P_ROOK, capture });
                result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::P_QUEEN, capture });
            }
            else {
                result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::PAWN, capture });
            }
        }

        for (; BitBoard bit = PopBit(LPawns); LPawns > 0) { // Loop each bit
            const MoveType capture = GetCaptureType<enemy>(board, bit);
            if ((bit & FirstRank<enemy>()) > 0) { // If promote
                result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::P_KNIGHT, capture });
                result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::P_BISHOP, capture });
                result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::P_ROOK, capture });
                result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::P_QUEEN, capture });
            }
            else {
                result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::PAWN, capture });
            }
        }


        // En passant

        BitBoard REPawns = PawnAttackRight<white>(nonRookPawns) & enPassant & (active | enPassantCheck);
        BitBoard LEPawns = PawnAttackLeft<white>(nonRookPawns) & enPassant & (active | enPassantCheck);
        BitBoard pinnedREPawns = REPawns & PawnAttackRight<white>(bishopPin);
        BitBoard stillPinREPawns = REPawns & bishopPin;
        REPawns = REPawns & (~pinnedREPawns | stillPinREPawns);
        if (REPawns > 0) {
            BitBoard bit = PopBit(REPawns);
            result.push_back({ PawnAttackLeft<enemy>(bit), bit, MoveType::EPASSANT, MoveType::EPASSANT });
        }

        BitBoard pinnedLEPawns = LEPawns & PawnAttackLeft<white>(bishopPin);
        BitBoard stillPinLEPawns = LEPawns & bishopPin;
        LEPawns = LEPawns & (~pinnedLEPawns | stillPinLEPawns);
        if (LEPawns > 0) {
            BitBoard bit = PopBit(LEPawns);
            result.push_back({ PawnAttackRight<enemy>(bit), bit, MoveType::EPASSANT, MoveType::EPASSANT });
        }



        // Kinghts

        BitBoard knights = Knight<white>(board) & ~(rookPin | bishopPin); // Pinned knights can not move

        while (knights > 0) { // Loop each bit
            int pos = PopPos(knights);
            BitBoard moves = Lookup::knight_attacks[pos] & capturable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::KNIGHT, capture });
            }
        }

        // Bishops

        BitBoard bishops = Bishop<white>(board) & ~rookPin; // A rook pinned bishop can not move

        BitBoard pinnedBishop = bishopPin & bishops;
        BitBoard notPinnedBishop = bishops ^ pinnedBishop;

        while (pinnedBishop > 0) {
            int pos = PopPos(pinnedBishop);
            BitBoard moves = board.BishopAttack(pos, board.m_Board) & capturable & bishopPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::BISHOP, capture });
            }
        }

        while (notPinnedBishop > 0) {
            int pos = PopPos(notPinnedBishop);
            BitBoard moves = board.BishopAttack(pos, board.m_Board) & capturable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::BISHOP, capture });
            }
        }

        // Rooks
        BitBoard rooks = Rook<white>(board) & ~bishopPin; // A bishop pinned rook can not move

        BitBoard pinnedRook = rookPin & (rooks);
        BitBoard notPinnedRook = rooks ^ pinnedRook;

        while (pinnedRook > 0) {
            int pos = PopPos(pinnedRook);
            BitBoard moves = board.RookAttack(pos, board.m_Board) & capturable & rookPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::ROOK, capture });
            }
        }

        while (notPinnedRook > 0) {
            int pos = PopPos(notPinnedRook);
            BitBoard moves = board.RookAttack(pos, board.m_Board) & capturable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::ROOK, capture });
            }
        }

        // Queens
        BitBoard queens = Queen<white>(board);

        BitBoard pinnedStraightQueen = rookPin & queens;
        BitBoard pinnedDiagonalQueen = bishopPin & queens;

        BitBoard notPinnedQueen = queens ^ (pinnedStraightQueen | pinnedDiagonalQueen);


        while (pinnedStraightQueen > 0) {
            int pos = PopPos(pinnedStraightQueen);
            BitBoard moves = board.RookAttack(pos, board.m_Board) & capturable & rookPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
            }
        }

        // TODO: Put this in bishop and rook loops instead perhaps
        while (pinnedDiagonalQueen > 0) {
            int pos = PopPos(pinnedDiagonalQueen);
            BitBoard moves = board.BishopAttack(pos, board.m_Board) & capturable & bishopPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
            }
        }

        while (notPinnedQueen > 0) {
            int pos = PopPos(notPinnedQueen);
            BitBoard moves = board.QueenAttack(pos, board.m_Board) & capturable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
            }
        }

        // King
        int kingpos = GET_SQUARE(King<white>(board));
        BitBoard moves = Lookup::king_attacks[kingpos] & ~Player<white>(board) & Enemy<white>(board);
        moves &= ~danger;

        for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
            const MoveType capture = GetCaptureType<enemy>(board, bit);
            result.push_back({ 1ull << kingpos, bit, MoveType::KING, capture });
        }

        if (result.size() > 0) return result;

        // Add atleast one move to avoid checkmate false positive
        const BitBoard nonBishopPawn = pawns & ~bishopPin;

        BitBoard FPawns = PawnForward<white>(nonBishopPawn) & (~board.m_Board) & active; // No diagonally pinned pawn can move forward
        const BitBoard pinnedFPawns = FPawns & PawnForward<white>(rookPin);
        const BitBoard movePinFPawns = FPawns & rookPin; // Pinned after moving
        FPawns = FPawns & (~pinnedFPawns | movePinFPawns);
        for (; const uint64 bit = PopBit(FPawns); FPawns > 0) { // Loop each bit
            if ((bit & FirstRank<enemy>()) > 0) { // If promote
                result.push_back({ PawnForward<enemy>(bit), bit, MoveType::P_KNIGHT });
                result.push_back({ PawnForward<enemy>(bit), bit, MoveType::P_BISHOP });
                result.push_back({ PawnForward<enemy>(bit), bit, MoveType::P_ROOK });
                result.push_back({ PawnForward<enemy>(bit), bit, MoveType::P_QUEEN });
            }
            else {
                result.push_back({ PawnForward<enemy>(bit), bit, MoveType::PAWN });
            }
            return result;
        }


        BitBoard F2Pawns = PawnForward<white>(nonBishopPawn & Lookup::StartingPawnRank(white)) & ~board.m_Board;
        F2Pawns = PawnForward<white>(F2Pawns) & (~board.m_Board) & active; // TODO: Use Fpawns to save calculation

        BitBoard pinnedF2Pawns = F2Pawns & Pawn2Forward<white>(rookPin);
        BitBoard movePinF2Pawns = F2Pawns & rookPin;
        F2Pawns = F2Pawns & (~pinnedF2Pawns | movePinF2Pawns);
        for (; const BitBoard bit = PopBit(F2Pawns); F2Pawns > 0) { // Loop each bit
            result.push_back({ Pawn2Forward<enemy>(bit), bit, MoveType::PAWN2 });
            return result;
        }

        BitBoard moveable = ~Player<white>(board) & active;

        // Kinghts

        knights = Knight<white>(board) & ~(rookPin | bishopPin); // Pinned knights can not move

        while (knights > 0) { // Loop each bit
            int pos = PopPos(knights);
            BitBoard moves = Lookup::knight_attacks[pos] & moveable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::KNIGHT, capture });
                return result;
            }
        }

        // Bishops

        pinnedBishop = bishopPin & bishops;
        notPinnedBishop = bishops ^ pinnedBishop;

        while (pinnedBishop > 0) {
            int pos = PopPos(pinnedBishop);
            BitBoard moves = board.BishopAttack(pos, board.m_Board) & moveable & bishopPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::BISHOP, capture });
                return result;
            }
        }

        while (notPinnedBishop > 0) {
            int pos = PopPos(notPinnedBishop);
            BitBoard moves = board.BishopAttack(pos, board.m_Board) & moveable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::BISHOP, capture });
                return result;
            }
        }

        // Castles
        BitBoard CK = CastleKing<white>(danger, board.m_Board, rooks, depth);
        BitBoard CQ = CastleQueen<white>(danger, board.m_Board, rooks, depth);
        if (CK) {
            result.push_back({ King<white>(board), CK, MoveType::KCASTLE });
            return result;
        }
        if (CQ) {
            result.push_back({ King<white>(board), CQ, MoveType::QCASTLE });
            return result;
        }

        pinnedRook = rookPin & (rooks);
        notPinnedRook = rooks ^ pinnedRook;

        while (pinnedRook > 0) {
            int pos = PopPos(pinnedRook);
            BitBoard moves = board.RookAttack(pos, board.m_Board) & moveable & rookPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::ROOK, capture });
                return result;
            }
        }

        while (notPinnedRook > 0) {
            int pos = PopPos(notPinnedRook);
            BitBoard moves = board.RookAttack(pos, board.m_Board) & moveable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::ROOK, capture });
                return result;
            }
        }

        // Queens

        pinnedStraightQueen = rookPin & queens;
        pinnedDiagonalQueen = bishopPin & queens;

        notPinnedQueen = queens ^ (pinnedStraightQueen | pinnedDiagonalQueen);


        while (pinnedStraightQueen > 0) {
            int pos = PopPos(pinnedStraightQueen);
            BitBoard moves = board.RookAttack(pos, board.m_Board) & moveable & rookPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
                return result;
            }
        }

        // TODO: Put this in bishop and rook loops instead perhaps
        while (pinnedDiagonalQueen > 0) {
            int pos = PopPos(pinnedDiagonalQueen);
            BitBoard moves = board.BishopAttack(pos, board.m_Board) & moveable & bishopPin;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
                return result;
            }
        }

        while (notPinnedQueen > 0) {
            int pos = PopPos(notPinnedQueen);
            BitBoard moves = board.QueenAttack(pos, board.m_Board) & moveable;
            for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType<enemy>(board, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
                return result;
            }
        }

        // King
        moves = Lookup::king_attacks[kingpos] & ~Player<white>(board);
        moves &= ~danger;

        for (; BitBoard bit = PopBit(moves); moves > 0) { // Loop each bit
            const MoveType capture = GetCaptureType<enemy>(board, bit);
            result.push_back({ 1ull << kingpos, bit, MoveType::KING, capture });
            return result;
        }

        return result;
    }

    template<Color white>
    inline int64 CastleKing(uint64 danger, uint64 board, uint64 rooks, int depth) const {
        if constexpr (white) {
            return((m_Info[depth].m_WhiteCastleKing && (board & 0b110) == 0 && (danger & 0b1110) == 0) && (rooks & 0b1) > 0) * (1ull << 1);
        } else {
            return (m_Info[depth].m_BlackCastleKing && ((board & (0b110ull << 56)) == 0) && ((danger & (0b1110ull << 56)) == 0) && (rooks & 0b1ull << 56) > 0) * (1ull << 57);
        }
    }
    
    template<Color white>
    inline int64 CastleQueen(uint64 danger, uint64 board, uint64 rooks, int depth) const {
        if constexpr (white) {
            return ((m_Info[depth].m_WhiteCastleQueen && (board & 0b01110000) == 0 && (danger & 0b00111000) == 0) && (rooks & 0b10000000) > 0) * (1ull << 5);
        } else {
            return (m_Info[depth].m_BlackCastleQueen && ((board & (0b01110000ull << 56)) == 0) && ((danger & (0b00111000ull << 56)) == 0) && (rooks & (0b10000000ull << 56)) > 0) * (1ull << 61);
        }
    }

    uint64 Danger(Color white, const Board& board) {
        Color enemy = !white;
        uint64 danger = PawnAttack(board, enemy);

        uint64 knights = Knight(board, enemy);

        while (knights > 0) { // Loop each bit
            danger |= Lookup::knight_attacks[PopPos(knights)];
        }

        uint64 bishops = Bishop(board, enemy) | Queen(board, enemy);
        while (bishops > 0) { // Loop each bit
            int pos = PopPos(bishops);
            danger |= (board.BishopAttack(pos, board.m_Board) & ~(1ull << pos));
        }
        uint64 rooks = Rook(board, enemy) | Queen(board, enemy);

        while (rooks > 0) { // Loop each bit
            int pos = PopPos(rooks);
            danger |= (board.RookAttack(pos, board.m_Board) & ~(1ull << pos));
        }

        danger |= Lookup::king_attacks[GET_SQUARE(King(board, enemy))];

        return danger;
    }

    template<Color enemy>
    MoveType GetCaptureType(const Board& board, uint64 bit) {
        if (Pawn<enemy>(board) & bit) {
            return MoveType::PAWN;
        } 
        else if (Knight<enemy>(board) & bit) {
            return MoveType::KNIGHT;
        }
        else if (Bishop<enemy>(board) & bit) {
            return MoveType::BISHOP;
        }
        else if (Rook<enemy>(board) & bit) {
            return MoveType::ROOK;
        }
        else if (Queen<enemy>(board) & bit) {
            return MoveType::QUEEN;
        }
        else {
            return MoveType::NONE;
        }
    }

    Move GetMove(std::string str) {
        const Color white = m_Info[0].m_WhiteMove;
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
                type = MoveType::P_QUEEN;
                break;
            }
        }
        else {
            if (Pawn(*m_RootBoard, white) & from) {
                if (Pawn2Forward(Pawn(*m_RootBoard, white) & from, white) == to) {
                    type = MoveType::PAWN2;
                }
                else if (m_Info[0].m_EnPassant & to) {
                    type = MoveType::EPASSANT;
                }
                else {
                    type = MoveType::PAWN;
                }
            }
            else if (Knight(*m_RootBoard, white) & from) {
                type = MoveType::KNIGHT;
            }
            else if (Bishop(*m_RootBoard, white) & from) {
                type = MoveType::BISHOP;
            }
            else if (Rook(*m_RootBoard, white) & from) {
                type = MoveType::ROOK;
            }
            else if (Queen(*m_RootBoard, white) & from) {
                type = MoveType::QUEEN;
            }
            else if (King(*m_RootBoard, white) & from) {
                uint64 danger = Danger(white, *m_RootBoard);
                if (((from & 0b1000) && (to & 0b10)) || ((from & (0b1000ull << 56)) && (to & (0b10ull << 56)))) {
                    type = MoveType::KCASTLE;
                } else if (((from & 0b1000) && (to & 0b100000)) || ((from & (0b1000ull << 56)) && (to & (0b100000ull << 56)))) {
                    type = MoveType::QCASTLE;
                } else {
                    type = MoveType::KING;
                }
            }
        }

        assert("Could not read move", type == MoveType::NONE);
        return Move(from, to, type);
    }

    std::string GetFen() const {
        return BoardtoFen(*m_RootBoard, m_Info[0]);
    }
};

