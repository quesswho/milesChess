#pragma once

#include "Movelist.h"

#define TEMPO 20

// Relative static evaluation
static int64 Evaluate(const Board& board, bool white) {

    int64 middlegame = 0, endgame = 0, result = 0;

    uint64 wp = board.m_WhitePawn, wkn = board.m_WhiteKnight, wb = board.m_WhiteBishop, wr = board.m_WhiteRook, wq = board.m_WhiteQueen, wk = board.m_WhiteKing,
        bp = board.m_BlackPawn, bkn = board.m_BlackKnight, bb = board.m_BlackBishop, br = board.m_BlackRook, bq = board.m_BlackQueen, bk = board.m_BlackKing;

    const int64 pawnVal = 100, knightVal = 350, bishopVal = 350, rookVal = 525, queenVal = 1000, kingVal = 10000;

    int64 phase = 4 + 4 + 8 + 8;

    int whiteking = GET_SQUARE(wk), blackking = GET_SQUARE(bk);

    int whiteAttack = 0, blackAttack = 0;

    middlegame += white ? TEMPO : -TEMPO;

    // Pawns
    while (wp > 0) {
        int rpos = PopPos(wp);
        int pos = 63 - rpos;
        middlegame += pawnVal + Lookup::pawn_table[pos];
        endgame += pawnVal + Lookup::eg_pawn_table[pos];
        if ((Lookup::white_passed[rpos] & board.m_BlackPawn) == 0) { // pawn is passed
            int64 bit = 1ull << rpos;
            if ((PawnAttackRight<false>(bit) & board.m_WhitePawn) || (PawnAttackLeft<false>(bit) & board.m_WhitePawn)) { // If passed pawn is defended too
                endgame += Lookup::passed_pawn_table[pos] * 1.3;
                middlegame += 20 * 1.3;
            } else {
                endgame += Lookup::passed_pawn_table[pos];
                middlegame += 20;
            }
        }
        if ((Lookup::white_forward[pos] & board.m_WhitePawn) != 0) { // if doubled
            endgame -= 20;
            middlegame -= 10;
        }
    }

    while (bp > 0) {
        int pos = PopPos(bp);
        middlegame -= pawnVal + Lookup::pawn_table[pos];
        endgame -= pawnVal + Lookup::eg_pawn_table[pos];
        if ((Lookup::black_passed[pos] & board.m_WhitePawn) == 0) { // pawn is passed
            int64 bit = 1ull << pos;
            if ((PawnAttackRight<true>(bit) & board.m_BlackPawn) || (PawnAttackLeft<true>(bit) & board.m_BlackPawn)) { // If passed pawn is defended too
                endgame -= Lookup::passed_pawn_table[pos] * 1.3;
                middlegame -= 20 * 1.3;
            } else {
                endgame -= Lookup::passed_pawn_table[pos];
                middlegame -= 20;
            }
        }
        if ((Lookup::black_forward[pos] & board.m_BlackPawn) != 0) { // if doubled
            endgame -= 20;
            middlegame -= 10;
        }
    }

    // Knights
    int wkncnt = 0;
    while (wkn > 0) {
        int rpos = PopPos(wkn);
        int pos = 63 - rpos;
        middlegame += knightVal + Lookup::knight_table[pos];
        endgame += knightVal + Lookup::knight_table[pos];
        if (uint64 temp = Lookup::b_king_safety[blackking] & Lookup::knight_attacks[rpos]) {
            whiteAttack += 2 * COUNT_BIT(temp);
        }
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
            blackAttack += 2 * COUNT_BIT(temp);
        }
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
        int pos = 63 - rpos;
        middlegame += bishopVal + Lookup::bishop_table[pos];
        endgame += bishopVal + Lookup::bishop_table[pos];
        uint64 bish_atk = board.BishopAttack(rpos, board.m_Board);
        if (uint64 temp = Lookup::b_king_safety[blackking] & bish_atk) {
            whiteAttack += 2 * COUNT_BIT(temp);
        }
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
        uint64 bish_atk = board.BishopAttack(pos, board.m_Board);
        if (uint64 temp = Lookup::w_king_safety[whiteking] & bish_atk) {
            blackAttack += 2 * COUNT_BIT(temp);
        }
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
        int pos = 63 - rpos;
        middlegame += rookVal + Lookup::rook_table[pos];
        endgame += rookVal + Lookup::eg_rook_table[pos];
        uint64 rook_atk = board.RookAttack(rpos, board.m_Board);
        if (uint64 temp = Lookup::b_king_safety[blackking] & rook_atk) {
            whiteAttack += 3 * COUNT_BIT(temp);
        }
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
        uint64 rook_atk = board.RookAttack(pos, board.m_Board);
        if (uint64 temp = Lookup::w_king_safety[whiteking] & rook_atk) {
            blackAttack += 3 * COUNT_BIT(temp);
        }
        phase -= 2;
        brcnt++;
    }

    if (brcnt == 2) {
        result -= 15;
    }

    // Queens
    while (wq > 0) {
        int rpos = PopPos(wq);
        int pos = 63 - rpos;
        middlegame += queenVal + Lookup::queen_table[pos];
        endgame += queenVal + Lookup::eg_queen_table[pos];
        uint64 queen_atk = board.QueenAttack(rpos, board.m_Board);
        if (uint64 temp = Lookup::b_king_safety[blackking] & queen_atk) {
            whiteAttack += 5 * COUNT_BIT(temp);
        }
        // Don't develop queen on starting pos too early
        if (rpos == 4) {
            if (board.m_WhiteKnight & 0b10ull) middlegame += 2;
            if (board.m_WhiteKnight & 0b1000000ull) middlegame += 2;
            if (board.m_WhiteBishop & 0b100ull) middlegame += 2;
            if (board.m_WhiteBishop & 0b100000ull) middlegame += 2;
        }
        phase -= 4;
    }

    while (bq > 0) {
        int pos = PopPos(bq);
        middlegame -= queenVal + Lookup::queen_table[pos];
        endgame -= queenVal + Lookup::eg_queen_table[pos];
        phase -= 4;
        uint64 queen_atk = board.QueenAttack(pos, board.m_Board);
        if (uint64 temp = Lookup::king_attacks[whiteking] & queen_atk) {
            blackAttack += 5 * COUNT_BIT(temp);
        }
        // Don't develop queen on starting pos too early
        if (pos == 60) {
            if (board.m_BlackKnight & 0b10ull << 56) middlegame -= 2;
            if (board.m_BlackKnight & 0b1000000ull << 56) middlegame -= 2;
            if (board.m_BlackBishop & 0b100ull << 56) middlegame -= 2;
            if (board.m_BlackBishop & 0b100000ull << 56) middlegame -= 2;
        }
    }

    // Kings
    if (wk > 0) {
        int pos = 63 - whiteking;
        middlegame += kingVal + Lookup::king_table[pos] + Lookup::king_safetyindex[whiteAttack];
        endgame += kingVal + Lookup::eg_king_table[pos];
    }

    if (bk > 0) {
        middlegame -= kingVal + Lookup::king_table[blackking] + Lookup::king_safetyindex[blackAttack];
        endgame -= kingVal + Lookup::eg_king_table[blackking];
    }

    phase = (phase * 256) / 16;
    result += (middlegame * (256 - phase) + endgame * phase) / 256;
    return white ? result : -result;
}