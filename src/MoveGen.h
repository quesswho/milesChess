#pragma once

#include "Movelist.h"

class MoveGen {
public:
    
    uint64 Check(const Board& board, uint64& danger, uint64& active, uint64& rookPin, uint64& bishopPin, uint64& enPassant) const {
        const bool white = board.m_BoardInfo.m_WhiteMove;
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

    std::vector<Move> GenerateMoves(const Board& board) {
        std::vector<Move> result;

        const bool white = board.m_BoardInfo.m_WhiteMove;
        const bool enemy = !white;
        uint64 danger = 0, active = 0, rookPin = 0, bishopPin = 0, enPassant = board.m_BoardInfo.m_EnPassant;
        uint64 enPassantCheck = Check(board, danger, active, rookPin, bishopPin, enPassant);
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
            if ((bit & Lookup::FirstRank(enemy)) > 0) { // If promote
                result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::P_KNIGHT });
                result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::P_BISHOP });
                result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::P_ROOK });
                result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::P_QUEEN });
            }
            else {
                result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::PAWN });
            }
        }

        for (; uint64 bit = PopBit(LPawns); LPawns > 0) { // Loop each bit
            if ((bit & Lookup::FirstRank(enemy)) > 0) { // If promote
                result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::P_KNIGHT });
                result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::P_BISHOP });
                result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::P_ROOK });
                result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::P_QUEEN });
            }
            else {
                result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::PAWN });
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
            result.push_back({ PawnAttackLeft(bit, enemy), bit, MoveType::EPASSANT });
        }

        uint64 pinnedLEPawns = LEPawns & PawnAttackLeft(bishopPin, white);
        uint64 stillPinLEPawns = LEPawns & bishopPin;
        LEPawns = LEPawns & (~pinnedLEPawns | stillPinLEPawns);
        if (LEPawns > 0) {
            uint64 bit = PopBit(LEPawns);
            result.push_back({ PawnAttackRight(bit, enemy), bit, MoveType::EPASSANT });
        }



        // Kinghts

        uint64 knights = Knight(board, white) & ~(rookPin | bishopPin); // Pinned knights can not move

        while (knights > 0) { // Loop each bit
            int pos = PopPos(knights);
            uint64 moves = Lookup::knight_attacks[pos] & moveable;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                result.push_back({ 1ull << pos, bit, MoveType::KNIGHT });
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
                result.push_back({ 1ull << pos, bit, MoveType::BISHOP });
            }
        }

        while (notPinnedBishop > 0) {
            int pos = PopPos(notPinnedBishop);
            uint64 moves = board.BishopAttack(pos, board.m_Board) & moveable;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                result.push_back({ 1ull << pos, bit, MoveType::BISHOP });
            }
        }

        // Rooks
        uint64 rooks = Rook(board, white) & ~bishopPin; // A bishop pinned rook can not move

        uint64 pinnedRook = rookPin & rooks;
        uint64 notPinnedRook = rooks ^ pinnedRook;

        while (pinnedRook > 0) {
            int pos = PopPos(pinnedRook);
            uint64 moves = board.RookAttack(pos, board.m_Board) & moveable & rookPin;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                result.push_back({ 1ull << pos, bit, MoveType::ROOK });
            }
        }

        while (notPinnedRook > 0) {
            int pos = PopPos(notPinnedRook);
            uint64 moves = board.RookAttack(pos, board.m_Board) & moveable;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                result.push_back({ 1ull << pos, bit, MoveType::ROOK });
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
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN });
            }
        }

        while (pinnedDiagonalQueen > 0) {
            int pos = PopPos(pinnedDiagonalQueen);
            uint64 moves = board.BishopAttack(pos, board.m_Board) & moveable & bishopPin;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN });
            }
        }

        while (notPinnedQueen > 0) {
            int pos = PopPos(notPinnedQueen);
            uint64 moves = board.QueenAttack(pos, board.m_Board) & moveable;
            for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
                result.push_back({ 1ull << pos, bit, MoveType::QUEEN });
            }
        }

        // King
        int kingpos = GET_SQUARE(King(board, white));
        uint64 moves = Lookup::king_attacks[kingpos] & ~Player(board, white);
        moves &= ~danger;

        for (; uint64 bit = PopBit(moves); moves > 0) { // Loop each bit
            result.push_back({ 1ull << kingpos, bit, MoveType::KING });
        }

        // Castles
        uint64 CK = board.CastleKing(danger, white);
        uint64 CQ = board.CastleQueen(danger, white);
        if (CK) {
            result.push_back({ King(board, white), CK, MoveType::KCASTLE });
        }
        if (CQ) {
            result.push_back({ King(board, white), CQ, MoveType::QCASTLE });
        }


        return result;
    }
};