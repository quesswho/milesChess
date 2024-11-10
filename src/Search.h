#pragma once

#include "Transposition.h"
#include "Evaluate.h"
#include "TableBase.h"
#include "MoveGen.h"

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
    int64 m_MaxTime;
    Position m_Position;
private:
	//uint64 m_Hash[MAX_DEPTH];
    //uint64 m_PawnHash[MAX_DEPTH];
    //uint64 m_History[256];
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
        m_Table->Clear();
        m_Position.SetPosition(fen);
	}

    template<Color white>
	uint64 Perft_r(Position& pos, int depth) {
        if (!m_Running) return 0;
        assert("Hash is not matched!", Zobrist_Hash(pos) != pos.m_Hash);
		//const std::vector<Move> moves = TGenerateMoves<ALL, white>(pos);
        MoveGen moveGen(pos, 0, false);
        if (depth == m_Maxdepth - 1) {
            uint64 count = 0;
            while ((moveGen.Next()) != 0) count++;
            return count;
        }
		int64 result = 0;
		//for (const Move& move : moves) {
        Move move;
        while ((move = moveGen.Next()) != 0) {
            pos.MovePiece(move);
			int64 count = Perft_r<!white>(pos, depth + 1);
			result += count;
			if (depth == 0) {
                if(m_Running) sync_printf("%s: %llu: %s\n", MoveToString(move).c_str(), count, m_Position.ToFen().c_str());
			}
            pos.UndoMove(move);
		}
		return result;
	}

	uint64 Perft(int maxdepth) {
        m_Running = true;
        m_Maxdepth = maxdepth;
		Timer time;
		time.Start();
        uint64 result = m_Position.m_WhiteMove ? Perft_r<WHITE>(m_Position, 0) : Perft_r<BLACK>(m_Position, 0);
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

    int64 Quiesce(Position& board, int64 alpha, int64 beta, int ply, int depth) {
        m_NodeCnt++;
        int64 bestScore = Evaluate(board, m_PawnTable);
        if (bestScore >= beta) { // Return if we fail soft
            return bestScore;
        }
        if (alpha < bestScore) {
            alpha = bestScore;
        }

        // Probe Transposition table
        Move hashMove = Move();
        TTEntry* entry = m_Table->Probe(board.m_Hash);
        if (entry != nullptr) {
            if (entry->m_Depth >= depth && (entry->m_Bound & (entry->m_Value >= beta ? LOWER_BOUND : UPPER_BOUND))) {
                return entry->m_Value;
            }
            hashMove = entry->m_BestMove;
        }


        Move bestMove = 0;
        int movecnt = 0;

        MoveGen moveGen(board, hashMove, true);
        Move move;
        while ((move = moveGen.Next()) != 0) {
            movecnt++;
            if (CaptureType(move) == ColoredPieceType::NOPIECE) continue; // Only analyze capturing moves

            board.MovePiece(move);
            int64 score = -Quiesce(board, -beta, -alpha, ply + 1, depth - 1);
            board.UndoMove(move);
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

        // Check for mate or stalemate
        if (!movecnt) {
            if (board.m_InCheck) {
                std::vector<Move> mate = GenerateMoves<ALL>(board); // TODO: Generate evasions in Quiescense which will give mate or draw if movecnt == 0
                if(mate.size() == 0) bestScore = -MATE_SCORE + ply;
            } else { // Commented out because it yields better performance without it even though it may not be correct
                //std::vector<Move> mate = GenerateMoves<ALL>(board);
                //if (mate.size() == 0) bestScore = 0;
            }
        }

        if (bestMove != 0) {
            if (bestScore >= beta) {
                m_Table->Enter(board.m_Hash, TTEntry(board.m_Hash, bestMove, bestScore, LOWER_BOUND, ply, depth, board.m_FullMoves));
            } else {
                m_Table->Enter(board.m_Hash, TTEntry(board.m_Hash, bestMove, bestScore, UPPER_BOUND, ply, depth, board.m_FullMoves));
            }
        }

        return bestScore;
    }

	int64 AlphaBeta(Position& board, MoveStack* stack, int64 alpha, int64 beta, int ply, int depth) {
        m_NodeCnt++;
        if (!m_Running || m_Timer.EndMs() >= m_MaxTime) return 0;

        // Prevent explosions
        assert("Ply is more than MAX_DEPTH!\n", ply >= MAX_DEPTH);
        depth = std::min(depth, MAX_DEPTH - 1);


        // Probe Transposition table
        Move hashMove = Move();
        TTEntry* entry = m_Table->Probe(board.m_Hash);
        if (entry != nullptr) {
            if (entry->m_Depth >= depth && (entry->m_Bound & (entry->m_Value >= beta ? LOWER_BOUND : UPPER_BOUND))) {
                return entry->m_Value;
            }
            hashMove = entry->m_BestMove;
        }

        // Probe the tablebase
        int success;
        int v = TableBase::Probe_DTZ(board, &success);
        if (success) {
            return Signum(v) * (MATE_SCORE-v);
        }


        // Quiesce search if we reached the bottom
        if (depth <= 1) {
			return Quiesce(board, alpha, beta, ply, depth);
		}

        // We count any repetion as draw
        // TODO: Maybe employ hashing to make this O(1) instead of O(n)
        for (int i = 0; i < board.m_States[ply].m_HalfMoves && i < board.m_Ply; i++) {
            if (board.m_States[i].m_Hash == board.m_Hash) {
                return 0;
            }
        }

        
		int64 bestScore = -MATE_SCORE;
        int64 old_alpha = alpha;
        Move bestMove = 0;

        int movecnt = 0;

        MoveGen moveGen(board, hashMove, false);
        Move move;
		while ((move = moveGen.Next()) != 0) {
            movecnt++;
            int newdepth = depth - 1;
            int extension = 0;
            int delta = beta - alpha;

            
            board.MovePiece(move);

            if (ply >= 4 && CaptureType(move) == ColoredPieceType::NOPIECE && !board.m_InCheck) {
                int reduction = (1500 - delta * 800 / m_RootDelta) / 1024 * std::log(ply);
                // If we are on pv node then decrease reduction
                if (*stack->m_PV != Move()) reduction -= 1;

                // If hash move is not found then increase reduction
                if (hashMove == Move() && depth <= ply * 2) reduction += 1;

                reduction = std::max(0, reduction);
                newdepth = std::max(1, newdepth - reduction);
            }

            newdepth += extension;
            if (board.m_InCheck && ply < MAX_DEPTH) extension++;

			int64 score = -AlphaBeta(board, stack+1, -beta, -alpha, ply + 1, newdepth);
            board.UndoMove(move);
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

        // Check for mate or stalemate
        if (!movecnt) {
            if (board.m_InCheck) {
                bestScore = -MATE_SCORE + ply;
            } else {
                bestScore = 0;
            }
        }

        if (bestScore >= beta) {
            m_Table->Enter(board.m_Hash, TTEntry(board.m_Hash, bestMove, bestScore, LOWER_BOUND, ply, depth, board.m_FullMoves));
        } else {
            m_Table->Enter(board.m_Hash, TTEntry(board.m_Hash, bestMove, bestScore, UPPER_BOUND, ply, depth, board.m_FullMoves));
        }

		return bestScore;
	}

    

    // Calculate time to allocate for a move
    void MoveTimed(int64 wtime, int64 btime, int64 winc, int64 binc) {
        int64 timediff = llabs(wtime - btime);

        bool moreTime = m_Position.m_WhiteMove ? wtime > btime : wtime < btime;
        int64 timeleft = m_Position.m_WhiteMove ? wtime : btime;
        int64 timeinc = m_Position.m_WhiteMove ? winc : binc;

        int64 est_movesleft = std::max(60 - m_Position.m_FullMoves, 20ull);
        int64 est_timeleft = timeleft + est_movesleft * timeinc;

        int64 target = est_timeleft / est_movesleft - 20; // - overhead
        float x = (m_Position.m_FullMoves - 20.0f)/30.0f;
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
        std::vector<Move> generatedMoves = GenerateMoves<ALL>(m_Position);
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

            TTEntry* entry = m_Table->Probe(m_Position.m_Hash);
            if (entry != nullptr) {
                rootAlpha = entry->m_Value - m_RootDelta;
                rootBeta = entry->m_Value + m_RootDelta;
            }
            int failHigh = 0;
            while (true) {
                alpha = rootAlpha;
                beta = rootBeta;
                bestScore = -MATE_SCORE;
                std::stable_sort(m_RootMoves.begin(), m_RootMoves.end());
                for (RootMove& move : m_RootMoves) {
                    m_Position.MovePiece(move.move);
                    int64 score = -AlphaBeta(m_Position, stack+1, -beta, -alpha, 1, std::max(1, m_Maxdepth - failHigh));
                    m_Position.UndoMove(move.move);
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
            sync_printf("info depth %i score cp %lli time %lli nodes %llu tps %llu\n", m_Maxdepth, (m_Position.m_WhiteMove ? 1 : -1) * bestScore, (int64)m_Timer.EndMs(), m_NodeCnt, (uint64)(m_NodeCnt / m_Timer.End()));
            sync_printf("info pv");
            for (int i = 0; i < MAX_DEPTH && stack->m_PV[i] != Move(); i++) {
                sync_printf(" %s", MoveToString(stack->m_PV[i]).c_str());
            }
            sync_printf("\n");

            finalMove = bestMove;
            
            if (bestScore >= beta) {
                m_Table->Enter(m_Position.m_Hash, TTEntry(m_Position.m_Hash, bestMove, bestScore, LOWER_BOUND, 0, m_Maxdepth, m_Position.m_FullMoves));
            } else {
                m_Table->Enter(m_Position.m_Hash, TTEntry(m_Position.m_Hash, bestMove, bestScore, UPPER_BOUND, 0, m_Maxdepth, m_Position.m_FullMoves));
            }

            
            if (m_Timer.EndMs() * 2 >= m_MaxTime) break; // We won't have enough time to calculate more depth anyway
            if (bestScore >= MATE_SCORE - MAX_DEPTH || bestScore <= -MATE_SCORE + MAX_DEPTH) break; // Position is solved so exit out
        }
        sync_printf("bestmove %s\n", MoveToString(finalMove).c_str());
        delete[] stack;
    }

    template<Color white>
    uint64 TDanger(const Position& board) {
        constexpr Color enemy = !white;

        uint64 danger = PawnAttack<enemy>(board);

        uint64 knights = Knight<enemy>(board);

        while (knights > 0) { // Loop each bit
            danger |= Lookup::knight_attacks[PopPos(knights)];
        }

        uint64 bishops = Bishop<enemy>(board) | Queen<enemy>(board);
        while (bishops > 0) { // Loop each bit
            int pos = PopPos(bishops);
            danger |= (board.BishopAttack(pos, board.m_Board) & ~(1ull << pos));
        }
        uint64 rooks = Rook<enemy>(board) | Queen<enemy>(board);

        while (rooks > 0) { // Loop each bit
            int pos = PopPos(rooks);
            danger |= (board.RookAttack(pos, board.m_Board) & ~(1ull << pos));
        }

        danger |= Lookup::king_attacks[GET_SQUARE(King<enemy>(board))];

        return danger;
    }

    Move GetMove(std::string str) {
        return m_Position.m_WhiteMove ? TGetMove<WHITE>(str) : TGetMove<BLACK>(str);
    }

    template<Color white>
    Move TGetMove(std::string str) {
        
        const BoardPos fromPos = ('h' - str[0] + (str[1] - '1') * 8);
        const BitBoard from = 1ull << fromPos;
        const BoardPos toPos = ('h' - str[2] + (str[3] - '1') * 8);
        const BitBoard to = 1ull << toPos;
        ColoredPieceType capture = GetCaptureType<!white>(m_Position, 1ull << toPos);

        uint8 flags = 0;

        ColoredPieceType type = NOPIECE;
        if (str.size() > 4) {
            switch (str[4]) {
            case 'k':
                flags |= 0b100;
                break;
            case 'b':
                flags |= 0b1000;
                break;
            case 'r':
                flags |= 0b10000;
                break;
            case 'q':
                flags |= 0b100000;
                break;
            }
        }
        if (Pawn<white>(m_Position) & from) {
            type = GetColoredPiece<white>(PAWN);
            if (m_Position.m_States[m_Position.m_Ply].m_EnPassant & to) {
                flags |= 0b1;
                capture = GetColoredPiece<!white>(PAWN);
            }
        }
        else if (Knight<white>(m_Position) & from) {
            type = GetColoredPiece<white>(KNIGHT);
        }
        else if (Bishop<white>(m_Position) & from) {
            type = GetColoredPiece<white>(BISHOP);
        }
        else if (Rook<white>(m_Position) & from) {
            type = GetColoredPiece<white>(ROOK);
        }
        else if (Queen<white>(m_Position) & from) {
            type = GetColoredPiece<white>(QUEEN);
        }
        else if (King<white>(m_Position) & from) {
            type = GetColoredPiece<white>(KING);
            if (((from & 0b1000) && (to & 0b10)) 
                || ((from & (0b1000ull << 56)) && (to & (0b10ull << 56)))
                || ((from & 0b1000) && (to & 0b100000)) 
                || ((from & (0b1000ull << 56)) && (to & (0b100000ull << 56)))) {

                flags |= 0b10;
            } 
        }

        

        assert("Could not read move", type == MoveType::NONE);
        return fromPos | toPos << 6 | type << 12 | capture << 16 | flags << 20;
    }

    std::string GetFen() const {
        return m_Position.ToFen();
    }
};

