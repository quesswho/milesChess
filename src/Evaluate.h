#pragma once

#include <algorithm>
#include <cstdlib>

#include "Movelist.h"

#define TEMPO 20

static constexpr int64 PAWN_VALUE = 100, KNIGHT_VALUE = 350, BISHOP_VALUE = 350, ROOK_VALUE = 525, QUEEN_VALUE = 1000,
                       KING_VALUE = 10000;

// Flips rank only. Must not flip the file too.
static inline int MirrorSquare(int square) {
    return square ^ 56;
}

static inline int SquareDistance(int a, int b) {
    return std::max(std::abs(a / 8 - b / 8), std::abs(a % 8 - b % 8));
}

// 0 in the centre, 6 in a corner.
static inline int CenterDistance(int square) {
    int rank = square / 8, col = square % 8;
    return std::max(3 - rank, rank - 4) + std::max(3 - col, col - 4);
}

static inline bool IsDarkSquare(int square) {
    return ((square / 8 + square % 8) & 1) != 0;
}

// Distance to the nearer of the two corners of the given colour.
static inline int CornerDistance(int square, bool dark) {
    int rank = square / 8, col = square % 8;
    return dark ? std::min(std::max(rank, 7 - col), std::max(7 - rank, col))
                : std::min(std::max(rank, col), std::max(7 - rank, 7 - col));
}

// Compute a gradient for bare king positions. This moves the position towards
// and easy checkmate.
static bool BareKingScore(const Position& board, int64& score) {
    if (board.m_WhitePawn || board.m_BlackPawn) return false;

    BitBoard whiteMen = board.m_White & ~board.m_WhiteKing;
    BitBoard blackMen = board.m_Black & ~board.m_BlackKing;
    if ((whiteMen != 0) == (blackMen != 0)) return false;

    bool whiteStrong = whiteMen != 0;
    BitBoard knights = whiteStrong ? board.m_WhiteKnight : board.m_BlackKnight;
    BitBoard bishops = whiteStrong ? board.m_WhiteBishop : board.m_BlackBishop;
    BitBoard rooks = whiteStrong ? board.m_WhiteRook : board.m_BlackRook;
    BitBoard queens = whiteStrong ? board.m_WhiteQueen : board.m_BlackQueen;

    // A lone minor cannot mate.
    if (!rooks && !queens && CountBits(knights | bishops) < 2) {
        score = 0;
        return true;
    }

    int strongKing = GetSquare(whiteStrong ? board.m_WhiteKing : board.m_BlackKing);
    int weakKing = GetSquare(whiteStrong ? board.m_BlackKing : board.m_WhiteKing);

    int64 drive = 20 * CenterDistance(weakKing) + 12 * (7 - SquareDistance(strongKing, weakKing));

    if (!rooks && !queens && CountBits(knights) == 1 && CountBits(bishops) == 1) {
        // Only corners the bishop covers mate. Added on top of the edge push,
        // so the wrong corner still beats the centre.
        bool dark = IsDarkSquare(GetSquare(bishops));
        drive += 24 * (7 - CornerDistance(weakKing, dark)) + 6 * (7 - CornerDistance(strongKing, dark));
    }

    int64 material = KNIGHT_VALUE * CountBits(knights) + BISHOP_VALUE * CountBits(bishops)
                     + ROOK_VALUE * CountBits(rooks) + QUEEN_VALUE * CountBits(queens);

    score = whiteStrong ? material + drive : -(material + drive);
    return true;
}

template<Color white>
static Score Pawn(const Position& board, int pos, int rpos) {
    const int64 pawnBaseVal = 100;

    int middlegame = pawnBaseVal + Lookup::pawn_table[pos];
    int endgame = pawnBaseVal + Lookup::eg_pawn_table[pos];
    if ((Lookup::pawn_passed<white>(rpos) & Pawn<!white>(board)) == 0) { // pawn is passed
        int64 bit = 1ull << rpos;
        if ((PawnAttackRight<white>(bit) & Pawn<white>(board))
            || (PawnAttackLeft<white>(bit) & Pawn<white>(board))) { // If passed pawn is defended too
            endgame += Lookup::passed_pawn_table[pos] * 1.3;
            middlegame += 20 * 1.3;
        } else {
            endgame += Lookup::passed_pawn_table[pos];
            middlegame += 20;
        }
    }
    if ((Lookup::pawn_forward<white>(rpos) & Pawn<white>(board)) != 0) { // if doubled
        endgame -= 50;
        middlegame -= 15;
    }
    if ((Lookup::isolated_mask[rpos] & Pawn<white>(board)) == 0) {
        middlegame -= 3;
        endgame -= 15;
    }
    return Score({ middlegame, endgame });
}

static Score Pawns(const Position& board) {
    uint64 wp = board.m_WhitePawn, bp = board.m_BlackPawn;
    Score score = { 0, 0 };


    while (wp > 0) {
        int rpos = PopPos(wp);
        score += Pawn<WHITE>(board, MirrorSquare(rpos), rpos);
    }

    while (bp > 0) {
        int pos = PopPos(bp);
        score -= Pawn<BLACK>(board, pos, pos);
    }
    return score;
}

// Relative static evaluation
static int64 Evaluate(const Position& board, PawnTable* table) {
    int64 bareKing = 0;
    if (BareKingScore(board, bareKing)) return board.m_WhiteMove ? bareKing : -bareKing;

    int64 middlegame = 0, endgame = 0, result = 0;
    Score score = { 0, 0 };
    BitBoard wp = board.m_WhitePawn, wkn = board.m_WhiteKnight, wb = board.m_WhiteBishop, wr = board.m_WhiteRook,
             wq = board.m_WhiteQueen, wk = board.m_WhiteKing, bp = board.m_BlackPawn, bkn = board.m_BlackKnight,
             bb = board.m_BlackBishop, br = board.m_BlackRook, bq = board.m_BlackQueen, bk = board.m_BlackKing;

    const int64 pawnVal = PAWN_VALUE, knightVal = KNIGHT_VALUE, bishopVal = BISHOP_VALUE, rookVal = ROOK_VALUE,
                queenVal = QUEEN_VALUE, kingVal = KING_VALUE;

    int64 phase = 4 + 4 + 8 + 8;

    int whiteking = GetSquare(wk), blackking = GetSquare(bk);

    int whiteAttack = 0, blackAttack = 0;

    middlegame += board.m_WhiteMove ? TEMPO : -TEMPO;

    // Calculate material imbalance
    for (int i = PieceType::PAWN; i < PieceType::QUEEN; i++) {
        for (int j = PieceType::PAWN; j < i; j++) {
            // TODO: separate imbalance factor for mg and eg
            middlegame += Lookup::imbalance_factor[i - 1][j - 1]
                          * (CountBits(board.m_Pieces[i][0]) * CountBits(board.m_Pieces[j][0])
                             - CountBits(board.m_Pieces[i][1]) * CountBits(board.m_Pieces[j][1]));
            endgame += Lookup::imbalance_factor[i - 1][j - 1]
                       * (CountBits(board.m_Pieces[i][0]) * CountBits(board.m_Pieces[j][0])
                          - CountBits(board.m_Pieces[i][1]) * CountBits(board.m_Pieces[j][1]));
        }
    }


    // Pawns

    PTEntry* pawnStructure = table->Probe(board.m_PawnHash);
    if (pawnStructure != nullptr) {
        score += pawnStructure->m_Score;
    } else {
        Score pawn = Pawns(board);
        score += pawn;
        table->Enter(board.m_PawnHash, PTEntry(board.m_PawnHash, pawn));
    }

    // Knights
    int wkncnt = 0;
    while (wkn > 0) {
        int rpos = PopPos(wkn);
        int pos = MirrorSquare(rpos);
        middlegame += knightVal + Lookup::knight_table[pos];
        endgame += knightVal + Lookup::knight_table[pos];
        if (uint64 temp = Lookup::b_king_safety[blackking] & Lookup::knight_attacks[rpos]) {
            whiteAttack += 2 * CountBits(temp);
        }
        BitBoard moveable = Lookup::knight_attacks[rpos] & ~board.m_White;
        int knight_mobility = CountBits(moveable);
        middlegame += knight_mobility;
        phase -= 1;
        wkncnt++;
    }

    if (wkncnt == 2) { // Knight pair bonus
        result += 5;
    }

    int bkncnt = 0;
    while (bkn > 0) {
        int pos = PopPos(bkn);
        middlegame -= knightVal + Lookup::knight_table[pos];
        endgame -= knightVal + Lookup::knight_table[pos];
        if (uint64 temp = Lookup::w_king_safety[whiteking] & Lookup::knight_attacks[pos]) {
            blackAttack += 2 * CountBits(temp);
        }
        BitBoard moveable = Lookup::knight_attacks[pos] & ~board.m_Black;
        int knight_mobility = CountBits(moveable);
        middlegame -= knight_mobility;

        phase -= 1;
        bkncnt++;
    }

    if (bkncnt == 2) { // Knight pair bonus
        result -= 5;
    }

    // Bishops
    int wbcnt = 0;
    while (wb > 0) {
        int rpos = PopPos(wb);
        int pos = MirrorSquare(rpos);
        middlegame += bishopVal + Lookup::bishop_table[pos];
        endgame += bishopVal + Lookup::bishop_table[pos];
        BitBoard bish_atk = board.BishopAttack(rpos, board.m_Board);
        if (uint64 temp = Lookup::b_king_safety[blackking] & bish_atk) {
            whiteAttack += 2 * CountBits(temp);
        }

        int bishop_mobility = CountBits(bish_atk & ~board.m_White);
        middlegame += bishop_mobility;

        phase -= 1;
        wbcnt++;
    }

    if (wbcnt == 2) {
        result += 30;
    }

    int bbcnt = 0;
    while (bb > 0) {
        int pos = PopPos(bb);
        middlegame -= bishopVal + Lookup::bishop_table[pos];
        endgame -= bishopVal + Lookup::bishop_table[pos];
        BitBoard bish_atk = board.BishopAttack(pos, board.m_Board);
        if (uint64 temp = Lookup::w_king_safety[whiteking] & bish_atk) {
            blackAttack += 2 * CountBits(temp);
        }

        int bishop_mobility = CountBits(bish_atk & ~board.m_Black);
        middlegame -= bishop_mobility;

        phase -= 1;
        bbcnt++;
    }

    if (bbcnt == 2) {
        result -= 30;
    }

    // Rooks
    int wrcnt = 0;
    while (wr > 0) {
        int rpos = PopPos(wr);
        int pos = MirrorSquare(rpos);
        middlegame += rookVal + Lookup::rook_table[pos];
        endgame += rookVal + Lookup::eg_rook_table[pos];
        BitBoard rook_atk = board.RookAttack(rpos, board.m_Board);
        if (uint64 temp = Lookup::b_king_safety[blackking] & rook_atk) {
            whiteAttack += 3 * CountBits(temp);
        }

        int rook_mobility = CountBits(rook_atk & ~board.m_White);
        middlegame += rook_mobility;

        phase -= 2;
        wrcnt++;
    }

    if (wrcnt == 2) {
        result += 15;
    }

    int brcnt = 0;
    while (br > 0) {
        int pos = PopPos(br);
        middlegame -= rookVal + Lookup::rook_table[pos];
        endgame -= rookVal + Lookup::eg_rook_table[pos];
        BitBoard rook_atk = board.RookAttack(pos, board.m_Board);
        if (uint64 temp = Lookup::w_king_safety[whiteking] & rook_atk) {
            blackAttack += 3 * CountBits(temp);
        }
        int rook_mobility = CountBits(rook_atk & ~board.m_Black);
        middlegame -= rook_mobility;

        phase -= 2;
        brcnt++;
    }

    if (brcnt == 2) {
        result -= 15;
    }

    // Queens
    while (wq > 0) {
        int rpos = PopPos(wq);
        int pos = MirrorSquare(rpos);
        middlegame += queenVal + Lookup::queen_table[pos];
        endgame += queenVal + Lookup::eg_queen_table[pos];
        BitBoard queen_atk = board.QueenAttack(rpos, board.m_Board);
        if (uint64 temp = Lookup::b_king_safety[blackking] & queen_atk) {
            whiteAttack += 5 * CountBits(temp);
        }

        int queen_mobility = CountBits(queen_atk & ~board.m_White);
        middlegame += queen_mobility;

        // Don't develop queen on starting pos too early
        if (rpos == 4) {
            if (board.m_WhiteKnight & 0b10ull) middlegame += 4;
            if (board.m_WhiteKnight & 0b1000000ull) middlegame += 4;
            if (board.m_WhiteBishop & 0b100ull) middlegame += 4;
            if (board.m_WhiteBishop & 0b100000ull) middlegame += 4;
        }
        phase -= 4;
    }

    while (bq > 0) {
        int pos = PopPos(bq);
        middlegame -= queenVal + Lookup::queen_table[pos];
        endgame -= queenVal + Lookup::eg_queen_table[pos];
        phase -= 4;
        BitBoard queen_atk = board.QueenAttack(pos, board.m_Board);
        if (uint64 temp = Lookup::w_king_safety[whiteking] & queen_atk) {
            blackAttack += 5 * CountBits(temp);
        }
        int queen_mobility = CountBits(queen_atk & ~board.m_Black);
        middlegame -= queen_mobility;

        // Don't develop queen on starting pos too early
        if (pos == 60) {
            if (board.m_BlackKnight & 0b10ull << 56) middlegame -= 4;
            if (board.m_BlackKnight & 0b1000000ull << 56) middlegame -= 4;
            if (board.m_BlackBishop & 0b100ull << 56) middlegame -= 4;
            if (board.m_BlackBishop & 0b100000ull << 56) middlegame -= 4;
        }
    }

    // Kings
    if (wk > 0) {
        int pos = MirrorSquare(whiteking);
        middlegame += kingVal + Lookup::king_table[pos] + Lookup::king_safetyindex[whiteAttack];
        endgame += kingVal + Lookup::eg_king_table[pos];
    }

    if (bk > 0) {
        middlegame -= kingVal + Lookup::king_table[blackking] + Lookup::king_safetyindex[blackAttack];
        endgame -= kingVal + Lookup::eg_king_table[blackking];
    }

    middlegame += score.mg;
    endgame += score.eg;

    phase = (phase * 256) / 24;
    result += (middlegame * (256 - phase) + endgame * phase) / 256;
    return board.m_WhiteMove ? result : -result;
}
