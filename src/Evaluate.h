#pragma once

#include "Movelist.h"

#define TEMPO 20


template<bool white>
static Score Pawn(const Board& board, int pos, int rpos) {
    const int64 pawnBaseVal = 100;

    int middlegame = pawnBaseVal + Lookup::pawn_table[pos];
    int endgame = pawnBaseVal + Lookup::eg_pawn_table[pos];
    if ((Lookup::pawn_passed<white>(rpos) & Pawn<!white>(board)) == 0) { // pawn is passed
        int64 bit = 1ull << rpos;
        if ((PawnAttackRight<white>(bit) & Pawn<white>(board)) || (PawnAttackLeft<white>(bit) & Pawn<white>(board))) { // If passed pawn is defended too
            endgame += Lookup::passed_pawn_table[pos] * 1.3;
            middlegame += 20 * 1.3;
        } else {
            endgame += Lookup::passed_pawn_table[pos];
            middlegame += 20;
        }
    }
    if ((Lookup::pawn_forward<white>(pos) & Pawn<white>(board)) != 0) { // if doubled
        endgame -= 50;
        middlegame -= 15;
    }
    if ((Lookup::isolated_mask[pos] & Pawn<white>(board)) == 0) {
        middlegame -= 3;
        endgame -= 15;
    }
    return Score({ middlegame, endgame });
}

static Score Pawns(const Board& board) {
    uint64 wp = board.m_WhitePawn, bp = board.m_BlackPawn;
    Score score = { 0, 0 };


    while (wp > 0) {
        int rpos = PopPos(wp);
        score += Pawn<true>(board, 63 - rpos, rpos);
    }

    while (bp > 0) {
        int pos = PopPos(bp);
        score -= Pawn<false>(board, pos, pos);
    }
    return score;
}

// Relative static evaluation
static int64 Evaluate(const Board& board, PawnTable* table, uint64 pawnhash, bool white) {

    int64 middlegame = 0, endgame = 0, result = 0;
    Score score = { 0, 0 };
    BitBoard wp = board.m_WhitePawn, wkn = board.m_WhiteKnight, wb = board.m_WhiteBishop, wr = board.m_WhiteRook, wq = board.m_WhiteQueen, wk = board.m_WhiteKing,
        bp = board.m_BlackPawn, bkn = board.m_BlackKnight, bb = board.m_BlackBishop, br = board.m_BlackRook, bq = board.m_BlackQueen, bk = board.m_BlackKing;

    const int64 pawnVal = 100, knightVal = 350, bishopVal = 350, rookVal = 525, queenVal = 1000, kingVal = 10000;

    int64 phase = 4 + 4 + 8 + 8;

    int whiteking = GET_SQUARE(wk), blackking = GET_SQUARE(bk);

    int whiteAttack = 0, blackAttack = 0;

    middlegame += white ? TEMPO : -TEMPO;

    // Calculate material imbalance
    for (int i = PieceType::PAWN; i < PieceType::QUEEN; i++) {
        for (int j = PieceType::PAWN; j < i; j++) {
            // TODO: separate imbalance factor for mg and eg
            middlegame += Lookup::imbalance_factor[i-1][j-1]*(COUNT_BIT(board.m_Pieces[i][0]) * COUNT_BIT(board.m_Pieces[j][0])
                - COUNT_BIT(board.m_Pieces[i][1])* COUNT_BIT(board.m_Pieces[j][1]));
            endgame += Lookup::imbalance_factor[i][j] * (COUNT_BIT(board.m_Pieces[i][0]) * COUNT_BIT(board.m_Pieces[j][0])
                - COUNT_BIT(board.m_Pieces[i][1]) * COUNT_BIT(board.m_Pieces[j][1]));
        }
    }


    // Pawns

    PTEntry* pawnStructure = table->Probe(pawnhash);
    if (pawnStructure != nullptr) {
        score += pawnStructure->m_Score;
    } else {
        Score pawn = Pawns(board);
        table->Enter(pawnhash, PTEntry(pawnhash, pawn));
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
        BitBoard bish_atk = board.BishopAttack(rpos, board.m_Board);
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
        BitBoard bish_atk = board.BishopAttack(pos, board.m_Board);
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
        BitBoard rook_atk = board.RookAttack(rpos, board.m_Board);
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
        BitBoard rook_atk = board.RookAttack(pos, board.m_Board);
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
        BitBoard queen_atk = board.QueenAttack(rpos, board.m_Board);
        if (uint64 temp = Lookup::b_king_safety[blackking] & queen_atk) {
            whiteAttack += 5 * COUNT_BIT(temp);
        }
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

    middlegame += score.mg;
    endgame += score.eg;

    phase = (phase * 256) / 16;
    result += (middlegame * (256 - phase) + endgame * phase) / 256;
    return white ? result : -result;
}