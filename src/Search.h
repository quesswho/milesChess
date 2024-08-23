#pragma once

#include "Movelist.h"

#define MAX_DEPTH 64 // Maximum depth that the engine will go

class Search {
private:

	int m_Maxdepth; // Maximum depth currently set
	BoardInfo m_Info[64];
	std::unique_ptr<Board> m_RootBoard;
	uint64 m_Hash[64];
public:

	Search() 
        : m_Maxdepth(0)
	{}

	void LoadPosition(std::string fen) {
        m_Info[0] = FenInfo(fen);
		m_RootBoard = std::make_unique<Board>(Board(fen));
        m_Hash[0] = Zobrist_Hash(*m_RootBoard, m_Info[0]);
	}

	int64 Evaluate(const Board& board) {

		int64 centipawns = 0;

		uint64 wp = board.m_WhitePawn, wkn = board.m_WhiteKnight, wb = board.m_WhiteBishop, wr = board.m_WhiteRook, wq = board.m_WhiteQueen, wk = board.m_WhiteKing,
			bp = board.m_BlackPawn, bkn = board.m_BlackKnight, bb = board.m_BlackBishop, br = board.m_BlackRook, bq = board.m_BlackQueen, bk = board.m_BlackKing;

		const uint64 pawnVal = 100, knightVal = 350, bishopVal = 350, rookVal = 525, queenVal = 1000, kingVal = 10000;

		// Pawns
		while (wp > 0) {
			int pos = 63 - PopPos(wp);
			centipawns += pawnVal + Lookup::pawn_table[pos];
		}

		while (bp > 0) {
			int pos = PopPos(bp);
			centipawns -= pawnVal + Lookup::pawn_table[pos];
		}

		// Knights
		while (wkn > 0) {
			int pos = 63 - PopPos(wkn);
			centipawns += knightVal + Lookup::knight_table[pos];
		}

		while (bkn > 0) {
			int pos = PopPos(bkn);
			centipawns -= knightVal + Lookup::knight_table[pos];
		}

		// Bishops
		while (wb > 0) {
			int pos = 63 - PopPos(wb);
			centipawns += bishopVal + Lookup::bishop_table[pos];
		}

		while (bb > 0) {
			int pos = PopPos(bb);
			centipawns -= bishopVal + Lookup::bishop_table[pos];
		}

		// Rooks
		while (wr > 0) {
			int pos = 63 - PopPos(wr);
			centipawns += rookVal + Lookup::rook_table[pos];
		}

		while (br > 0) {
			int pos = PopPos(br);
			centipawns -= rookVal + Lookup::rook_table[pos];
		}

		while (wq > 0) {
			int pos = 63 - PopPos(wq);
			centipawns += queenVal + Lookup::queen_table[pos];
		}

		while (bq > 0) {
			int pos = PopPos(bq);
			centipawns -= queenVal + Lookup::queen_table[pos];;
		}

		while (wk > 0) {
			int pos = 63 - PopPos(wk);
			centipawns += kingVal + Lookup::king_table[pos];
		}

		while (bk > 0) {
			int pos = PopPos(bk);
			centipawns -= kingVal + Lookup::king_table[pos];;
		}

		return centipawns;
	}

	uint64 Perft_r(const Board& board, int depth) {
		const std::vector<Move> moves = GenerateMoves(board, depth);
		if (depth == m_Maxdepth-1) return moves.size();

		int64 result = 0;
		for (const Move& move : moves) {
			int64 count = Perft_r(MovePiece(board, move, depth + 1), depth + 1);
			result += count;
			if (depth == 0) {
				printf("%s: %llu: %s\n", move.toString().c_str(), count, BoardtoFen(MovePiece(board, move, depth + 1), m_Info[depth+1]).c_str());
			}
		}
		return result;
	}

	uint64 Perft(std::string str, int maxdepth) {
        m_Maxdepth = maxdepth;
		Timer time;
		Board board(str);
		time.Start();
		uint64 result = Perft_r(board, 0);
		float t = time.End();
		printf("%.3fMNodes/s\n", (result / 1000000.0f) / t);
		return result;
	}

	struct Line {
		Line()
			: count(0), moves()
		{
		}

		int count; // Number of moves
		Move moves[MAX_DEPTH];
	};

	int64 AlphaBeta_r(Board board, int64 alpha, int64 beta, int depth) {
		int64 sgn = ((depth + m_Info[0].m_WhiteMove) & 0b1) ? 1 : -1;
		if (depth == m_Maxdepth) {
			return sgn * Evaluate(board);
		}
		const std::vector<Move> moves = GenerateMoves(board, depth);
		if (moves.size() == 0) {

			return -sgn * (10000 + depth); // Mate in 0 is 10000 centipawns worth
		}
		Line line;
		int64 bestScore = -110000;
		for (const Move& move : moves) {
			int64 score = -AlphaBeta_r(MovePiece(board, move, depth + 1), -beta, -alpha, depth + 1);
			if (score > bestScore) {
				bestScore = score;
				if (score > alpha) {
					alpha = score;
				}
			}
			if (score >= beta) { // Exit out early
				return bestScore;
			}
		}
		return bestScore;
	}

	Move BestMove(int depth) {
		m_Maxdepth = depth;
		const std::vector<Move> moves = GenerateMoves(*m_RootBoard, 0);
		if (moves.size() == 0) return Move(0, 0, MoveType::NONE);
		Move best = moves[0];
		int64 bestscore = -110000;
		for (const Move& move : moves) {
			int64 score = -AlphaBeta_r(MovePiece(*m_RootBoard, move, 1), -110000, 110000, 1);
			if (score > bestscore) {
				bestscore = score;
				best = move;
			}
		}
		return best;
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

		if (white) {
			assert("Cant move to same color piece", move.m_To & m_White);
			switch (move.m_Type) {
			case MoveType::PAWN:
				return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::PAWN2:
                m_Info[depth].m_EnPassant = move.m_To >> 8;
				return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::KNIGHT:
				return Board(wp, wkn ^ swp, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::BISHOP:
				return Board(wp, wkn, wb ^ swp, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::ROOK:
                if (move.m_From == 0b1ull) {
                    m_Info[depth].m_WhiteCastleKing = false;
                }
                else if (move.m_From == 0b10000000ull) {
                    m_Info[depth].m_WhiteCastleQueen = false;
                }
				return Board(wp, wkn, wb, wr ^ swp, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::QUEEN:
				return Board(wp, wkn, wb, wr, wq ^ swp, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::KING:
                m_Info[depth].m_WhiteCastleQueen = false;
                m_Info[depth].m_WhiteCastleKing = false;
				return Board(wp, wkn, wb, wr, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::EPASSANT:
				return Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & ~(move.m_To >> 8), bkn, bb, br, bq, bk);
			case MoveType::KCASTLE:
                m_Info[depth].m_WhiteCastleQueen = false;
                m_Info[depth].m_WhiteCastleKing = false;
				return Board(wp, wkn, wb, wr ^ 0b101ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::QCASTLE:
                m_Info[depth].m_WhiteCastleQueen = false;
                m_Info[depth].m_WhiteCastleKing = false;
				return Board(wp, wkn, wb, wr ^ 0b10010000ull, wq, wk ^ swp, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_KNIGHT:
				return Board(wp & (~move.m_From), wkn | move.m_To, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_BISHOP:
				return Board(wp & (~move.m_From), wkn, wb | move.m_To, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_ROOK:
				return Board(wp & (~move.m_From), wkn, wb, wr | move.m_To, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::P_QUEEN:
				return Board(wp & (~move.m_From), wkn, wb, wr, wq | move.m_To, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk);
			case MoveType::NONE:
				assert("Invalid move!");
				break;
			}
		}
		else {
			assert("Cant move to same color piece", move.m_To & m_Black);
			switch (move.m_Type) {
			case MoveType::PAWN:
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk);
			case MoveType::PAWN2:
                m_Info[depth].m_EnPassant = move.m_To << 8;
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk);
			case MoveType::KNIGHT:
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn ^ swp, bb, br, bq, bk);
			case MoveType::BISHOP:
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb ^ swp, br, bq, bk);
			case MoveType::ROOK:
                if (move.m_From == (1ull << 56)) {
                    m_Info[depth].m_BlackCastleKing = false;
                }
                else if (move.m_From == (0b10000000ull << 56)) {
                    m_Info[depth].m_BlackCastleQueen = false;
                }
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ swp, bq, bk);
			case MoveType::QUEEN:
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq ^ swp, bk);
			case MoveType::KING:
                m_Info[depth].m_BlackCastleKing = false;
                m_Info[depth].m_BlackCastleQueen = false;
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br, bq, bk ^ swp);
			case MoveType::EPASSANT:
				return Board(wp & ~(move.m_To << 8), wkn, wb, wr, wq, wk, bp ^ swp, bkn, bb, br, bq, bk);
			case MoveType::KCASTLE:
                m_Info[depth].m_BlackCastleKing = false;
                m_Info[depth].m_BlackCastleQueen = false;
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ (0b101ull << 56), bq, bk ^ swp);
			case MoveType::QCASTLE:
                m_Info[depth].m_BlackCastleKing = false;
                m_Info[depth].m_BlackCastleQueen = false;
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp, bkn, bb, br ^ (0b10010000ull << 56), bq, bk ^ swp);
			case MoveType::P_KNIGHT:
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn | move.m_To, bb, br, bq, bk);
			case MoveType::P_BISHOP:
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb | move.m_To, br, bq, bk);
			case MoveType::P_ROOK:
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb, br | move.m_To, bq, bk);
			case MoveType::P_QUEEN:
				return Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp & (~move.m_From), bkn, bb, br, bq | move.m_To, bk);
			case MoveType::NONE:
				assert("Invalid move!");
				break;
			}
		}
	}

    void MoveRootPiece(const Move& move) {
        const uint64 re = ~move.m_To;
        const uint64 swp = move.m_From | move.m_To;
        const bool white = m_Info[0].m_WhiteMove;
        const uint64 wp = m_RootBoard->m_WhitePawn, wkn = m_RootBoard->m_WhiteKnight, wb = m_RootBoard->m_WhiteBishop, wr = m_RootBoard->m_WhiteRook, wq = m_RootBoard->m_WhiteQueen, wk = m_RootBoard->m_WhiteKing,
            bp = m_RootBoard->m_BlackPawn, bkn = m_RootBoard->m_BlackKnight, bb = m_RootBoard->m_BlackBishop, br = m_RootBoard->m_BlackRook, bq = m_RootBoard->m_BlackQueen, bk = m_RootBoard->m_BlackKing;

        m_Info[0].m_HalfMoves++;
        if (!white) m_Info[0].m_FullMoves++;
        m_Info[0].m_EnPassant = 0;
        m_Info[0].m_WhiteMove = !white;

        if (white) {
            assert("Cant move to same color piece", move.m_To & m_White);
            switch (move.m_Type) {
            case MoveType::PAWN:
                m_RootBoard = std::make_unique<Board>(Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
                break;
            case MoveType::PAWN2:
                m_Info[0].m_EnPassant = move.m_To >> 8;
                m_RootBoard = std::make_unique<Board>(Board(wp ^ swp, wkn, wb, wr, wq, wk, bp & re, bkn & re, bb & re, br & re, bq & re, bk));
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
                break;
            case MoveType::PAWN2:
                m_Info[0].m_EnPassant = move.m_To << 8;
                m_RootBoard = std::make_unique<Board>(Board(wp & re, wkn & re, wb & re, wr & re, wq & re, wk, bp ^ swp, bkn, bb, br, bq, bk));
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
            active = Lookup::active_moves[kingsq * 64 + GET_SQUARE(bishopcheck)];
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

        //PrintMap(pawns);
        //printf("\n");
        // PrintMap(nonRookPawns);
         //printf("\n");

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
                result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::PAWN });
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
            result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::EPASSANT, MoveType::PAWN }); // TODO: This could be obsolete as en passant can only capture pawns anyways
        }

        uint64 pinnedLEPawns = LEPawns & PawnAttackLeft(bishopPin, white);
        uint64 stillPinLEPawns = LEPawns & bishopPin;
        LEPawns = LEPawns & (~pinnedLEPawns | stillPinLEPawns);
        if (LEPawns > 0) {
            uint64 bit = PopBit(LEPawns);
            result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::EPASSANT, MoveType::PAWN });
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

        uint64 pinnedRook = rookPin & rooks;
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

    MoveType GetCaptureType(const Board& board, bool enemy, int bit) {
        if (Pawn(board, enemy) & bit) {
            return MoveType::PAWN;
        } else if (Knight(board, enemy) & bit) {
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
                type = MoveType::P_ROOK;
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
                if (CastleKing(danger, m_RootBoard->m_Board, Pawn(*m_RootBoard, white), white) && !(Lookup::king_attacks[posTo] & from)) {
                    type = MoveType::KCASTLE;
                }
                else if (CastleQueen(danger, m_RootBoard->m_Board, Pawn(*m_RootBoard, white), white) && !(Lookup::king_attacks[posTo] & from)) {
                    type = MoveType::QCASTLE;
                }
                else {
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

