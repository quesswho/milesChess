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
#define NONE_SCORE 32766
#define MIN_ALPHA -32767ll
#define MAX_BETA 32767ll

// Quadratic https://www.chessprogramming.org/Triangular_PV-Table
struct MoveStack {
    Move m_PV[MAX_DEPTH] = {};
    int m_Ply;
    int m_Eval = NONE_SCORE;
};

struct RootMove {
    int score;
    Move move;
    bool operator<(const RootMove& m) const {
        return m.score < score;
    }
};

enum NodeType {
    ROOT,
    PV,
    NON_PV
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

    template<NodeType node>
    int64 Quiesce(Position& board, MoveStack* stack, int64 alpha, int64 beta, int depth) {
        constexpr bool PVNode = node != NON_PV;
        m_NodeCnt++;

        // Check for repetition
        for (int i = 4; i < board.m_States[board.m_Ply].m_HalfMoves && i < board.m_Ply; i += 2) {
            if (board.m_States[board.m_Ply - i].m_Hash == board.m_Hash) {
                return 0;
            }
        }

        // Probe Transposition table
        Move hashMove = Move();
        bool ttPV = PVNode;
        TTEntry* entry = m_Table->Probe(board.m_Hash);
        if (entry != nullptr) {
            if (!PVNode && entry->m_Depth >= depth && (entry->m_Bound & (entry->m_Score >= beta ? LOWER_BOUND : UPPER_BOUND))) {
                return entry->m_Score;
            }
            hashMove = entry->m_BestMove;
            ttPV |= entry->m_PV;
        }

        int64 bestScore = Evaluate(board, m_PawnTable);
        stack->m_Eval = bestScore;
        if (bestScore >= beta) { // Return if we fail soft
            return bestScore;
        }
        if (alpha < bestScore) {
            alpha = bestScore;
        }

        Move bestMove = 0;
        int movecnt = 0;

        MoveGen moveGen(board, hashMove, true);
        Move move;
        while ((move = moveGen.Next()) != 0) {
            movecnt++;
            if (CaptureType(move) == ColoredPieceType::NOPIECE) continue; // Only analyze capturing moves

            board.MovePiece(move);
            int64 score = -Quiesce<node>(board, stack + 1, -beta, -alpha, depth - 1);
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
                if(mate.size() == 0) bestScore = -MATE_SCORE + stack->m_Ply;
            } else { // Commented out because it yields better performance without it even though it may not be correct
                //std::vector<Move> mate = GenerateMoves<ALL>(board);
                //if (mate.size() == 0) bestScore = 0;
            }
        }

        
        m_Table->Enter(board.m_Hash,
            TTEntry(board.m_Hash,
                bestMove,
                bestScore,
                bestScore >= beta ? LOWER_BOUND : PVNode ? EXACT_BOUND : UPPER_BOUND,
                stack->m_Ply,
                depth,
                board.m_FullMoves,
                ttPV
            ));
        

        return bestScore;
    }

    template<NodeType node>
	int64 AlphaBeta(Position& board, MoveStack* stack, int64 alpha, int64 beta, int depth, bool cutNode) {

        constexpr bool PVNode = node != NON_PV;
        constexpr bool rootNode = node == ROOT;

        m_NodeCnt++;
        if (!m_Running || m_Timer.EndMs() >= m_MaxTime) return 0;

        // Quiesce search if we reached the bottom
        if (depth <= 0) {
            return Quiesce<node>(board, stack, alpha, beta, depth);
        }

        if (!rootNode) {
            // We count any repetion as draw
            // TODO: Maybe employ hashing to make this O(1) instead of O(n)
            for (int i = 4; i < board.m_States[board.m_Ply].m_HalfMoves && i < board.m_Ply; i += 2) {
                if (board.m_States[board.m_Ply - i].m_Hash == board.m_Hash) {
                    return 0;
                }
            }
        }

        // Prevent explosions
        assert("Ply is more than MAX_DEPTH!\n", stack->m_Ply >= MAX_DEPTH);
        depth = std::min(depth, MAX_DEPTH - 1);


        // Probe Transposition table
        Move hashMove = Move();
        TTEntry* entry = m_Table->Probe(board.m_Hash);
        int64 ttScore = NONE_SCORE;
        int ttDepth;
        bool ttPV = PVNode;
        if (entry != nullptr) {
            // Check for TT cutoff
            if (!PVNode && entry->m_Depth >= depth && (entry->m_Bound & (entry->m_Score >= beta ? LOWER_BOUND : UPPER_BOUND))) {
                return entry->m_Score;
            }
            hashMove = entry->m_BestMove;
            ttScore  = entry->m_Score;
            ttDepth  = entry->m_Depth;
            ttPV    |= entry->m_PV;
        }

        // Probe the tablebase
        if (!rootNode) {
            int success;
            //printf("%s\n", board.ToFen().c_str());
            int v = TableBase::Probe_DTZ(board, &success);
            if (success) {
                int value = Signum(v) * (MATE_SCORE - v);
                // TODO: Store value in hashtable
                if(!entry) m_Table->Enter(board.m_Hash,
                    TTEntry(board.m_Hash,
                        0, // No move
                        value,
                        v > 0 ? LOWER_BOUND : v < 0 ? UPPER_BOUND : EXACT_BOUND,
                        stack->m_Ply,
                        depth,
                        board.m_FullMoves,
                        ttPV
                    ));
                return value;
            }
        }

        int64 staticEval = Evaluate(board, m_PawnTable);
        stack->m_Eval = staticEval;
        bool improving = false;

        if (!board.m_InCheck && stack->m_Ply >= 2) {
            improving = stack->m_Eval > (stack - 2)->m_Eval;
        } else if (stack->m_Ply >= 4) {
            improving = stack->m_Eval > (stack - 4)->m_Eval;
        }
        
		int64 bestScore = -MATE_SCORE;
        int64 old_alpha = alpha;
        Move bestMove = 0;

        int64 score = bestScore;

        int movecnt = 0;

        MoveGen moveGen(board, hashMove, false);
        Move move;
		while ((move = moveGen.Next()) != 0) {
            movecnt++;
            int newDepth = depth - 1;
            int reduction = 0, extension = 0;
            int delta = beta - alpha;
            bool capture = CaptureType(move) != NOPIECE;
            // Singular extension
            if (!rootNode && stack->m_Ply < 2 * m_Maxdepth && depth >= 6 && move == hashMove && ttScore < MATE_SCORE && (entry->m_Bound & LOWER_BOUND)) {
                int64 singularBeta = ttScore - depth;
                int64 extScore = -AlphaBeta<NON_PV>(board, stack + 1, singularBeta - 1, singularBeta, newDepth / 2, cutNode);
                if (extScore < singularBeta) {
                    extension = 1;
                } else if (singularBeta >= beta) { // Multi-cut pruning
                    return singularBeta;
                } else if (ttScore >= beta) { // Negative extension
                    extension = -2 + PVNode;
                } else if (cutNode) { // Expected cutnode is unlikely to be good
                    extension = -2;
                }
            }

            board.MovePiece(move);

            newDepth += extension;

            // Late move reduction
            if (depth >= 2 && movecnt > 1 + rootNode) {
                //reduction = ((375 + 220 * std::log(depth) * std::log(movecnt)) / (1 + capture)) / 1000;
                reduction = ((500 + 400 * std::log(depth) * std::log(movecnt)) / (1 + capture)) / 1000;
                // Extend checks
                if (board.m_InCheck && stack->m_Ply < MAX_DEPTH) reduction -= 1;

                // If we are on pv node then decrease reduction
                if (PVNode) reduction -= 1;
                if (ttPV) reduction -= 1;

                //if (CaptureType(hashMove) != NOPIECE) reduction += 1;

                // If hash move is not found then increase reduction
                if (hashMove == 0) reduction += 1;

                // Reduce if we are not improving
                if (!improving) reduction += 1;

                // Reduce expected cut node
                if (cutNode) reduction += 1;

                int reducedDepth = std::min(std::max(1, newDepth - reduction), newDepth+1);
                score = -AlphaBeta<NON_PV>(board, stack + 1, -alpha-1, -alpha, reducedDepth, true);
                if (score > alpha && reducedDepth < newDepth) {
                    // newdepth is different so search it again att full depth
                    if (reducedDepth < newDepth) {
                        score = -AlphaBeta<NON_PV>(board, stack + 1, -alpha - 1, -alpha, newDepth, !cutNode);
                    }
                }
            } else if (!PVNode || movecnt > 1) {
                score = -AlphaBeta<NON_PV>(board, stack + 1, -alpha - 1, -alpha, newDepth, !cutNode);
            }

            if (PVNode && (movecnt == 1 || score > alpha)) {
                score = -AlphaBeta<PV>(board, stack + 1, -beta, -alpha, newDepth, false);
            }

            
            board.UndoMove(move);
			if (score > bestScore) {
				bestScore = score;
                bestMove = move;
				if (score > alpha) {
                    if (PVNode) {
                        Update_PV(stack->m_PV, bestMove, (stack + 1)->m_PV);
                    }
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
                bestScore = -MATE_SCORE + stack->m_Ply;
            } else {
                bestScore = 0;
            }
        }

        m_Table->Enter(board.m_Hash, 
            TTEntry(board.m_Hash, 
            bestMove, 
            bestScore, 
            bestScore >= beta ? LOWER_BOUND : PVNode ? EXACT_BOUND : UPPER_BOUND, 
            stack->m_Ply, 
            depth, 
            board.m_FullMoves,
            ttPV
        ));

		return bestScore;
	}

    

    // Calculate time to allocate for a move
    void MoveTimed(int64 wtime, int64 btime, int64 winc, int64 binc) {
        int64 timediff = llabs(wtime - btime);

        bool moreTime = m_Position.m_WhiteMove ? wtime > btime : wtime < btime;
        int64 timeleft = m_Position.m_WhiteMove ? wtime : btime;
        int64 timeinc = m_Position.m_WhiteMove ? winc : binc;

        int64 est_movesleft = std::max(60 - (int64)m_Position.m_FullMoves, 20ll);
        int64 est_timeleft = timeleft + est_movesleft * timeinc;

        int64 target = std::max(std::min(est_timeleft / est_movesleft - 20, timeleft/2), 20ll); // Don't let the time run out and - overhead
        float x = (m_Position.m_FullMoves - 20.0f)/30.0f;
        float factor = exp(-x*x);   // Bell curve
        int64 result = (target * factor);
        sync_printf("info movetime %lli\n", result);
        UCIMove(result);
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
        m_Timer.Start();

        m_Maxdepth = 0;

        m_NodeCnt = 1;

        //MoveStack movestack[64] = {};
        MoveStack* stack = new MoveStack[MAX_DEPTH];

        for (int i = 0; i < MAX_DEPTH; i++) {
            (stack + i)->m_Ply = i;
        }
        
        // Start timer
        Move bestMove = 0;
        Move finalMove = bestMove;
        int64 bestScore = -MATE_SCORE;
        int64 rootAlpha = MIN_ALPHA;
        int64 rootBeta = MAX_BETA;
        m_RootDelta = 10;
        // Iterative deepening
        while (m_Running && m_Timer.EndMs() < m_MaxTime && MAX_DEPTH > m_Maxdepth) {
            bestScore = -MATE_SCORE;

            m_RootDelta = 10;
            rootAlpha = MIN_ALPHA;
            rootBeta = MAX_BETA;

            int64 alpha, beta;

            m_Maxdepth++;

            TTEntry* entry = m_Table->Probe(m_Position.m_Hash);
            if (entry != nullptr) {
                rootAlpha = entry->m_Score - m_RootDelta;
                rootBeta = entry->m_Score + m_RootDelta;
            }
            int failHigh = 0;

            // Aspiration window
            while (true) {
                alpha = rootAlpha;
                beta = rootBeta;
                bestScore = AlphaBeta<ROOT>(m_Position, stack, rootAlpha, rootBeta, std::max(1, m_Maxdepth - failHigh), false);

                if (bestScore <= rootAlpha) { // Failed low
                    rootBeta = (rootAlpha + rootBeta) / 2;
                    rootAlpha = std::max(bestScore - m_RootDelta, MIN_ALPHA);
                    failHigh = 0;
                }
                else if (bestScore >= rootBeta) { // Failed high
                    rootBeta = std::min(bestScore + m_RootDelta, MAX_BETA);
                    failHigh++;
                } else {
                    break; // We found good bounds so exit out
                }

                m_RootDelta *= 1.5;
            }
            if (!m_Running || m_Timer.EndMs() >= m_MaxTime) break;

            // Print pv and search info
            sync_printf("info depth %i score cp %lli time %lli nodes %llu tps %llu\n", m_Maxdepth, bestScore, (int64)m_Timer.EndMs(), m_NodeCnt, (uint64)(m_NodeCnt / m_Timer.End()));
            std::ostringstream oss;
            oss << "info pv";
            for (int i = 0; i < MAX_DEPTH && stack->m_PV[i] != Move(); i++) {
                oss << " " << MoveToString(stack->m_PV[i]);
            }
            oss << "\n";
            sync_printf("%s", oss.str().c_str());

            finalMove = stack->m_PV[0];
            
            m_Table->Enter(m_Position.m_Hash,
                TTEntry(m_Position.m_Hash,
                    bestMove,
                    bestScore,
                    bestScore >= beta ? LOWER_BOUND : EXACT_BOUND,
                    0,
                    m_Maxdepth,
                    m_Position.m_FullMoves,
                    true
                ));
            
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

