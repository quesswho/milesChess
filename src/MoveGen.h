#pragma once
#include "Position.h"


enum MoveGenType {
	ALL,
    QUIESCENCE
};

template<Color white>
static BitBoard TCheck(const Position& board, BitBoard& danger, BitBoard& active, BitBoard& rookPin, BitBoard& bishopPin, BitBoard& enPassant) {
    constexpr Color enemy = !white;

    int kingsq = GET_SQUARE(King<white>(board));

    BitBoard knightPawnCheck = Lookup::knight_attacks[kingsq] & Knight<enemy>(board);


    BitBoard pr = PawnRight<enemy>(board) & King<white>(board);
    BitBoard pl = PawnLeft<enemy>(board) & King<white>(board);

    knightPawnCheck |= PawnAttackLeft<white>(pr); // Reverse the pawn attack to find the attacking pawn
    knightPawnCheck |= PawnAttackRight<white>(pl);

    BitBoard enPassantCheck = PawnForward<white>((PawnAttackLeft<white>(pr) | PawnAttackRight<white>(pl))) & enPassant;

    danger = PawnAttack<enemy>(board);

    // Active moves, mask for checks
    active = 0xFFFFFFFFFFFFFFFFull; // No checks
    BitBoard rookcheck = board.RookAttack(kingsq, board.m_Board) & (Rook<enemy>(board) | Queen<enemy>(board));
    if (rookcheck > 0) { // If a rook piece is attacking the king
        active = Lookup::active_moves[kingsq * 64 + GET_SQUARE(rookcheck)];
        danger |= Lookup::check_mask[kingsq * 64 + GET_SQUARE(rookcheck)] & ~rookcheck;
    }
    rookPin = 0;
    BitBoard rookpinner = board.RookXray(kingsq, board.m_Board) & (Rook<enemy>(board) | Queen<enemy>(board));
    while (rookpinner > 0) {
        int pos = PopPos(rookpinner);
        BitBoard mask = Lookup::active_moves[kingsq * 64 + pos];
        if (mask & Player<white>(board)) rookPin |= mask;
    }

    BitBoard bishopcheck = board.BishopAttack(kingsq, board.m_Board) & (Bishop<enemy>(board) | Queen<enemy>(board));
    if (bishopcheck > 0) { // If a bishop piece is attacking the king
        if (active != 0xFFFFFFFFFFFFFFFFull) { // case where rook and bishop checks king
            active = 0;
        } else {
            active = Lookup::active_moves[kingsq * 64 + GET_SQUARE(bishopcheck)];
        }
        danger |= Lookup::check_mask[kingsq * 64 + GET_SQUARE(bishopcheck)] & ~bishopcheck;
    }

    bishopPin = 0;
    BitBoard bishoppinner = board.BishopXray(kingsq, board.m_Board) & (Bishop<enemy>(board) | Queen<enemy>(board));
    while (bishoppinner > 0) {
        int pos = PopPos(bishoppinner);
        BitBoard mask = Lookup::active_moves[kingsq * 64 + pos];
        if (mask & Player<white>(board)) {
            bishopPin |= mask;
        }
    }

    if (enPassant) {
        if ((King<white>(board) & Lookup::EnPassantRank<white>()) && (Pawn<white>(board) & Lookup::EnPassantRank<white>()) && ((Rook<enemy>(board) | Queen<enemy>(board)) & Lookup::EnPassantRank<white>())) {
            BitBoard REPawn = PawnRight<white>(board) & enPassant;
            BitBoard LEPawn = PawnLeft<white>(board) & enPassant;
            if (REPawn) {
                BitBoard noEPPawn = board.m_Board & ~(PawnForward<enemy>(enPassant) | PawnAttackLeft<enemy>(REPawn)); // Remove en passanter and en passant target
                if (board.RookAttack(kingsq, noEPPawn) & (Rook<enemy>(board) | Queen<enemy>(board))) enPassant = 0; // If there is a rook or queen attacking king after removing pawns
            }
            if (LEPawn) {
                BitBoard noEPPawn = board.m_Board & ~(PawnForward<enemy>(enPassant) | PawnAttackRight<enemy>(LEPawn)); // Remove en passanter and en passant target
                if (board.RookAttack(kingsq, noEPPawn) & (Rook<enemy>(board) | Queen<enemy>(board))) enPassant = 0; // If there is a rook or queen attacking king after removing pawns
            }
        }
    }

    if (knightPawnCheck && (active != 0xFFFFFFFFFFFFFFFFull)) { // If double checked we have to move the king
        active = 0;
    } else if (knightPawnCheck && active == 0xFFFFFFFFFFFFFFFFull) {
        active = knightPawnCheck;
    }



    BitBoard knights = Knight<enemy>(board);

    while (knights > 0) { // Loop each bit
        danger |= Lookup::knight_attacks[PopPos(knights)];
    }

    BitBoard bishops = Bishop<enemy>(board) | Queen<enemy>(board);
    while (bishops > 0) { // Loop each bit
        int pos = PopPos(bishops);
        danger |= (board.BishopAttack(pos, board.m_Board) & ~(1ull << pos));
    }
    BitBoard rooks = Rook<enemy>(board) | Queen<enemy>(board);

    while (rooks > 0) { // Loop each bit
        int pos = PopPos(rooks);
        danger |= (board.RookAttack(pos, board.m_Board) & ~(1ull << pos));
    }

    danger |= Lookup::king_attacks[GET_SQUARE(King<enemy>(board))];

    return enPassantCheck;
}

static BitBoard Check(Color white, const Position& board, BitBoard& danger, BitBoard& active, BitBoard& rookPin, BitBoard& bishopPin, BitBoard& enPassant) {
    return white ? TCheck<WHITE>(board, danger, active, rookPin, bishopPin, enPassant) : TCheck<BLACK>(board, danger, active, rookPin, bishopPin, enPassant);
}

template<Color enemy>
ColoredPieceType GetCaptureType(const Position& board, uint64 bit) {
    if (Pawn<enemy>(board) & bit) {
        return GetColoredPiece<enemy>(PAWN);
    } else if (Knight<enemy>(board) & bit) {
        return GetColoredPiece<enemy>(KNIGHT);
    } else if (Bishop<enemy>(board) & bit) {
        return GetColoredPiece<enemy>(BISHOP);
    } else if (Rook<enemy>(board) & bit) {
        return GetColoredPiece<enemy>(ROOK);
    } else if (Queen<enemy>(board) & bit) {
        return GetColoredPiece<enemy>(QUEEN);
    } else {
        return GetColoredPiece<enemy>(NONE);
    }
}

template<Color white>
static inline int64 CastleKing(uint8 castle, uint64 danger, uint64 board, uint64 rooks) {
    if constexpr (white) {
        return (((castle & 1) && (board & 0b110) == 0 && (danger & 0b1110) == 0) && (rooks & 0b1) > 0) * (1ull << 1);
    } else {
        return ((castle & 0b100) && ((board & (0b110ull << 56)) == 0) && ((danger & (0b1110ull << 56)) == 0) && (rooks & 0b1ull << 56) > 0) * (1ull << 57);
    }
}

template<Color white>
static inline int64 CastleQueen(uint8 castle, uint64 danger, uint64 board, uint64 rooks) {
    if constexpr (white) {
        return (((castle & 0b10) && (board & 0b01110000) == 0 && (danger & 0b00111000) == 0) && (rooks & 0b10000000) > 0) * (1ull << 5);
    } else {
        return ((castle & 0b1000) && ((board & (0b01110000ull << 56)) == 0) && ((danger & (0b00111000ull << 56)) == 0) && (rooks & (0b10000000ull << 56)) > 0) * (1ull << 61);
    }
}

template<MoveGenType T, Color white>
static std::vector<Move> TGenerateMoves(const Position& board) {
    std::vector<Move> result;
    result.reserve(64); // Pre allocation increases perft speed by almost 3x

    constexpr Color enemy = !white;
    BitBoard danger = 0, active = 0, rookPin = 0, bishopPin = 0, enPassant = board.m_States[board.m_Ply].m_EnPassant;
    BitBoard enPassantCheck = TCheck<white>(board, danger, active, rookPin, bishopPin, enPassant);
    BitBoard moveable = ~Player<white>(board) & active;

    constexpr ColoredPieceType pawntype = GetColoredPiece<white>(PAWN);
    constexpr ColoredPieceType knighttype = GetColoredPiece<white>(KNIGHT);
    constexpr ColoredPieceType bishoptype = GetColoredPiece<white>(BISHOP);
    constexpr ColoredPieceType rooktype = GetColoredPiece<white>(ROOK);
    constexpr ColoredPieceType queentype = GetColoredPiece<white>(QUEEN);
    constexpr ColoredPieceType kingtype = GetColoredPiece<white>(KING);

    // Pawns

    BitBoard pawns = Pawn<white>(board);
    BitBoard nonBishopPawn = pawns & ~bishopPin;

    BitBoard FPawns = PawnForward<white>(nonBishopPawn) & (~board.m_Board) & active; // No diagonally pinned pawn can move forward

    if constexpr (T != QUIESCENCE) {
        BitBoard F2Pawns = PawnForward<white>(nonBishopPawn & Lookup::StartingPawnRank<white>()) & ~board.m_Board;
        F2Pawns = PawnForward<white>(F2Pawns) & (~board.m_Board) & active; // TODO: Use Fpawns to save calculation

        BitBoard pinnedF2Pawns = F2Pawns & Pawn2Forward<white>(rookPin);
        BitBoard movePinF2Pawns = F2Pawns & rookPin;
        F2Pawns = F2Pawns & (~pinnedF2Pawns | movePinF2Pawns);

        while (F2Pawns > 0) { // Loop each bit
            const BoardPos pos = PopPos(F2Pawns);
            result.push_back(BuildMove(PawnPos2Forward<enemy>(pos), pos, pawntype));
        }
    }

    BitBoard pinnedFPawns = FPawns & PawnForward<white>(rookPin);
    BitBoard movePinFPawns = FPawns & rookPin; // Pinned after moving
    FPawns = FPawns & (~pinnedFPawns | movePinFPawns);

    const BitBoard nonRookPawns = pawns & ~rookPin;
    BitBoard RPawns = PawnAttackRight<white>(nonRookPawns) & Enemy<white>(board) & active;
    BitBoard LPawns = PawnAttackLeft<white>(nonRookPawns) & Enemy<white>(board) & active;

    BitBoard pinnedRPawns = RPawns & PawnAttackRight<white>(bishopPin);
    BitBoard pinnedLPawns = LPawns & PawnAttackLeft<white>(bishopPin);
    BitBoard movePinRPawns = RPawns & bishopPin; // is it pinned after move
    BitBoard movePinLPawns = LPawns & bishopPin;
    RPawns = RPawns & (~pinnedRPawns | movePinRPawns);
    LPawns = LPawns & (~pinnedLPawns | movePinLPawns);

    if ((FPawns | RPawns | LPawns) & FirstRank<enemy>()) { // Reduces two conditional statements
        uint64 FPromote = FPawns & FirstRank<enemy>();
        uint64 RPromote = RPawns & FirstRank<enemy>();
        uint64 LPromote = LPawns & FirstRank<enemy>();

        FPawns = FPawns & ~FirstRank<enemy>();
        RPawns = RPawns & ~FirstRank<enemy>();
        LPawns = LPawns & ~FirstRank<enemy>();

        // Add Forward pawns
        while (FPromote > 0) {
            const BoardPos pos = PopPos(FPromote);
            result.push_back(BuildMove(PawnPosForward<enemy>(pos), pos, pawntype, NOPIECE, 0b000100));
            result.push_back(BuildMove(PawnPosForward<enemy>(pos), pos, pawntype, NOPIECE, 0b001000));
            result.push_back(BuildMove(PawnPosForward<enemy>(pos), pos, pawntype, NOPIECE, 0b010000));
            result.push_back(BuildMove(PawnPosForward<enemy>(pos), pos, pawntype, NOPIECE, 0b100000));
        }

        while (RPromote > 0) { // Loop each bit
            const BoardPos pos = PopPos(RPromote);
            const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << pos);
            result.push_back(BuildMove(PawnPosLeft<enemy>(pos), pos, pawntype, capture, 0b000100));
            result.push_back(BuildMove(PawnPosLeft<enemy>(pos), pos, pawntype, capture, 0b001000));
            result.push_back(BuildMove(PawnPosLeft<enemy>(pos), pos, pawntype, capture, 0b010000));
            result.push_back(BuildMove(PawnPosLeft<enemy>(pos), pos, pawntype, capture, 0b100000));
        }

        while (LPromote > 0) {
            const BoardPos pos = PopPos(LPromote);
            const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << pos);
            result.push_back(BuildMove(PawnPosRight<enemy>(pos), pos, pawntype, capture, 0b000100));
            result.push_back(BuildMove(PawnPosRight<enemy>(pos), pos, pawntype, capture, 0b001000));
            result.push_back(BuildMove(PawnPosRight<enemy>(pos), pos, pawntype, capture, 0b010000));
            result.push_back(BuildMove(PawnPosRight<enemy>(pos), pos, pawntype, capture, 0b100000));
        }
    }

    if constexpr (T != QUIESCENCE) {
        while (FPawns > 0) {
            const BoardPos pos = PopPos(FPawns);
            result.push_back(BuildMove(PawnPosForward<enemy>(pos), pos, pawntype));
        }
    }

    while (RPawns > 0) { // Loop each bit
        const BoardPos pos = PopPos(RPawns);
        const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << pos);
        result.push_back(BuildMove(PawnPosLeft<enemy>(pos), pos, pawntype, capture));
    }

    while (LPawns > 0) {
        const BoardPos pos = PopPos(LPawns);
        const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << pos);
        result.push_back(BuildMove(PawnPosRight<enemy>(pos), pos, pawntype, capture));
    }

    // En passant

    BitBoard REPawns = PawnAttackRight<white>(nonRookPawns) & enPassant & (active | enPassantCheck);
    BitBoard LEPawns = PawnAttackLeft<white>(nonRookPawns) & enPassant & (active | enPassantCheck);
    BitBoard pinnedREPawns = REPawns & PawnAttackRight<white>(bishopPin);
    BitBoard stillPinREPawns = REPawns & bishopPin;
    REPawns = REPawns & (~pinnedREPawns | stillPinREPawns);
    if (REPawns > 0) {
        const BoardPos pos = PopPos(REPawns);
        result.push_back(BuildMove(PawnPosLeft<enemy>(pos), pos, pawntype, GetColoredPiece<enemy>(PAWN), 0b000001));
    }

    BitBoard pinnedLEPawns = LEPawns & PawnAttackLeft<white>(bishopPin);
    BitBoard stillPinLEPawns = LEPawns & bishopPin;
    LEPawns = LEPawns & (~pinnedLEPawns | stillPinLEPawns);
    if (LEPawns > 0) {
        const BoardPos pos = PopPos(LEPawns);
        result.push_back(BuildMove(PawnPosRight<enemy>(pos), pos, pawntype, GetColoredPiece<enemy>(PAWN), 0b000001));
    }

    // Kinghts

    BitBoard knights = Knight<white>(board) & ~(rookPin | bishopPin); // Pinned knights can not move

    while (knights > 0) { // Loop each bit
        int pos = PopPos(knights);
        BitBoard moves = Lookup::knight_attacks[pos] & moveable;
        BitBoard qmove = moves & Enemy<white>(board);
        BitBoard smove = moves & ~Enemy<white>(board);
        while (qmove > 0) { // Loop each bit
            const BoardPos toPos = PopPos(qmove);
            const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << toPos);
            result.push_back(BuildMove(pos, toPos, knighttype, capture));
        }
        if constexpr (T != QUIESCENCE) {
            while (smove > 0) { // Loop each bit
                const BoardPos toPos = PopPos(smove);
                result.push_back(BuildMove(pos, toPos, knighttype));
            }
        }
    }

    // Bishops

    BitBoard bishops = Bishop<white>(board) & ~rookPin; // A rook pinned bishop can not move

    BitBoard pinnedBishop = bishopPin & bishops;
    BitBoard notPinnedBishop = bishops ^ pinnedBishop;

    while (pinnedBishop > 0) {
        int pos = PopPos(pinnedBishop);
        BitBoard moves = board.BishopAttack(pos, board.m_Board) & moveable & bishopPin;
        BitBoard qmove = moves & Enemy<white>(board);
        BitBoard smove = moves & ~Enemy<white>(board);
        while (qmove > 0) { // Loop each bit
            const BoardPos toPos = PopPos(qmove);
            const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << toPos);
            result.push_back(BuildMove(pos, toPos, bishoptype, capture));
        }
        if constexpr (T != QUIESCENCE) {
            while (smove > 0) { // Loop each bit
                const BoardPos toPos = PopPos(smove);
                result.push_back(BuildMove(pos, toPos, bishoptype));
            }
        }
    }

    while (notPinnedBishop > 0) {
        int pos = PopPos(notPinnedBishop);
        BitBoard moves = board.BishopAttack(pos, board.m_Board) & moveable;
        BitBoard qmove = moves & Enemy<white>(board);
        BitBoard smove = moves & ~Enemy<white>(board);
        while (qmove > 0) { // Loop each bit
            const BoardPos toPos = PopPos(qmove);
            const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << toPos);
            result.push_back(BuildMove(pos, toPos, bishoptype, capture));
        }
        if constexpr (T != QUIESCENCE) {
            while (smove > 0) { // Loop each bit
                const BoardPos toPos = PopPos(smove);
                result.push_back(BuildMove(pos, toPos, bishoptype));
            }
        }
    }

    // Rooks
    BitBoard rooks = Rook<white>(board) & ~bishopPin; // A bishop pinned rook can not move

    // Castles
    const BoardPos kingpos = GET_SQUARE(King<white>(board));
    BitBoard CK = CastleKing<white>(board.m_States[board.m_Ply].m_CastleRights, danger, board.m_Board, rooks);
    BitBoard CQ = CastleQueen<white>(board.m_States[board.m_Ply].m_CastleRights, danger, board.m_Board, rooks);
    if (CK) {
        result.push_back(BuildMove(kingpos, GET_SQUARE(CK), kingtype, NOPIECE, 0b000010));
    }
    if (CQ) {
        result.push_back(BuildMove(kingpos, GET_SQUARE(CQ), kingtype, NOPIECE, 0b000010));
    }

    BitBoard pinnedRook = rookPin & (rooks);
    BitBoard notPinnedRook = rooks ^ pinnedRook;

    while (pinnedRook > 0) {
        int pos = PopPos(pinnedRook);
        BitBoard moves = board.RookAttack(pos, board.m_Board) & moveable & rookPin;
        BitBoard qmove = moves & Enemy<white>(board);
        BitBoard smove = moves & ~Enemy<white>(board);
        while (qmove > 0) { // Loop each bit
            const BoardPos toPos = PopPos(qmove);
            const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << toPos);
            result.push_back(BuildMove(pos, toPos, rooktype, capture));
        }
        if constexpr (T != QUIESCENCE) {
            while (smove > 0) { // Loop each bit
                const BoardPos toPos = PopPos(smove);
                result.push_back(BuildMove(pos, toPos, rooktype));
            }
        }
    }

    while (notPinnedRook > 0) {
        int pos = PopPos(notPinnedRook);
        BitBoard moves = board.RookAttack(pos, board.m_Board) & moveable;
        BitBoard qmove = moves & Enemy<white>(board);
        BitBoard smove = moves & ~Enemy<white>(board);
        while (qmove > 0) { // Loop each bit
            const BoardPos toPos = PopPos(qmove);
            const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << toPos);
            result.push_back(BuildMove(pos, toPos, rooktype, capture));
        }
        if constexpr (T != QUIESCENCE) {
            while (smove > 0) { // Loop each bit
                const BoardPos toPos = PopPos(smove);
                result.push_back(BuildMove(pos, toPos, rooktype));
            }
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
        BitBoard qmove = moves & Enemy<white>(board);
        BitBoard smove = moves & ~Enemy<white>(board);
        while (qmove > 0) { // Loop each bit
            const BoardPos toPos = PopPos(qmove);
            const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << toPos);
            result.push_back(BuildMove(pos, toPos, queentype, capture));
        }
        if constexpr (T != QUIESCENCE) {
            while (smove > 0) { // Loop each bit
                const BoardPos toPos = PopPos(smove);
                result.push_back(BuildMove(pos, toPos, queentype));
            }
        }
    }

    // TODO: Put this in bishop and rook loops instead perhaps
    while (pinnedDiagonalQueen > 0) {
        int pos = PopPos(pinnedDiagonalQueen);
        BitBoard moves = board.BishopAttack(pos, board.m_Board) & moveable & bishopPin;
        BitBoard qmove = moves & Enemy<white>(board);
        BitBoard smove = moves & ~Enemy<white>(board);
        while (qmove > 0) { // Loop each bit
            const BoardPos toPos = PopPos(qmove);
            const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << toPos);
            result.push_back(BuildMove(pos, toPos, queentype, capture));
        }
        if constexpr (T != QUIESCENCE) {
            while (smove > 0) { // Loop each bit
                const BoardPos toPos = PopPos(smove);
                result.push_back(BuildMove(pos, toPos, queentype));
            }
        }
    }

    while (notPinnedQueen > 0) {
        int pos = PopPos(notPinnedQueen);
        BitBoard moves = board.QueenAttack(pos, board.m_Board) & moveable;
        BitBoard qmove = moves & Enemy<white>(board);
        BitBoard smove = moves & ~Enemy<white>(board);
        while (qmove > 0) { // Loop each bit
            const BoardPos toPos = PopPos(qmove);
            const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << toPos);
            result.push_back(BuildMove(pos, toPos, queentype, capture));
        }
        if constexpr (T != QUIESCENCE) {
            while (smove > 0) { // Loop each bit
                const BoardPos toPos = PopPos(smove);
                result.push_back(BuildMove(pos, toPos, queentype));
            }
        }
    }

    // King

    BitBoard kmoves = Lookup::king_attacks[kingpos] & ~Player<white>(board);
    kmoves &= ~danger;

    BitBoard qkmove = kmoves & Enemy<white>(board);
    BitBoard skmove = kmoves & ~Enemy<white>(board);
    while (qkmove > 0) { // Loop each bit
        const BoardPos toPos = PopPos(qkmove);
        const ColoredPieceType capture = GetCaptureType<enemy>(board, 1ull << toPos);
        result.push_back(BuildMove(kingpos, toPos, kingtype, capture));
    }
    if constexpr (T != QUIESCENCE) {
        while (skmove > 0) { // Loop each bit
            const BoardPos toPos = PopPos(skmove);
            result.push_back(BuildMove(kingpos, toPos, kingtype));
        }
    }


    return result;
}

template<MoveGenType T>
static inline std::vector<Move> GenerateMoves(const Position& board) {
    return board.m_WhiteMove ? TGenerateMoves<T, WHITE>(board) : TGenerateMoves<T, BLACK>(board);
}

template<MoveGenType T>
class MoveGen {
private:
	Position& m_Position;
	Move m_HashMove;
public:
	MoveGen(Position& position, Move hashmove);

};