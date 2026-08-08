#pragma once

#include "Transposition.h"
#include "Evaluate.h"
#include "TableBase.h"
#include "MoveGen.h"

#include <unordered_map>
#include <algorithm>
#include <cinttypes>
#include <atomic>
#include <vector>
#include <thread>
#include <cmath>

#define MAX_DEPTH       64 // Maximum depth that the engine will go
#define MATE_SCORE      32767 / 2
#define NONE_SCORE      32766
#define MIN_ALPHA       int64(-32767)
#define MAX_BETA        int64(32767)
#define DEFAULT_HASH_MB 16ull
#define PAWN_TABLE_MB   1ull

// Quadratic https://www.chessprogramming.org/Triangular_PV-Table
struct MoveStack {
    Move m_PV[MAX_DEPTH] = {};
    int m_Ply = 0;
    int m_Eval = NONE_SCORE;
    Move m_CurrentMove = 0; // 0 also marks a null move, so the child won't null move again
};

struct RootMove {
    int score;
    Move move;
    bool operator<(const RootMove& m) const { return m.score < score; }
};

enum NodeType { ROOT, PV, NON_PV };

struct SearchLimits {
    int maxDepth = MAX_DEPTH;
    uint64 maxNodes = 0;  // 0 = unlimited
    int64 maxTimeMs = -1; // -1 = unlimited
    bool useTablebase = true;
    bool silent = false; // Skip the per depth info lines, for searches nobody is watching
};

struct SearchResult {
    Move best = 0;
    int64 score = 0;
    uint64 nodes = 0;
    int depth = 0;
};

class Search {
public:
    Position m_Position;

private:
    //uint64 m_Hash[MAX_DEPTH];
    //uint64 m_PawnHash[MAX_DEPTH];
    //uint64 m_History[256];
    std::vector<std::unique_ptr<std::thread>> m_Threads;
    std::atomic<bool> m_Running;
    std::atomic<bool> m_Stopping;
    Timer m_Timer;
    int m_Maxdepth; // Maximum depth currently set
    uint64 m_NodeCnt;
    std::unique_ptr<TranspositionTable> m_Table;
    std::unique_ptr<PawnTable> m_PawnTable;
    std::vector<RootMove> m_RootMoves;
    int m_RootDelta;
    SearchLimits m_Limits;

public:
    Search(uint64 hashMB = DEFAULT_HASH_MB) : m_Maxdepth(0), m_Running(false), m_Stopping(false), m_NodeCnt(0) {
        m_Table = std::make_unique<TranspositionTable>(hashMB * 1024 * 1024);
        m_PawnTable = std::make_unique<PawnTable>(PAWN_TABLE_MB * 1024 * 1024);
        LoadPosition(Lookup::starting_pos);
    }

    ~Search() { Stop(); }

    void SetHashSize(uint64 hashMB) {
        Stop();
        m_Table->Resize(hashMB * 1024 * 1024);
    }

    void JoinThreads() {
        for (std::unique_ptr<std::thread>& t : m_Threads) {
            if (t->joinable()) {
                t->join();
            }
        }
        m_Threads.clear();
    }

    void Stop() {
        m_Stopping = true; // A search that has not entered Go() yet would miss m_Running
        m_Running = false;
        JoinThreads();
        m_Stopping = false;
    }

    bool TimeExpired() { return m_Limits.maxTimeMs >= 0 && m_Timer.EndMs() >= m_Limits.maxTimeMs; }

    bool ShouldStop() {
        if (!m_Running || m_Stopping) return true;
        if (m_Limits.maxNodes && m_NodeCnt >= m_Limits.maxNodes) return true;
        return TimeExpired();
    }

    void ClearTables() {
        m_Table->Clear();
        m_PawnTable->Clear();
    }

    void LoadPosition(std::string fen) { m_Position.SetPosition(fen); }

    static void Update_PV(Move* pv, Move move, Move* target) {
        for (*pv++ = move; target && *target != Move();) *pv++ = *target++;
        *pv = Move();
    }

    template<NodeType node>
    int64 Quiesce(Position& board, MoveStack* stack, int64 alpha, int64 beta, int depth) {
        constexpr bool PVNode = node != NON_PV;

        // Quiescence does not build a PV, so the line ends here.
        if (PVNode) stack->m_PV[0] = Move();

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
            if (!PVNode && entry->m_Depth >= depth
                && (entry->m_Bound & (entry->m_Score >= beta ? LOWER_BOUND : UPPER_BOUND))) {
                return entry->m_Score;
            }
            hashMove = entry->m_BestMove;
            ttPV |= entry->m_PV;
        }

        int64 bestScore = Evaluate(board, m_PawnTable.get());
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
                std::vector<Move> mate = GenerateMoves<ALL>(
                    board); // TODO: Generate evasions in Quiescense which will give mate or draw if movecnt == 0
                if (mate.size() == 0) bestScore = -MATE_SCORE + stack->m_Ply;
            } else { // Commented out because it yields better performance without it even though it may not be correct
                //std::vector<Move> mate = GenerateMoves<ALL>(board);
                //if (mate.size() == 0) bestScore = 0;
            }
        }


        m_Table->Enter(board.m_Hash, TTEntry(board.m_Hash, bestMove, bestScore,
                                             bestScore >= beta ? LOWER_BOUND
                                             : PVNode          ? EXACT_BOUND
                                                               : UPPER_BOUND,
                                             stack->m_Ply, depth, board.m_FullMoves, ttPV));


        return bestScore;
    }

    template<NodeType node>
    int64 AlphaBeta(Position& board, MoveStack* stack, int alpha, int beta, int depth, bool cutNode,
                    Move excluded = 0) {
        constexpr bool PVNode = node != NON_PV;
        constexpr bool rootNode = node == ROOT;

        // The parent copies this PV once the node returns, so it must not still
        // hold a line from an unrelated subtree.
        if (PVNode) stack->m_PV[0] = Move();

        m_NodeCnt++;
        if (ShouldStop()) return 0;

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
        assert(stack->m_Ply < MAX_DEPTH && "Ply is more than MAX_DEPTH!");
        depth = std::min(depth, MAX_DEPTH - 1);


        // Probe Transposition table.
        Move hashMove = Move();
        TTEntry* entry = excluded == 0 ? m_Table->Probe(board.m_Hash) : nullptr;
        int64 ttScore = NONE_SCORE;
        int ttDepth = 0;
        Bound ttBound = NO_BOUND;
        bool ttPV = PVNode;
        if (entry != nullptr) {
            // Check for TT cutoff
            if (!PVNode && entry->m_Depth >= depth
                && (entry->m_Bound & (entry->m_Score >= beta ? LOWER_BOUND : UPPER_BOUND))) {
                return entry->m_Score;
            }
            hashMove = entry->m_BestMove;
            ttScore = entry->m_Score;
            ttDepth = entry->m_Depth;
            ttBound = entry->m_Bound;
            ttPV |= entry->m_PV;
        }

        // Probe the tablebase. It scores the position, which excluding a move does not change
        if (!rootNode && excluded == 0 && m_Limits.useTablebase) {
            int success;
            //printf("%s\n", board.ToFen().c_str());
            int v = TableBase::Probe_DTZ(board, &success);
            if (success) {
                int value = Signum(v) * (MATE_SCORE - v);
                // TODO: Store value in hashtable
                if (!entry)
                    m_Table->Enter(board.m_Hash, TTEntry(board.m_Hash,
                                                         0, // No move
                                                         value,
                                                         v > 0   ? LOWER_BOUND
                                                         : v < 0 ? UPPER_BOUND
                                                                 : EXACT_BOUND,
                                                         stack->m_Ply, depth, board.m_FullMoves, ttPV));
                return value;
            }
        }

        int64 staticEval = Evaluate(board, m_PawnTable.get());
        stack->m_Eval = staticEval;
        bool improving = false;

        if (!board.m_InCheck && stack->m_Ply >= 2) {
            improving = stack->m_Eval > (stack - 2)->m_Eval;
        } else if (stack->m_Ply >= 4) {
            improving = stack->m_Eval > (stack - 4)->m_Eval;
        }

        // Null move pruning
        // Never null move while in check (the reply could capture our king) and never twice in a row,
        // which we signal by leaving its m_CurrentMove at 0. Passing is also a bad idea in
        // zugzwang, so require a piece on the board.
        if (!PVNode && !board.m_InCheck && excluded == 0 && stack->m_Ply >= 1 && (stack - 1)->m_CurrentMove != 0
            && stack->m_Eval >= beta && stack->m_Eval + 40 * depth - 200 >= beta && board.HasNonPawnMaterial()) {
            // The margin is in centipawns, so scale it before spending it as plies
            int reduction = (int)std::min<int64>((stack->m_Eval - beta) / 200, 6) + depth / 3 + 4;
            stack->m_CurrentMove = 0;
            board.NullMove();
            int nullscore = -AlphaBeta<NON_PV>(board, stack + 1, -beta, -beta + 1, depth - reduction, !cutNode);
            board.UndoNullMove();
            if (nullscore >= beta) {
                // A mate found by passing a move is not a real mate
                return nullscore >= MATE_SCORE - MAX_DEPTH ? beta : nullscore;
            }
        }

        int64 bestScore = -MATE_SCORE;
        int64 old_alpha = alpha;
        Move bestMove = 0;

        int64 score = bestScore;

        int movecnt = 0;

        MoveGen moveGen(board, hashMove, false);
        Move move;
        while ((move = moveGen.Next()) != 0) {
            if (move == excluded) continue;
            movecnt++;
            stack->m_CurrentMove = move;
            int newDepth = depth - 1;
            int reduction = 0, extension = 0;
            int delta = beta - alpha;
            bool capture = CaptureType(move) != NOPIECE;
            // Singular extension. Re-searches this node without the hash move, so it runs before the
            // move is made and is not negated: the score is ours, not the opponent's reply.
            if (!rootNode && excluded == 0 && stack->m_Ply < 2 * m_Maxdepth && depth >= 6 && move == hashMove
                && (ttBound & LOWER_BOUND) && ttDepth >= depth - 3 && ttScore < MATE_SCORE - MAX_DEPTH
                && ttScore > -MATE_SCORE + MAX_DEPTH) {
                int64 singularBeta = ttScore - depth;
                // The child reuses this ply's stack slot
                int64 savedEval = stack->m_Eval;
                Move savedMove = stack->m_CurrentMove;
                int64 singularScore =
                    AlphaBeta<NON_PV>(board, stack, singularBeta - 1, singularBeta, (depth - 1) / 2, cutNode, move);
                stack->m_Eval = savedEval;
                stack->m_CurrentMove = savedMove;

                if (singularScore < singularBeta) {
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

                int reducedDepth = std::min(std::max(1, newDepth - reduction), newDepth + 1);
                score = -AlphaBeta<NON_PV>(board, stack + 1, -alpha - 1, -alpha, reducedDepth, true);
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
            if (excluded != 0) {
                bestScore = alpha; // The excluded move was the only one
            } else if (board.m_InCheck) {
                bestScore = -MATE_SCORE + stack->m_Ply;
            } else {
                bestScore = 0;
            }
        }

        if (excluded == 0) {
            m_Table->Enter(board.m_Hash, TTEntry(board.m_Hash, bestMove, bestScore,
                                                 bestScore >= beta ? LOWER_BOUND
                                                 : PVNode          ? EXACT_BOUND
                                                                   : UPPER_BOUND,
                                                 stack->m_Ply, depth, board.m_FullMoves, ttPV));
        }

        return bestScore;
    }


    // Calculate time to allocate for a move
    void MoveTimed(int64 wtime, int64 btime, int64 winc, int64 binc, int depth = MAX_DEPTH) {
        int64 timediff = llabs(wtime - btime);

        bool moreTime = m_Position.m_WhiteMove ? wtime > btime : wtime < btime;
        int64 timeleft = m_Position.m_WhiteMove ? wtime : btime;
        int64 timeinc = m_Position.m_WhiteMove ? winc : binc;

        int64 est_movesleft = std::max(60 - (int64)m_Position.m_FullMoves, int64(20));
        int64 est_timeleft = timeleft + est_movesleft * timeinc;

        int64 target = std::max(std::min(est_timeleft / est_movesleft - 20, timeleft / 2),
                                int64(20)); // Don't let the time run out and - overhead
        float x = (m_Position.m_FullMoves - 20.0f) / 30.0f;
        float factor = 0.5 + 0.5 * exp(-x * x); // const + Bell curve, bounded [0.5,1]
        int64 result = (target * factor);
        sync_printf("info movetime %" PRId64 "\n", result);
        UCIMove(result, depth);
    }

    void UCIMove(int64 time, int depth = MAX_DEPTH) {
        SearchLimits limits;
        limits.maxTimeMs = time;
        limits.maxDepth = std::clamp(depth, 1, (int)MAX_DEPTH);
        StartAsync(limits);
    }

    void StartAsync(const SearchLimits& limits) {
        JoinThreads();
        m_Limits = limits;
        m_Threads.push_back(std::make_unique<std::thread>(&Search::UCIMove_async, &*this));
    }

    void UCIMove_async() {
        SearchResult result = Go(m_Limits);
        sync_printf("bestmove %s\n", result.best != Move() ? MoveToString(result.best).c_str() : "0000");
    }

    SearchResult Go(const SearchLimits& limits) {
        m_Limits = limits;
        m_Running = true;
        m_Timer.Start();

        m_Maxdepth = 0;

        m_NodeCnt = 1;

        //MoveStack movestack[64] = {};
        MoveStack* stack = new MoveStack[MAX_DEPTH];

        for (int i = 0; i < MAX_DEPTH; i++) {
            (stack + i)->m_Ply = i;
        }

        const int depthCap = std::min(m_Limits.maxDepth, (int)MAX_DEPTH);

        // Start timer
        Move finalMove = 0;
        int64 bestScore = -MATE_SCORE;
        int64 rootAlpha = MIN_ALPHA;
        int64 rootBeta = MAX_BETA;
        m_RootDelta = 10;
        // Iterative deepening
        while (!ShouldStop() && depthCap > m_Maxdepth) {
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
                bestScore =
                    AlphaBeta<ROOT>(m_Position, stack, rootAlpha, rootBeta, std::max(1, m_Maxdepth - failHigh), false);

                if (bestScore <= rootAlpha) { // Failed low
                    rootBeta = (rootAlpha + rootBeta) / 2;
                    rootAlpha = std::max(bestScore - m_RootDelta, MIN_ALPHA);
                    failHigh = 0;
                } else if (bestScore >= rootBeta) { // Failed high
                    rootBeta = std::min(bestScore + m_RootDelta, MAX_BETA);
                    failHigh++;
                } else {
                    break; // We found good bounds so exit out
                }

                m_RootDelta *= 1.5;
            }
            if (ShouldStop()) break;

            // Print pv and search info
            if (!m_Limits.silent) {
                sync_printf("info depth %i score cp %" PRId64 " time %" PRId64 " nodes %" PRIu64 " tps %" PRIu64 "\n",
                            m_Maxdepth, bestScore, (int64)m_Timer.EndMs(), m_NodeCnt,
                            (uint64)(m_NodeCnt / m_Timer.End()));
                std::ostringstream oss;
                oss << "info pv";
                for (int i = 0; i < MAX_DEPTH && stack->m_PV[i] != Move(); i++) {
                    oss << " " << MoveToString(stack->m_PV[i]);
                }
                oss << "\n";
                sync_printf("%s", oss.str().c_str());
            }

            if (stack->m_PV[0] != Move()) finalMove = stack->m_PV[0];

            m_Table->Enter(m_Position.m_Hash, TTEntry(m_Position.m_Hash, finalMove, bestScore,
                                                      bestScore >= beta ? LOWER_BOUND : EXACT_BOUND, 0, m_Maxdepth,
                                                      m_Position.m_FullMoves, true));

            if (m_Limits.maxTimeMs >= 0 && m_Timer.EndMs() * 2 >= m_Limits.maxTimeMs)
                break; // We won't have enough time to calculate more depth anyway
            if (bestScore >= MATE_SCORE - MAX_DEPTH || bestScore <= -MATE_SCORE + MAX_DEPTH)
                break; // Position is solved so exit out
        }
        delete[] stack;

        m_Running = false;

        if (finalMove == Move()) { // Stopped before a single depth finished, any legal move beats none
            std::vector<Move> moves = GenerateMoves<ALL>(m_Position);
            if (!moves.empty()) finalMove = moves[0];
        }

        SearchResult result;
        result.best = finalMove;
        result.score = bestScore;
        result.nodes = m_NodeCnt;
        result.depth = m_Maxdepth;
        return result;
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

        danger |= Lookup::king_attacks[GetSquare(King<enemy>(board))];

        return danger;
    }

    Move GetMove(std::string str) { return m_Position.m_WhiteMove ? TGetMove<WHITE>(str) : TGetMove<BLACK>(str); }

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
            case 'n': flags |= 0b100; break;
            case 'b': flags |= 0b1000; break;
            case 'r': flags |= 0b10000; break;
            case 'q': flags |= 0b100000; break;
            }
        }
        if (Pawn<white>(m_Position) & from) {
            type = GetColoredPiece<white>(PAWN);
            if (m_Position.m_States[m_Position.m_Ply].m_EnPassant & to) {
                flags |= 0b1;
                capture = GetColoredPiece<!white>(PAWN);
            }
        } else if (Knight<white>(m_Position) & from) {
            type = GetColoredPiece<white>(KNIGHT);
        } else if (Bishop<white>(m_Position) & from) {
            type = GetColoredPiece<white>(BISHOP);
        } else if (Rook<white>(m_Position) & from) {
            type = GetColoredPiece<white>(ROOK);
        } else if (Queen<white>(m_Position) & from) {
            type = GetColoredPiece<white>(QUEEN);
        } else if (King<white>(m_Position) & from) {
            type = GetColoredPiece<white>(KING);
            if (((from & 0b1000) && (to & 0b10)) || ((from & (0b1000ull << 56)) && (to & (0b10ull << 56)))
                || ((from & 0b1000) && (to & 0b100000)) || ((from & (0b1000ull << 56)) && (to & (0b100000ull << 56)))) {
                flags |= 0b10;
            }
        }


        assert(type != NOPIECE && "Could not read move");
        return fromPos | toPos << 6 | type << 12 | capture << 16 | flags << 20;
    }

    std::string GetFen() const { return m_Position.ToFen(); }
};
