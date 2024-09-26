#pragma once

#include "Transposition.h"

#include <unordered_map>
#include <algorithm>
#include <vector>
#include <thread>
#include <cmath>

#define MAX_DEPTH 64 // Maximum depth that the engine will go
#define MATE_SCORE 32767


class Search {
public:
	BoardInfo m_Info[64];
	std::unique_ptr<Board> m_RootBoard;
    int64 m_MaxTime;
private:
	uint64 m_Hash[64];
    std::vector<std::unique_ptr<std::thread>> m_Threads;
    bool m_Running;
    Timer m_Timer;
	int m_Maxdepth; // Maximum depth currently set
    uint64 m_NodeCnt;
    TranspositionTable* m_Table;
public:

	Search() 
        : m_Maxdepth(0), m_Running(false), m_MaxTime(999999999999999), m_NodeCnt(0)
	{
        m_Table = new TranspositionTable(1024 * 1024 * 1024);
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
        m_Table->Clear();
	}

	int64 Evaluate(const Board& board) {

		int64 middlegame = 0;
        int64 endgame = 0;

		uint64 wp = board.m_WhitePawn, wkn = board.m_WhiteKnight, wb = board.m_WhiteBishop, wr = board.m_WhiteRook, wq = board.m_WhiteQueen, wk = board.m_WhiteKing,
			bp = board.m_BlackPawn, bkn = board.m_BlackKnight, bb = board.m_BlackBishop, br = board.m_BlackRook, bq = board.m_BlackQueen, bk = board.m_BlackKing;

		const int64 pawnVal = 100, knightVal = 350, bishopVal = 350, rookVal = 525, queenVal = 1000, kingVal = 10000;

        int64 phase = 4 + 4 + 8 + 8;

        int whiteking = GET_SQUARE(wk), blackking = GET_SQUARE(bk);

        int whiteAttack = 0, blackAttack = 0;

		// Pawns
		while (wp > 0) {
            int rpos = PopPos(wp);
			int pos = 63 - rpos;
			middlegame += pawnVal + Lookup::pawn_table[pos];
            endgame    += pawnVal + Lookup::eg_pawn_table[pos];
            if ((Lookup::white_passed[rpos] & board.m_BlackPawn) == 0) { // pawn is passed
                endgame += 100;
                middlegame += 40;
            }
		}

		while (bp > 0) {
			int pos = PopPos(bp);
			middlegame -= pawnVal + Lookup::pawn_table[pos];
            endgame    -= pawnVal + Lookup::eg_pawn_table[pos];
            if ((Lookup::black_passed[pos] & board.m_WhitePawn) == 0) { // pawn is passed
                endgame -= 100;
                middlegame -= 40;
            }
		}

		// Knights
		while (wkn > 0) {
            int rpos = PopPos(wkn);
			int pos = 63 - rpos;
			middlegame += knightVal + Lookup::knight_table[pos];
            endgame    += knightVal + Lookup::knight_table[pos];
            if (uint64 temp = Lookup::b_king_safety[blackking] & Lookup::knight_attacks[rpos]) {
                whiteAttack += 2 * COUNT_BIT(temp);
            }
            phase -= 1;
		}

		while (bkn > 0) {
			int pos = PopPos(bkn);
			middlegame -= knightVal + Lookup::knight_table[pos];
            endgame    -= knightVal + Lookup::knight_table[pos];
            if (uint64 temp = Lookup::w_king_safety[whiteking] & Lookup::knight_attacks[pos]) {
                blackAttack += 2 * COUNT_BIT(temp);
            }
            phase -= 1;
		}

		// Bishops
		while (wb > 0) {
            int rpos = PopPos(wb);
			int pos = 63 - rpos;
			middlegame += bishopVal + Lookup::bishop_table[pos];
            endgame    += bishopVal + Lookup::bishop_table[pos];
            uint64 bish_atk = board.BishopAttack(rpos, board.m_Board);
            if (uint64 temp = Lookup::b_king_safety[blackking] & bish_atk) {
                whiteAttack += 2 * COUNT_BIT(temp);
            }
            phase -= 1;
		}

		while (bb > 0) {
			int pos = PopPos(bb);
			middlegame -= bishopVal + Lookup::bishop_table[pos];
            endgame    -= bishopVal + Lookup::bishop_table[pos];
            uint64 bish_atk = board.BishopAttack(pos, board.m_Board);
            if (uint64 temp = Lookup::w_king_safety[whiteking] & bish_atk) {
                blackAttack += 2 * COUNT_BIT(temp);
            }
            phase -= 1;
		}

		// Rooks
		while (wr > 0) {
            int rpos = PopPos(wr);
			int pos = 63 - rpos;
			middlegame += rookVal + Lookup::rook_table[pos];
            endgame    += rookVal + Lookup::eg_rook_table[pos];
            uint64 rook_atk = board.RookAttack(rpos, board.m_Board);
            if (uint64 temp = Lookup::b_king_safety[blackking] & rook_atk) {
                whiteAttack += 3 * COUNT_BIT(temp);
            }
            phase -= 2;
		}

		while (br > 0) {
			int pos = PopPos(br);
			middlegame -= rookVal + Lookup::rook_table[pos];
            endgame    -= rookVal + Lookup::eg_rook_table[pos];
            uint64 rook_atk = board.RookAttack(pos, board.m_Board);
            if (uint64 temp = Lookup::w_king_safety[whiteking] & rook_atk) {
                blackAttack += 3 * COUNT_BIT(temp);
            }
            phase -= 2;
		}

        // Queens
		while (wq > 0) {
            int rpos = PopPos(wq);
			int pos = 63 - rpos;
			middlegame += queenVal + Lookup::queen_table[pos];
            endgame    += queenVal + Lookup::eg_queen_table[pos];
            uint64 queen_atk = board.QueenAttack(rpos, board.m_Board);
            if (uint64 temp = Lookup::b_king_safety[blackking] & queen_atk) {
                whiteAttack += 5 * COUNT_BIT(temp);
            }
            phase -= 4;
		}

		while (bq > 0) {
			int pos = PopPos(bq);
			middlegame -= queenVal + Lookup::queen_table[pos];
            endgame    -= queenVal + Lookup::eg_queen_table[pos];
            phase -= 4;
            uint64 queen_atk = board.QueenAttack(pos, board.m_Board);
            if (uint64 temp = Lookup::king_attacks[whiteking] & queen_atk) {
                blackAttack += 5 * COUNT_BIT(temp);
            }
		}

		if (wk > 0) {
			int pos = 63 - whiteking;
			middlegame += kingVal + Lookup::king_table[pos] + Lookup::king_safetyindex[whiteAttack];
            endgame    += kingVal + Lookup::eg_king_table[pos];
		}

		if (bk > 0) {
		    middlegame -= kingVal + Lookup::king_table[blackking] + Lookup::king_safetyindex[blackAttack];
            endgame    -= kingVal + Lookup::eg_king_table[blackking];
		}
        
        phase = (phase * 256) / 16;
		return (middlegame * (256-phase) + endgame * phase) / 256;
	}



    // Simple Most Valuable Victim - Least Valuable Aggressor sorting function
    static bool Move_Order(const Move& a, const Move& b) {
        int64 score_a = 0;
        int64 score_b = 0;
        if ((int)a.m_Capture) score_a += 9 + Lookup::pieceValue[(int)a.m_Capture] - Lookup::pieceValue[(int)a.m_Type];
        if ((int)b.m_Capture) score_b += 9 + Lookup::pieceValue[(int)b.m_Capture] - Lookup::pieceValue[(int)b.m_Type];

        return score_a > score_b;
    }

	uint64 Perft_r(const Board& board, int depth) {
        if (!m_Running) return 0;
        assert("Hash is not matched!", Zobrist_Hash(board, m_Info[depth]) != m_Hash[depth]);
		const std::vector<Move> moves = GenerateMoves(board, depth);
		if (depth == m_Maxdepth-1) return moves.size();
		int64 result = 0;
		for (const Move& move : moves) {
			int64 count = Perft_r(MovePiece(board, move, depth + 1), depth + 1);
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
        uint64 result = Perft_r(*m_RootBoard, 0);
		float t = time.End();
        if(m_Running) sync_printf("%llu\n%.3fMNodes/s\n", result, (result / 1000000.0f) / t);
        m_Running = false;
		return result;
	}

    int64 Quiesce(const Board& board, int64 alpha, int64 beta, int depth) {
        m_NodeCnt++;
        int64 sgn = m_Info[depth].m_WhiteMove ? 1 : -1;
        int64 bestScore = sgn * Evaluate(board);
        if (bestScore >= beta) {
            return bestScore;
        }
        if (alpha < bestScore) {
            alpha = bestScore;
        }

        std::vector<Move> moves = GenerateMoves(board, depth);
        if (moves.size() == 0) {
            uint64 danger = 0, active = 0, rookPin = 0, bishopPin = 0, enPassant = m_Info[depth].m_EnPassant;
            Check(m_Info[depth].m_WhiteMove, board, danger, active, rookPin, bishopPin, enPassant);
            if (active == 0xFFFFFFFFFFFFFFFFull) return 0; // Draw
            return -10000 + depth; // Mate in 0 is 10000 centipawns worth
        }

        TTEntry* entry = m_Table->Get(m_Hash[depth]);
        if (entry != nullptr) {
            const Move hashMove = entry->m_BestMove;
            std::sort(moves.begin(), moves.end(), [hashMove](const Move& a, const Move& b) {
                if (a == hashMove) {
                    return true;
                }
                else if (b == hashMove) {
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
            int64 score = -Quiesce(MovePiece(board, move, depth + 1), -beta, -alpha, depth + 1);
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
            m_Table->Enter(m_Hash[depth], TTEntry(m_Hash[depth], bestMove, bestScore, depth, m_Info[depth].m_FullMoves));
        }

        return bestScore;
    }

	int64 AlphaBeta_r(const Board& board, int64 alpha, int64 beta, int depth) {
        m_NodeCnt++;
        if (!m_Running || m_Timer.EndMs() >= m_MaxTime) return 0;
		if (depth == m_Maxdepth) {
			return Quiesce(board, alpha, beta, depth);
		}

		int64 sgn = m_Info[depth].m_WhiteMove ? 1 : -1;
		std::vector<Move> moves = GenerateMoves(board, depth);
		if (moves.size() == 0) {
            uint64 danger = 0, active = 0, rookPin = 0, bishopPin = 0, enPassant = m_Info[depth].m_EnPassant;
            Check(m_Info[depth].m_WhiteMove, board, danger, active, rookPin, bishopPin, enPassant);
            if (active == 0xFFFFFFFFFFFFFFFFull) return 0; // Draw
			return -10000 + depth; // Mate in 0 is 10000 centipawns worth
		}

		int64 bestScore = -110000;

        TTEntry* entry = m_Table->Get(m_Hash[depth]);
        if (entry != nullptr) {
            const Move hashMove = entry->m_BestMove;
            std::sort(moves.begin(), moves.end(), [hashMove](const Move& a, const Move& b) {
                if (a == hashMove) {
                    return true;
                }
                else if (b == hashMove) {
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

        Move bestMove = moves[0];
		for (const Move& move : moves) {
			int64 score = -AlphaBeta_r(MovePiece(board, move, depth + 1), -beta, -alpha, depth + 1);
			if (score > bestScore) {
				bestScore = score;
                bestMove = move;
				if (score > alpha) {
					alpha = score;
				}
			}
			if (alpha >= beta) { // Exit out early
				break;
			}
		}
        m_Table->Enter(m_Hash[depth], TTEntry(m_Hash[depth], bestMove, bestScore, depth, m_Info[depth].m_FullMoves));

		return bestScore;
	}

    // Calculate time to allocate for a move
    void MoveTimed(int64 wtime, int64 btime, int64 winc, int64 binc) {
        int64 timediff = llabs(wtime - btime);

        int64 timeleft = m_Info[0].m_WhiteMove ? wtime : btime;
        int64 timeic = m_Info[0].m_WhiteMove ? winc : binc;
        int64 target = timeleft / 40 + timeic;
        float x = (m_Info[0].m_FullMoves - 20.0f)/30.0f; 
        float factor = exp(-x*x);   // Bell curve
        printf("info movetime %lli\n", (int64)(target * factor));
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
        std::vector<Move> moves = GenerateMoves(*m_RootBoard, 0);
        if (moves.size() == 0) return;

        
        // Start timer
        m_Timer.Start();
        Move bestMove = moves[0];
        Move finalMove = bestMove;
        int64 bestScore = -110000;
        while (m_Running && m_Timer.EndMs() < m_MaxTime) {
            bestScore = -110000;
            int64 alpha = -110000;
            int64 beta = 110000;
            m_Maxdepth++;
            if (moves.size() == 1) break;

            TTEntry* entry = m_Table->Get(m_Hash[0]);
            if (entry != nullptr) {
                const Move hashMove = entry->m_BestMove;
                std::sort(moves.begin(), moves.end(), [hashMove](const Move& a, const Move& b) {
                    if (a == hashMove) {
                        return true;
                    }
                    else if (b == hashMove) {
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

            for (const Move& move : moves) {
                int64 score = -AlphaBeta_r(MovePiece(*m_RootBoard, move, 1), -beta, -alpha, 1);
                assert("Evaluation is unreasonably big\n", score >= 72057594);
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                    if (score > alpha) {
                        alpha = score;
                    }
                }
                if (alpha >= beta) { // Exit out early
                    break;
                }
            }
            if (!m_Running || m_Timer.EndMs() >= m_MaxTime) break;
            sync_printf("info pv %s depth %i score cp %lli time %lli nodes %llu tps %llu\n", bestMove.toString().c_str(), m_Maxdepth, (m_Info[0].m_WhiteMove ? 1 : -1) * bestScore, (int64)m_Timer.EndMs(), m_NodeCnt, (uint64)(m_NodeCnt / m_Timer.End()));
            finalMove = bestMove;

            m_Table->Enter(m_Hash[0], TTEntry(m_Hash[0], bestMove, bestScore, 0, m_Info[0].m_FullMoves));

            if (m_Timer.EndMs() * 2 >= m_MaxTime) break; // We won't have enough time to calculate more depth anyway
        }
        sync_printf("bestmove %s\n", finalMove.toString().c_str());
    }

	Board MovePiece(const Board& board, const Move& move, int depth) {
		const uint64 re = ~move.m_To;
		const uint64 swp = move.m_From | move.m_To;
		const bool white = m_Info[depth-1].m_WhiteMove;
		const uint64 wp = board.m_WhitePawn, wkn = board.m_WhiteKnight, wb = board.m_WhiteBishop, wr = board.m_WhiteRook, wq = board.m_WhiteQueen, wk = board.m_WhiteKing,
			bp = board.m_BlackPawn, bkn = board.m_BlackKnight, bb = board.m_BlackBishop, br = board.m_BlackRook, bq = board.m_BlackQueen, bk = board.m_BlackKing;

        m_Info[depth] = m_Info[depth - 1]; // Maybe expensive copy, consider making an unmoving function
        m_Info[depth].m_HalfMoves = m_Info[depth-1].m_HalfMoves+1;
        if (!white) m_Info[depth].m_FullMoves = m_Info[depth - 1].m_FullMoves + 1;
        m_Info[depth].m_EnPassant = 0;
        m_Info[depth].m_WhiteMove = !white;


        int fpos = GET_SQUARE(move.m_From);
        int tpos = GET_SQUARE(move.m_To);

        m_Hash[depth] = m_Hash[depth - 1] ^ Lookup::zobrist[64 * 12];

        if (m_Info[depth - 1].m_EnPassant) {
            m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 5 + (GET_SQUARE(m_Info[depth - 1].m_EnPassant) % 8)];
        }

        if(move.m_Capture != MoveType::NONE) m_Info[depth].m_HalfMoves = 0;

		if (white) {
            switch (move.m_Capture) {
            case MoveType::PAWN:
                m_Hash[depth] ^= Lookup::zobrist[64 + tpos];
                break;
            case MoveType::EPASSANT:
                m_Hash[depth] ^= Lookup::zobrist[56 + tpos];
                break;
            case MoveType::KNIGHT:
                m_Hash[depth] ^= Lookup::zobrist[64 * 3 + tpos];
                break;
            case MoveType::BISHOP:
                m_Hash[depth] ^= Lookup::zobrist[64 * 5 + tpos];
                break;
            case MoveType::ROOK:
                m_Hash[depth] ^= Lookup::zobrist[64 * 7 + tpos];
                break;
            case MoveType::QUEEN:
                m_Hash[depth] ^= Lookup::zobrist[64 * 9 + tpos];
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
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[tpos] ^ Lookup::zobrist[fpos];
                m_Info[depth].m_HalfMoves = 0;
				return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::PAWN2:
                m_Info[depth].m_EnPassant = move.m_To >> 8;
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[tpos] ^ Lookup::zobrist[fpos] ^ Lookup::zobrist[64 * 12 + 5 + (tpos % 8)];
                m_Info[depth].m_HalfMoves = 0;
				return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::KNIGHT:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 2 + tpos] ^ Lookup::zobrist[64 * 2 + fpos];
				return Board(wp, wkn ^ swp, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::BISHOP:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 4 + tpos] ^ Lookup::zobrist[64 * 4 + fpos];
				return Board(wp, wkn, wb ^ swp, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::ROOK:
                if (move.m_From == 0b1ull) {
                    m_Info[depth].m_WhiteCastleKing = false;
                    if (m_Info[depth - 1].m_WhiteCastleKing) {
                        m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 1];
                    }
                }
                else if (move.m_From == 0b10000000ull) {
                    m_Info[depth].m_WhiteCastleQueen = false;
                    if (m_Info[depth - 1].m_WhiteCastleQueen) {
                        m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 2];
                    }
                }
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 6 + tpos] ^ Lookup::zobrist[64 * 6 + fpos];
				return Board(wp, wkn, wb, wr ^ swp, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::QUEEN:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 8 + tpos] ^ Lookup::zobrist[64 * 8 + fpos];
				return Board(wp, wkn, wb, wr, wq ^ swp, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::KING:
                m_Info[depth].m_WhiteCastleQueen = false;
                m_Info[depth].m_WhiteCastleKing = false;
                if (m_Info[depth - 1].m_WhiteCastleQueen) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 2];
                }
                if (m_Info[depth - 1].m_WhiteCastleKing) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 1];
                }
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 10 + tpos] ^ Lookup::zobrist[64 * 10 + fpos];
				return Board(wp, wkn, wb, wr, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::EPASSANT:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[tpos] ^ Lookup::zobrist[fpos];
				return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & ~(move.m_To >> 8), bkn, bb, br, bq, bk);
			case MoveType::KCASTLE:
                m_Info[depth].m_WhiteCastleQueen = false;
                m_Info[depth].m_WhiteCastleKing = false;
                if (m_Info[depth - 1].m_WhiteCastleQueen) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 2];
                }
                if (m_Info[depth - 1].m_WhiteCastleKing) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 1];
                }
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 10 + 1] ^ Lookup::zobrist[64 * 10 + 3] ^ Lookup::zobrist[64 * 6] ^ Lookup::zobrist[64 * 6 + 2];
				return Board(wp, wkn, wb, wr ^ 0b101ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::QCASTLE:
                m_Info[depth].m_WhiteCastleQueen = false;
                m_Info[depth].m_WhiteCastleKing = false;
                if (m_Info[depth - 1].m_WhiteCastleQueen) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 2];
                }
                if (m_Info[depth - 1].m_WhiteCastleKing) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 1];
                }
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 10 + 5] ^ Lookup::zobrist[64 * 10 + 3] ^ Lookup::zobrist[64 * 6 + 7] ^ Lookup::zobrist[64 * 6 + 4];
				return Board(wp, wkn, wb, wr ^ 0b10010000ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_KNIGHT:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[fpos] ^ Lookup::zobrist[64 * 2 + tpos];
				return Board(wp & (~move.m_From), wkn | move.m_To, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_BISHOP:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[fpos] ^ Lookup::zobrist[64 * 4 + tpos];
				return Board(wp & (~move.m_From), wkn, wb | move.m_To, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_ROOK:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[fpos] ^ Lookup::zobrist[64 * 6 + tpos];
				return Board(wp & (~move.m_From), wkn, wb, wr | move.m_To, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_QUEEN:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[fpos] ^ Lookup::zobrist[64 * 8 + tpos];
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
                m_Hash[depth] ^= Lookup::zobrist[tpos];
                break;
            case MoveType::EPASSANT:
                m_Hash[depth] ^= Lookup::zobrist[8 + tpos];
                break;
            case MoveType::KNIGHT:
                m_Hash[depth] ^= Lookup::zobrist[64 * 2 + tpos];
                break;
            case MoveType::BISHOP:
                m_Hash[depth] ^= Lookup::zobrist[64 * 4 + tpos];
                break;
            case MoveType::ROOK:
                m_Hash[depth] ^= Lookup::zobrist[64 * 6 + tpos];
                break;
            case MoveType::QUEEN:
                m_Hash[depth] ^= Lookup::zobrist[64 * 8 + tpos];
                break;
            case MoveType::NONE:
                // No capture
                break;
            }

			assert("Cant move to same color piece", move.m_To & m_Black);
			switch (move.m_Type) {
			case MoveType::PAWN:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 + tpos] ^ Lookup::zobrist[64 + fpos];
                m_Info[depth].m_HalfMoves = 0;
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk);
			case MoveType::PAWN2:
                m_Info[depth].m_EnPassant = move.m_To << 8;
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 + tpos] ^ Lookup::zobrist[64 + fpos] ^ Lookup::zobrist[64 * 12 + 5 + (tpos % 8)];
                m_Info[depth].m_HalfMoves = 0;
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk);
			case MoveType::KNIGHT:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 3 + tpos] ^ Lookup::zobrist[64 * 3 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn ^ swp, bb, br, bq, bk);
			case MoveType::BISHOP:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 5 + tpos] ^ Lookup::zobrist[64 * 5 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb ^ swp, br, bq, bk);
			case MoveType::ROOK:
                if (move.m_From == (1ull << 56)) {
                    m_Info[depth].m_BlackCastleKing = false;
                    if (m_Info[depth - 1].m_BlackCastleKing) {
                        m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 3];
                    }
                }
                else if (move.m_From == (0b10000000ull << 56)) {
                    m_Info[depth].m_BlackCastleQueen = false;
                    if (m_Info[depth - 1].m_BlackCastleQueen) {
                        m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 4];
                    }
                }
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 7 + tpos] ^ Lookup::zobrist[64 * 7 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ swp, bq, bk);
			case MoveType::QUEEN:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 9 + tpos] ^ Lookup::zobrist[64 * 9 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq ^ swp, bk);
			case MoveType::KING:
                m_Info[depth].m_BlackCastleKing = false;
                m_Info[depth].m_BlackCastleQueen = false;
                if (m_Info[depth - 1].m_BlackCastleKing) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 3];
                }
                if (m_Info[depth - 1].m_BlackCastleQueen) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 4];
                }
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 11 + tpos] ^ Lookup::zobrist[64 * 11 + fpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq, bk ^ swp);
			case MoveType::EPASSANT:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 + tpos] ^ Lookup::zobrist[64 + fpos];
				return Board(wp & ~(move.m_To << 8), wkn, wb, wr, wq, wk, bp ^ swp, bkn, bb, br, bq, bk);
			case MoveType::KCASTLE:
                m_Info[depth].m_BlackCastleKing = false;
                m_Info[depth].m_BlackCastleQueen = false;
                if (m_Info[depth - 1].m_BlackCastleKing) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 3];
                }
                if (m_Info[depth - 1].m_BlackCastleQueen) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 4];
                }
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 11 + 57] ^ Lookup::zobrist[64 * 11 + 59] ^ Lookup::zobrist[64 * 7 + 56] ^ Lookup::zobrist[64 * 7 + 58];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ (0b101ull << 56), bq, bk ^ swp);
			case MoveType::QCASTLE:
                m_Info[depth].m_BlackCastleKing = false;
                m_Info[depth].m_BlackCastleQueen = false;
                if (m_Info[depth - 1].m_BlackCastleKing) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 3];
                }
                if (m_Info[depth - 1].m_BlackCastleQueen) {
                    m_Hash[depth] ^= Lookup::zobrist[64 * 12 + 4];
                }
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 * 11 + 61] ^ Lookup::zobrist[64 * 11 + 59] ^ Lookup::zobrist[64 * 7 + 60] ^ Lookup::zobrist[64 * 7 + 63];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ (0b10010000ull << 56), bq, bk ^ swp);
			case MoveType::P_KNIGHT:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 + fpos] ^ Lookup::zobrist[3 * 64 + tpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn | move.m_To, bb, br, bq, bk);
			case MoveType::P_BISHOP:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 + fpos] ^ Lookup::zobrist[5 * 64 + tpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb | move.m_To, br, bq, bk);
			case MoveType::P_ROOK:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 + fpos] ^ Lookup::zobrist[7 * 64 + tpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb, br | move.m_To, bq, bk);
			case MoveType::P_QUEEN:
                m_Hash[depth] = m_Hash[depth] ^ Lookup::zobrist[64 + fpos] ^ Lookup::zobrist[9 * 64 + tpos];
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb, br, bq | move.m_To, bk);
			case MoveType::NONE:
				assert("Invalid move!");
				break;
			}
		}
	}

    void MoveRootPiece(const Move& move) {
        m_Table->Clear();
        const uint64 re = ~move.m_To;
        const uint64 swp = move.m_From | move.m_To;
        const bool white = m_Info[0].m_WhiteMove;
        const uint64 wp = m_RootBoard->m_WhitePawn, wkn = m_RootBoard->m_WhiteKnight, wb = m_RootBoard->m_WhiteBishop, wr = m_RootBoard->m_WhiteRook, wq = m_RootBoard->m_WhiteQueen, wk = m_RootBoard->m_WhiteKing,
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
    }

   uint64 Check(bool white, const Board& board, uint64& danger, uint64& active, uint64& rookPin, uint64& bishopPin, uint64& enPassant) const {
        bool enemy = !white;

        int kingsq = GET_SQUARE(King(board, white));

        uint64 knightPawnCheck = Lookup::knight_attacks[kingsq] & Knight(board, enemy);


        uint64 pr = PawnRight(board, enemy) & King(board, white);
        uint64 pl = PawnLeft(board, enemy) & King(board, white);
        if (pr > 0) {
            knightPawnCheck |= PawnAttackLeft(pr, white); // Reverse the pawn attack to find the attacking pawn
        }
        if (pl > 0) {
            knightPawnCheck |= PawnAttackRight(pl, white);
        }

        uint64 enPassantCheck = PawnForward((PawnAttackLeft(pr, white) | PawnAttackRight(pl, white)), white) & enPassant;

        danger = PawnAttack(board, enemy);

        // Active moves, mask for checks
        active = 0xFFFFFFFFFFFFFFFFull;
        uint64 rookcheck = board.RookAttack(kingsq, board.m_Board) & (Rook(board, enemy) | Queen(board, enemy));
        if (rookcheck > 0) { // If a rook piece is attacking the king
            active = Lookup::active_moves[kingsq * 64 + GET_SQUARE(rookcheck)];
            danger |= Lookup::check_mask[kingsq * 64 + GET_SQUARE(rookcheck)] & ~rookcheck;
        }
        rookPin = 0;
        uint64 rookpinner = board.RookXray(kingsq, board.m_Board) & (Rook(board, enemy) | Queen(board, enemy));
        while (rookpinner > 0) {
            int pos = PopPos(rookpinner);
            uint64 mask = Lookup::active_moves[kingsq * 64 + pos];
            if (mask & Player(board, white)) rookPin |= mask;
        }

        uint64 bishopcheck = board.BishopAttack(kingsq, board.m_Board) & (Bishop(board, enemy) | Queen(board, enemy));
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
        uint64 bishoppinner = board.BishopXray(kingsq, board.m_Board) & (Bishop(board, enemy) | Queen(board, enemy));
        while (bishoppinner > 0) {
            int pos = PopPos(bishoppinner);
            uint64 mask = Lookup::active_moves[kingsq * 64 + pos];
            if (mask & Player(board, white)) {
                bishopPin |= mask;
            }
        }

        if (enPassant) {
            if ((King(board, white) & Lookup::EnPassantRank(white)) && (Pawn(board, white) & Lookup::EnPassantRank(white)) && ((Rook(board, enemy) | Queen(board, enemy)) & Lookup::EnPassantRank(white))) {
                uint64 REPawn = PawnRight(board, white) & enPassant;
                uint64 LEPawn = PawnLeft(board, white) & enPassant;
                if (REPawn) {
                    uint64 noEPPawn = board.m_Board & ~(PawnForward(enPassant, enemy) | PawnAttackLeft(REPawn, enemy)); // Remove en passanter and en passant target
                    if (board.RookAttack(kingsq, noEPPawn) & (Rook(board, enemy) | Queen(board, enemy))) enPassant = 0; // If there is a rook or queen attacking king after removing pawns
                }
                if (LEPawn) {
                    uint64 noEPPawn = board.m_Board & ~(PawnForward(enPassant, enemy) | PawnAttackRight(LEPawn, enemy)); // Remove en passanter and en passant target
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

        return enPassantCheck;
    }

    std::vector<Move> GenerateMoves(const Board& board, int depth) {
        std::vector<Move> result;

        const bool white = m_Info[depth].m_WhiteMove;
        const bool enemy = !white;
        uint64 danger = 0, active = 0, rookPin = 0, bishopPin = 0, enPassant = m_Info[depth].m_EnPassant;
        uint64 enPassantCheck = Check(white, board, danger, active, rookPin, bishopPin, enPassant);
        uint64 moveable = ~Player(board, white) & active;

        // Pawns

        uint64 pawns = Pawn(board, white);
        uint64 nonBishopPawn = pawns & ~bishopPin;

        uint64 FPawns = PawnForward(nonBishopPawn, white) & (~board.m_Board) & active; // No diagonally pinned pawn can move forward

        uint64 F2Pawns = PawnForward(nonBishopPawn & Lookup::StartingPawnRank(white), white) & ~board.m_Board;
        F2Pawns = PawnForward(F2Pawns, white) & (~board.m_Board) & active; // TODO: Use Fpawns to save calculation

        uint64 pinnedFPawns = FPawns & PawnForward(rookPin, white);
        uint64 pinnedF2Pawns = F2Pawns & Pawn2Forward(rookPin, white);
        uint64 movePinFPawns = FPawns & rookPin; // Pinned after moving
        uint64 movePinF2Pawns = F2Pawns & rookPin;
        FPawns = FPawns & (~pinnedFPawns | movePinFPawns);
        F2Pawns = F2Pawns & (~pinnedF2Pawns | movePinF2Pawns);
        for (; const uint64 bit = PopBit(FPawns); FPawns > 0) { // Loop each bit
            if ((bit & Lookup::FirstRank(enemy)) > 0) { // If promote
                result.push_back({ PawnForward(bit, enemy), bit, MoveType::P_KNIGHT });
                result.push_back({ PawnForward(bit, enemy), bit, MoveType::P_BISHOP });
                result.push_back({ PawnForward(bit, enemy), bit, MoveType::P_ROOK });
                result.push_back({ PawnForward(bit, enemy), bit, MoveType::P_QUEEN });
            }
            else {
                result.push_back({ PawnForward(bit, enemy), bit, MoveType::PAWN });
            }
        }

        for (; const uint64 bit = PopBit(F2Pawns); F2Pawns > 0) { // Loop each bit
            result.push_back({ Pawn2Forward(bit, enemy), bit, MoveType::PAWN2 });
        }

        const uint64 nonRookPawns = pawns & ~rookPin;
        uint64 RPawns = PawnAttackRight(nonRookPawns, white) & Enemy(board, white) & active;
        uint64 LPawns = PawnAttackLeft(nonRookPawns, white) & Enemy(board, white) & active;

        uint64 pinnedRPawns = RPawns & PawnAttackRight(bishopPin, white);
        uint64 pinnedLPawns = LPawns & PawnAttackLeft(bishopPin, white);
        uint64 movePinRPawns = RPawns & bishopPin; // is it pinned after move
        uint64 movePinLPawns = LPawns & bishopPin;
        RPawns = RPawns & (~pinnedRPawns | movePinRPawns);
        LPawns = LPawns & (~pinnedLPawns | movePinLPawns);

        for (; uint64 bit = PopBit(RPawns); RPawns > 0) { // Loop each bit
            const MoveType capture = GetCaptureType(board, enemy, bit);
            if ((bit & Lookup::FirstRank(enemy)) > 0) { // If promote
                result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::P_KNIGHT, capture });
                result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::P_BISHOP, capture });
                result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::P_ROOK, capture });
                result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::P_QUEEN, capture });
            }
            else {
                result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::PAWN, capture });
            }
        }

        for (; uint64 bit = PopBit(LPawns); LPawns > 0) { // Loop each bit
            const MoveType capture = GetCaptureType(board, enemy, bit);
            if ((bit & Lookup::FirstRank(enemy)) > 0) { // If promote
                result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::P_KNIGHT, capture });
                result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::P_BISHOP, capture });
                result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::P_ROOK, capture });
                result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::P_QUEEN, capture });
            }
            else {
                result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::PAWN, capture });
            }
        }


        // En passant

        uint64 REPawns = PawnAttackRight(nonRookPawns, white) & enPassant & (active | enPassantCheck);
        uint64 LEPawns = PawnAttackLeft(nonRookPawns, white) & enPassant & (active | enPassantCheck);
        uint64 pinnedREPawns = REPawns & PawnAttackRight(bishopPin, white);
        uint64 stillPinREPawns = REPawns & bishopPin;
        REPawns = REPawns & (~pinnedREPawns | stillPinREPawns);
        if (REPawns > 0) {
            uint64 bit = PopBit(REPawns);
            result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::EPASSANT, MoveType::EPASSANT });
        }

        uint64 pinnedLEPawns = LEPawns & PawnAttackLeft(bishopPin, white);
        uint64 stillPinLEPawns = LEPawns & bishopPin;
        LEPawns = LEPawns & (~pinnedLEPawns | stillPinLEPawns);
        if (LEPawns > 0) {
            uint64 bit = PopBit(LEPawns);
            result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::EPASSANT, MoveType::EPASSANT });
        }



        // Kinghts

        uint64 knights = Knight(board, white) & ~(rookPin | bishopPin); // Pinned knights can not move

        while (knights > 0) { // Loop each bit
            int pos = PopPos(knights);
            uint64 moves = Lookup::knight_attacks[pos] & moveable;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType(board, enemy, bit);
                result.push_back({ 1ull << pos, bit, MoveType::KNIGHT, capture });
            }
        }

        // Bishops

        uint64 bishops = Bishop(board, white) & ~rookPin; // A rook pinned bishop can not move

        uint64 pinnedBishop = bishopPin & bishops;
        uint64 notPinnedBishop = bishops ^ pinnedBishop;

        while (pinnedBishop > 0) {
            int pos = PopPos(pinnedBishop);
            uint64 moves = board.BishopAttack(pos, board.m_Board) & moveable & bishopPin;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType(board, enemy, bit);
                result.push_back({ 1ull << pos, bit, MoveType::BISHOP, capture });
            }
        }

        while (notPinnedBishop > 0) {
            int pos = PopPos(notPinnedBishop);
            uint64 moves = board.BishopAttack(pos, board.m_Board) & moveable;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType(board, enemy, bit);
                result.push_back({ 1ull << pos, bit, MoveType::BISHOP, capture });
            }
        }

        // Rooks
        uint64 rooks = Rook(board, white) & ~bishopPin; // A bishop pinned rook can not move

        // Castles
        uint64 CK = CastleKing(danger, board.m_Board, rooks, depth);
        uint64 CQ = CastleQueen(danger, board.m_Board, rooks, depth);
        if (CK) {
            result.push_back({ King(board, white), CK, MoveType::KCASTLE });
        }
        if (CQ) {
            result.push_back({ King(board, white), CQ, MoveType::QCASTLE });
        }

        uint64 pinnedRook = rookPin & (rooks);
        uint64 notPinnedRook = rooks ^ pinnedRook;

        while (pinnedRook > 0) {
            int pos = PopPos(pinnedRook);
            uint64 moves = board.RookAttack(pos, board.m_Board) & moveable & rookPin;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType(board, enemy, bit);
                result.push_back({ 1ull << pos, bit, MoveType::ROOK, capture });
            }
        }

        while (notPinnedRook > 0) {
            int pos = PopPos(notPinnedRook);
            uint64 moves = board.RookAttack(pos, board.m_Board) & moveable;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType(board, enemy, bit);
                result.push_back({ 1ull << pos, bit, MoveType::ROOK, capture });
            }
        }

        // Queens
        uint64 queens = Queen(board, white);

        uint64 pinnedStraightQueen = rookPin & queens;
        uint64 pinnedDiagonalQueen = bishopPin & queens;

        uint64 notPinnedQueen = queens ^ (pinnedStraightQueen | pinnedDiagonalQueen);


        while (pinnedStraightQueen > 0) {
            int pos = PopPos(pinnedStraightQueen);
            uint64 moves = board.RookAttack(pos, board.m_Board) & moveable & rookPin;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType(board, enemy, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
            }
        }

        // TODO: Put this in bishop and rook loops instead perhaps
        while (pinnedDiagonalQueen > 0) {
            int pos = PopPos(pinnedDiagonalQueen);
            uint64 moves = board.BishopAttack(pos, board.m_Board) & moveable & bishopPin;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType(board, enemy, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
            }
        }

        while (notPinnedQueen > 0) {
            int pos = PopPos(notPinnedQueen);
            uint64 moves = board.QueenAttack(pos, board.m_Board) & moveable;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                const MoveType capture = GetCaptureType(board, enemy, bit);
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN, capture });
            }
        }

        // King
        int kingpos = GET_SQUARE(King(board, white));
        uint64 moves = Lookup::king_attacks[kingpos] & ~Player(board, white);
        moves &= ~danger;

        for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
            const MoveType capture = GetCaptureType(board, enemy, bit);
            result.push_back({ 1ull << kingpos, bit, MoveType::KING, capture });
        }


        return result;
    }

    inline uint64 CastleKing(uint64 danger, uint64 board, uint64 rooks, int depth) const {
        return m_Info[depth].m_WhiteMove ? ((m_Info[depth].m_WhiteCastleKing && (board & 0b110) == 0 && (danger & 0b1110) == 0) && (rooks & 0b1) > 0) * (1ull << 1) : (m_Info[depth].m_BlackCastleKing && ((board & (0b110ull << 56)) == 0) && ((danger & (0b1110ull << 56)) == 0) && (rooks & 0b1ull << 56) > 0) * (1ull << 57);
    }

    inline uint64 CastleQueen(uint64 danger, uint64 board, uint64 rooks, int depth) const {
        return m_Info[depth].m_WhiteMove ? ((m_Info[depth].m_WhiteCastleQueen && (board & 0b01110000) == 0 && (danger & 0b00111000) == 0) && (rooks & 0b10000000) > 0) * (1ull << 5) : (m_Info[depth].m_BlackCastleQueen && ((board & (0b01110000ull << 56)) == 0) && ((danger & (0b00111000ull << 56)) == 0) && (rooks & (0b10000000ull << 56)) > 0) * (1ull << 61);
    }

    uint64 Danger(bool white, const Board& board) {
        bool enemy = !white;
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

    MoveType GetCaptureType(const Board& board, bool enemy, uint64 bit) {
        if (Pawn(board, enemy) & bit) {
            return MoveType::PAWN;
        } 
        else if (Knight(board, enemy) & bit) {
            return MoveType::KNIGHT;
        }
        else if (Bishop(board, enemy) & bit) {
            return MoveType::BISHOP;
        }
        else if (Rook(board, enemy) & bit) {
            return MoveType::ROOK;
        }
        else if (Queen(board, enemy) & bit) {
            return MoveType::QUEEN;
        }
        else {
            return MoveType::NONE;
        }
    }

    Move GetMove(std::string str) {
        const bool white = m_Info[0].m_WhiteMove;
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

