#include "main.h"
#include "zobrist.h"
#include "move.h"
#include "attacks.h"
#include "board.h"
#include "sliding_movegen.h"

#if defined(__BMI2__) || defined(USE_BMI2_FORCE)
    static inline U64 get_bishop_attacks(uint8_t square, U64 occupancy) {
        return BISHOP_ATTACKS[square][_pext_u64(occupancy, BISHOP_MASKS[square])];
    }

    static inline U64 get_rook_attacks(uint8_t square, U64 occupancy) {
        return ROOK_ATTACKS[square][_pext_u64(occupancy, ROOK_MASKS[square])];
    }
#else
    static inline U64 get_bishop_attacks(uint8_t square, U64 occupancy) {
        occupancy &= BISHOP_MASKS[square];
        occupancy *= BISHOP_MAGICS[square];
        occupancy >>= 64 - BISHOP_RELEVANT_BITS[square];
        return BISHOP_ATTACKS[square][occupancy];
    }

    static inline U64 get_rook_attacks(uint8_t square, U64 occupancy) {
        occupancy &= ROOK_MASKS[square];
        occupancy *= ROOK_MAGICS[square];
        occupancy >>= 64 - ROOK_RELEVANT_BITS[square];
        return ROOK_ATTACKS[square][occupancy];
    }
#endif

static inline U64 get_queen_attacks(uint8_t square, U64 occupancy) {
    return get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy);
}

Board::Board(string fen) : board{None} {

    for (int i = 0; i < 12; i++) {
        bitboards[i] = EMPTY;
    }

    for (int i = 0; i < 3; i++) {
        occupancy[i] = EMPTY;
    }

    en_passant = no_sq;
    state_history.clear();
    castle = 0;
    hash_key = 0;

    vector<string> splitted_fen = split(fen);
    
    // Step 1 : initialize every flag that are not relative to piece positions

    // 1.1 : turn
    side = (splitted_fen[1] == "w") ? WHITE : BLACK;

    // 1.2 : castling rights
    for (const char s : splitted_fen[2]) {
        if (s == 'K') {castle |= 0b0001;}
        if (s == 'Q') {castle |= 0b0010;}
        if (s == 'k') {castle |= 0b0100;}
        if (s == 'q') {castle |= 0b1000;}
    }
    
    // 1.3 : en-passant square
    if (splitted_fen[3] == "-") {
        en_passant = no_sq;  // No en-passant allowed
    } else {
        for (int i = 0; i < 64; ++i) {
            if (SQUARES_NAMES[i] == splitted_fen[3]) {
                en_passant = i;
                break;
            }
        }
    }

    if (splitted_fen.size() >= 5) {
        move_count[0] = std::atoi(splitted_fen[4].c_str());
        move_count[1] = std::atoi(splitted_fen[5].c_str());
    } else {
        move_count[0] = 0;
        move_count[1] = 1;
    }

    // Step 2 : place pieces
    uint8_t square = A8;
    for (const char s : splitted_fen[0]) {

        // Empty squares
        if (isdigit(s)) {
            square += atoi(string(1, s).c_str());
            continue;
        }

        // new row
        if (s == '/') {
            square -= 16;
            continue;
        }

        // Place piece
        if      (s == 'K') {add_piece(WKING,   square);}
        else if (s == 'Q') {add_piece(WQUEEN,  square);}
        else if (s == 'R') {add_piece(WROOK,   square);}
        else if (s == 'B') {add_piece(WBISHOP, square);}
        else if (s == 'N') {add_piece(WKNIGHT, square);}
        else if (s == 'P') {add_piece(WPAWN,   square);}
        else if (s == 'k') {add_piece(BKING,   square);}
        else if (s == 'q') {add_piece(BQUEEN,  square);}
        else if (s == 'r') {add_piece(BROOK,   square);}
        else if (s == 'b') {add_piece(BBISHOP, square);}
        else if (s == 'n') {add_piece(BKNIGHT, square);}
        else if (s == 'p') {add_piece(BPAWN,   square);}

        square += 1;

    }

    update_occupancy();
    zobrist_key();
}

void Board::update_occupancy() {
    occupancy[WHITE] = bitboards[WKING] | bitboards[WQUEEN] | bitboards[WROOK] | bitboards[WBISHOP] | bitboards[WKNIGHT] | bitboards[WPAWN];
    occupancy[BLACK] = bitboards[BKING] | bitboards[BQUEEN] | bitboards[BROOK] | bitboards[BBISHOP] | bitboards[BKNIGHT] | bitboards[BPAWN];
    occupancy[BOTH]  = occupancy[WHITE] | occupancy[BLACK];
}

inline void Board::add_piece(int8_t piece, uint8_t square) {
    bitboards[piece] = add_square(bitboards[piece], square);
    board[square] = piece;
}

inline void Board::remove_piece(int8_t piece, uint8_t square) {
    bitboards[piece] = remove_square(bitboards[piece], square);
    board[square] = None;
}

void Board::zobrist_key() {

    hash_key = 0;
    hash_key ^= castlingKey[castle];

    U64 bitboard;
    int square;

    for (int piece = WPAWN; piece <= BKING; piece++) {
        bitboard = bitboards[piece];

        while (bitboard) {
            square = ls1b_index(bitboard);
            hash_key ^= RANDOM_ARRAY[64 * hash_piece[piece] + 8 * square_rank(square) + square_file(square)];
            bitboard = remove_square(bitboard, square);
        }
    }

    if (side == WHITE) {
        hash_key ^= RANDOM_ARRAY[780];
    }

    if (en_passant != no_sq) {
        hash_key ^= RANDOM_ARRAY[772 + square_file(en_passant)];
    }

}

bool Board::is_square_attacked(uint8_t square, uint8_t by_side) {

    if ((by_side == WHITE) && (PAWN_ATTACKS[BLACK][square] & bitboards[WPAWN])) {
        return true;
    } else if ((by_side == BLACK) && (PAWN_ATTACKS[WHITE][square] & bitboards[BPAWN])) {
        return true;
    } else if (KNIGHT_ATTACKS[square] & (by_side == WHITE ? bitboards[WKNIGHT] : bitboards[BKNIGHT])) {
        return true;
    } else if (KING_ATTACKS[square] & (by_side == WHITE ? bitboards[WKING] : bitboards[BKING])) {
        return true;
    } else if (get_bishop_attacks(square, occupancy[BOTH]) & (by_side == WHITE ? bitboards[WBISHOP] | bitboards[WQUEEN] : bitboards[BBISHOP] | bitboards[BQUEEN])) {
        return true;
    } else if (get_rook_attacks(square, occupancy[BOTH]) & (by_side == WHITE ? bitboards[WROOK] | bitboards[WQUEEN] : bitboards[BROOK] | bitboards[BQUEEN])) {
        return true;
    }
    return false;
}

vector<Move> Board::generate_moves() {

    vector<Move> movelist;
    U64 bitboard;
    U64 attacks;
    uint8_t from_square;
    uint8_t to_square;

    for (int8_t piece = WPAWN; piece <= BKING; piece++) {
        bitboard = bitboards[piece] & occupancy[side];

        if (side == WHITE) {
            
            if (piece == WPAWN) {
                while (bitboard) {
                    from_square = ls1b_index(bitboard);

                    // quiet moves
                    to_square = from_square + 8;
                    if (!contains_square(occupancy[BOTH], to_square)) {
                        if ((to_square >= A8) && (to_square <= H8)) { // pawn promotion
                            movelist.push_back(Move(from_square, to_square, None, WQUEEN));
                            movelist.push_back(Move(from_square, to_square, None, WROOK));
                            movelist.push_back(Move(from_square, to_square, None, WBISHOP));
                            movelist.push_back(Move(from_square, to_square, None, WKNIGHT));
                        } else { // pawn push
                            movelist.push_back(Move(from_square, to_square)); // single push
                            if ((from_square >= A2) && (from_square <= H2) && !contains_square(occupancy[BOTH], to_square+8)) {
                                movelist.push_back(Move(from_square, to_square+8)); // double push
                            }
                        }
                    }

                    // capture moves
                    attacks = PAWN_ATTACKS[WHITE][from_square] & occupancy[BLACK];
                    while (attacks) {
                        to_square = ls1b_index(attacks);
                        if ((to_square >= A8) && (to_square <= H8)) { // capture promotion
                            movelist.push_back(Move(from_square, to_square, board[to_square], WQUEEN));
                            movelist.push_back(Move(from_square, to_square, board[to_square], WROOK));
                            movelist.push_back(Move(from_square, to_square, board[to_square], WBISHOP));
                            movelist.push_back(Move(from_square, to_square, board[to_square], WKNIGHT));
                        } else { // normal capture
                            movelist.push_back(Move(from_square, to_square, board[to_square]));
                        }
                        attacks = remove_square(attacks, to_square);
                    }

                    // en passant
                    if (en_passant != no_sq) {
                        attacks = PAWN_ATTACKS[WHITE][from_square] & (1ULL << en_passant);
                        if (attacks) { 
                            movelist.push_back(Move(from_square, en_passant, BPAWN, None, true));
                        }
                    }

                    bitboard = remove_square(bitboard, from_square);
                }
            } else if (piece == WKING) { // castling move
                if (castle & WK) { // kingside
                    if (!contains_square(occupancy[BOTH], F1) && !contains_square(occupancy[BOTH], G1)) {
                        if (!is_square_attacked(E1, BLACK) && ! is_square_attacked(F1, BLACK)) {
                            movelist.push_back(Move(E1, G1));
                        }
                    }
                }
                if (castle & WQ) { // queenside
                    if (!contains_square(occupancy[BOTH], B1) && !contains_square(occupancy[BOTH], C1) && !contains_square(occupancy[BOTH], D1)) {
                        if (!is_square_attacked(E1, BLACK) && ! is_square_attacked(D1, BLACK)) {
                            movelist.push_back(Move(E1, C1));
                        }
                    }
                }
            }
        
        } else if (side == BLACK) {

            if (piece == BPAWN) {
                while (bitboard) {
                    from_square = ls1b_index(bitboard);

                    // quiet moves
                    to_square = from_square - 8;
                    if (!contains_square(occupancy[BOTH], to_square)) {
                        if ((to_square >= A1) && (to_square <= H1)) { // pawn promotion
                            movelist.push_back(Move(from_square, to_square, None, BQUEEN));
                            movelist.push_back(Move(from_square, to_square, None, BROOK));
                            movelist.push_back(Move(from_square, to_square, None, BBISHOP));
                            movelist.push_back(Move(from_square, to_square, None, BKNIGHT));
                        } else { // pawn push
                            movelist.push_back(Move(from_square, to_square)); // single push
                            if ((from_square >= A7) && (from_square <= H7) && !contains_square(occupancy[BOTH], to_square-8)) {
                                movelist.push_back(Move(from_square, to_square-8)); // double push
                            }
                        }
                    }

                    // capture moves
                    attacks = PAWN_ATTACKS[BLACK][from_square] & occupancy[WHITE];
                    while (attacks) {
                        to_square = ls1b_index(attacks);
                        if ((to_square >= A1) && (to_square <= H1)) { // capture promotion
                            movelist.push_back(Move(from_square, to_square, board[to_square], BQUEEN));
                            movelist.push_back(Move(from_square, to_square, board[to_square], BROOK));
                            movelist.push_back(Move(from_square, to_square, board[to_square], BBISHOP));
                            movelist.push_back(Move(from_square, to_square, board[to_square], BKNIGHT));
                        } else { // normal capture
                            movelist.push_back(Move(from_square, to_square, board[to_square]));
                        }
                        attacks = remove_square(attacks, to_square);
                    }

                    // en passant
                    if (en_passant != no_sq) {
                        attacks = PAWN_ATTACKS[BLACK][from_square] & (1ULL << en_passant);
                        if (attacks) { 
                            movelist.push_back(Move(from_square, en_passant, WPAWN, None, true));
                        }
                    }

                    bitboard = remove_square(bitboard, from_square);
                }
            } else if (piece == BKING) { // castling move
                if (castle & BK) { // kingside
                    if (!contains_square(occupancy[BOTH], F8) && !contains_square(occupancy[BOTH], G8)) {
                        if (!is_square_attacked(E8, WHITE) && !is_square_attacked(F8, WHITE)) {
                            movelist.push_back(Move(E8, G8));
                        }
                    }
                }
                if (castle & BQ) { // queenside
                    if (!contains_square(occupancy[BOTH], B8) && !contains_square(occupancy[BOTH], C8) && !contains_square(occupancy[BOTH], D8)) {
                        if (!is_square_attacked(E8, WHITE) && ! is_square_attacked(D8, WHITE)) {
                            movelist.push_back(Move(E8, C8));
                        }
                    }
                }
            }
        }

        // knight
        if (((piece == WKNIGHT) && (side == WHITE)) || ((piece == BKNIGHT) && (side == BLACK))) {
            while (bitboard) {
                from_square = ls1b_index(bitboard);
                attacks = KNIGHT_ATTACKS[from_square] & (~occupancy[side]);

                while (attacks) {
                    to_square = ls1b_index(attacks);

                    if (contains_square(occupancy[BOTH], to_square)) { // capture
                        movelist.push_back(Move(from_square, to_square, board[to_square]));    
                    } else { // quiet move
                        movelist.push_back(Move(from_square, to_square));
                    }

                    attacks = remove_square(attacks, to_square);
                }

                bitboard = remove_square(bitboard, from_square);
            }
        }

        // bishop
        else if (((piece == WBISHOP) && (side == WHITE)) || ((piece == BBISHOP) && (side == BLACK))) {
            while (bitboard) {
                from_square = ls1b_index(bitboard);
                attacks = get_bishop_attacks(from_square, occupancy[BOTH]) & (~occupancy[side]);

                while (attacks) {
                    to_square = ls1b_index(attacks);

                    if (contains_square(occupancy[BOTH], to_square)) { // capture
                        movelist.push_back(Move(from_square, to_square, board[to_square]));    
                    } else { // quiet move
                        movelist.push_back(Move(from_square, to_square));
                    }

                    attacks = remove_square(attacks, to_square);
                }

                bitboard = remove_square(bitboard, from_square);
            }
        }

        // rook
        else if (((piece == WROOK) && (side == WHITE)) || ((piece == BROOK) && (side == BLACK))) {
            while (bitboard) {
                from_square = ls1b_index(bitboard);
                attacks = get_rook_attacks(from_square, occupancy[BOTH]) & (~occupancy[side]);

                while (attacks) {
                    to_square = ls1b_index(attacks);

                    if (contains_square(occupancy[BOTH], to_square)) { // capture
                        movelist.push_back(Move(from_square, to_square, board[to_square]));    
                    } else { // quiet move
                        movelist.push_back(Move(from_square, to_square));
                    }

                    attacks = remove_square(attacks, to_square);
                }

                bitboard = remove_square(bitboard, from_square);
            }
        }

        // rook
        else if (((piece == WQUEEN) && (side == WHITE)) || ((piece == BQUEEN) && (side == BLACK))) {
            while (bitboard) {
                from_square = ls1b_index(bitboard);
                attacks = get_queen_attacks(from_square, occupancy[BOTH]) & (~occupancy[side]);

                while (attacks) {
                    to_square = ls1b_index(attacks);

                    if (contains_square(occupancy[BOTH], to_square)) { // capture
                        movelist.push_back(Move(from_square, to_square, board[to_square]));    
                    } else { // quiet move
                        movelist.push_back(Move(from_square, to_square));
                    }

                    attacks = remove_square(attacks, to_square);
                }

                bitboard = remove_square(bitboard, from_square);
            }
        }

        // king
        else if (((piece == WKING) && (side == WHITE)) || ((piece == BKING) && (side == BLACK))) {
            while (bitboard) {
                from_square = ls1b_index(bitboard);
                attacks = KING_ATTACKS[from_square] & (~occupancy[side]);

                while (attacks) {
                    to_square = ls1b_index(attacks);

                    if (contains_square(occupancy[BOTH], to_square)) { // capture
                        movelist.push_back(Move(from_square, to_square, board[to_square]));    
                    } else { // quiet move
                        movelist.push_back(Move(from_square, to_square));
                    }

                    attacks = remove_square(attacks, to_square);
                }

                bitboard = remove_square(bitboard, from_square);
            }
        }
    }

    return movelist;
}

vector<Move> Board::generate_captures() {
    
    // pawn promotions are considered capture as they change the material balance

    vector<Move> movelist;
    U64 bitboard;
    U64 attacks;
    int from_square;
    int to_square;

    for (int piece = WPAWN; piece <= BKING; piece++) {
        bitboard = bitboards[piece] & occupancy[side];

        if (side == WHITE) {
            
            if (piece == WPAWN) {
                while (bitboard) {
                    from_square = ls1b_index(bitboard);

                    // quiet moves
                    to_square = from_square + 8;
                    if (!contains_square(occupancy[BOTH], to_square)) {
                        if ((to_square >= A8) && (to_square <= H8)) { // pawn promotion
                            movelist.push_back(Move(from_square, to_square, None, WQUEEN));
                            movelist.push_back(Move(from_square, to_square, None, WROOK));
                            movelist.push_back(Move(from_square, to_square, None, WBISHOP));
                            movelist.push_back(Move(from_square, to_square, None, WKNIGHT));
                        }
                    }

                    // capture moves
                    attacks = PAWN_ATTACKS[WHITE][from_square] & occupancy[BLACK];
                    while (attacks) {
                        to_square = ls1b_index(attacks);
                        if ((to_square >= A8) && (to_square <= H8)) { // capture promotion
                            movelist.push_back(Move(from_square, to_square, board[to_square], WQUEEN));
                            movelist.push_back(Move(from_square, to_square, board[to_square], WROOK));
                            movelist.push_back(Move(from_square, to_square, board[to_square], WBISHOP));
                            movelist.push_back(Move(from_square, to_square, board[to_square], WKNIGHT));
                        } else { // normal capture
                            movelist.push_back(Move(from_square, to_square, board[to_square]));
                        }
                        attacks = remove_square(attacks, to_square);
                    }

                    // en passant
                    if (en_passant != no_sq) {
                        attacks = PAWN_ATTACKS[WHITE][from_square] & (1ULL << en_passant);
                        if (attacks) { 
                            movelist.push_back(Move(from_square, en_passant, BPAWN, None, true));
                        }
                    }

                    bitboard = remove_square(bitboard, from_square);
                }
            }
        
        } else if (side == BLACK) {

            if (piece == BPAWN) {
                while (bitboard) {
                    from_square = ls1b_index(bitboard);

                    // quiet moves
                    to_square = from_square - 8;
                    if (!contains_square(occupancy[BOTH], to_square)) {
                        if ((to_square >= A1) && (to_square <= H1)) { // pawn promotion
                            movelist.push_back(Move(from_square, to_square, None, BQUEEN));
                            movelist.push_back(Move(from_square, to_square, None, BROOK));
                            movelist.push_back(Move(from_square, to_square, None, BBISHOP));
                            movelist.push_back(Move(from_square, to_square, None, BKNIGHT));
                        }
                    }

                    // capture moves
                    attacks = PAWN_ATTACKS[BLACK][from_square] & occupancy[WHITE];
                    while (attacks) {
                        to_square = ls1b_index(attacks);
                        if ((to_square >= A1) && (to_square <= H1)) { // capture promotion
                            movelist.push_back(Move(from_square, to_square, board[to_square], BQUEEN));
                            movelist.push_back(Move(from_square, to_square, board[to_square], BROOK));
                            movelist.push_back(Move(from_square, to_square, board[to_square], BBISHOP));
                            movelist.push_back(Move(from_square, to_square, board[to_square], BKNIGHT));
                        } else { // normal capture
                            movelist.push_back(Move(from_square, to_square, board[to_square]));
                        }
                        attacks = remove_square(attacks, to_square);
                    }

                    // en passant
                    if (en_passant != no_sq) {
                        attacks = PAWN_ATTACKS[BLACK][from_square] & (1ULL << en_passant);
                        if (attacks) { 
                            movelist.push_back(Move(from_square, en_passant, WPAWN, None, true));
                        }
                    }

                    bitboard = remove_square(bitboard, from_square);
                }
            } 
        }

        // knight
        if (((piece == WKNIGHT) && (side == WHITE)) || ((piece == BKNIGHT) && (side == BLACK))) {
            while (bitboard) {
                from_square = ls1b_index(bitboard);
                attacks = KNIGHT_ATTACKS[from_square] & (~occupancy[side]);

                while (attacks) {
                    to_square = ls1b_index(attacks);

                    if (contains_square(occupancy[BOTH], to_square)) { // capture
                        movelist.push_back(Move(from_square, to_square, board[to_square]));    
                    }
                    attacks = remove_square(attacks, to_square);
                }

                bitboard = remove_square(bitboard, from_square);
            }
        }

        // bishop
        else if (((piece == WBISHOP) && (side == WHITE)) || ((piece == BBISHOP) && (side == BLACK))) {
            while (bitboard) {
                from_square = ls1b_index(bitboard);
                attacks = get_bishop_attacks(from_square, occupancy[BOTH]) & (~occupancy[side]);

                while (attacks) {
                    to_square = ls1b_index(attacks);

                    if (contains_square(occupancy[BOTH], to_square)) { // capture
                        movelist.push_back(Move(from_square, to_square, board[to_square]));    
                    }

                    attacks = remove_square(attacks, to_square);
                }

                bitboard = remove_square(bitboard, from_square);
            }
        }

        // rook
        else if (((piece == WROOK) && (side == WHITE)) || ((piece == BROOK) && (side == BLACK))) {
            while (bitboard) {
                from_square = ls1b_index(bitboard);
                attacks = get_rook_attacks(from_square, occupancy[BOTH]) & (~occupancy[side]);

                while (attacks) {
                    to_square = ls1b_index(attacks);

                    if (contains_square(occupancy[BOTH], to_square)) { // capture
                        movelist.push_back(Move(from_square, to_square, board[to_square]));    
                    }

                    attacks = remove_square(attacks, to_square);
                }

                bitboard = remove_square(bitboard, from_square);
            }
        }

        // rook
        else if (((piece == WQUEEN) && (side == WHITE)) || ((piece == BQUEEN) && (side == BLACK))) {
            while (bitboard) {
                from_square = ls1b_index(bitboard);
                attacks = get_queen_attacks(from_square, occupancy[BOTH]) & (~occupancy[side]);

                while (attacks) {
                    to_square = ls1b_index(attacks);

                    if (contains_square(occupancy[BOTH], to_square)) { // capture
                        movelist.push_back(Move(from_square, to_square, board[to_square]));    
                    }

                    attacks = remove_square(attacks, to_square);
                }

                bitboard = remove_square(bitboard, from_square);
            }
        }

        // king
        else if (((piece == WKING) && (side == WHITE)) || ((piece == BKING) && (side == BLACK))) {
            while (bitboard) {
                from_square = ls1b_index(bitboard);
                attacks = KING_ATTACKS[from_square] & (~occupancy[side]);

                while (attacks) {
                    to_square = ls1b_index(attacks);

                    if (contains_square(occupancy[BOTH], to_square)) { // capture
                        movelist.push_back(Move(from_square, to_square, board[to_square]));    
                    }

                    attacks = remove_square(attacks, to_square);
                }

                bitboard = remove_square(bitboard, from_square);
            }
        }
    }

    return movelist;
}

bool Board::make(Move move) {

    int8_t piece = board[move.from_sq];

    state_history.push_back(State(en_passant, castle, move_count));
    move_stack.push_back(move);
    hash_history.push_back(hash_key);

    move_count[0] += 1;
    if (side == BLACK) {
        move_count[1] += 1;
    }

    hash_key ^= castlingKey[castle];
    if (en_passant != no_sq) {
        hash_key ^= RANDOM_ARRAY[772 + square_file(en_passant)];
    }

    castle &= castle_mask[move.from_sq] & castle_mask[move.to_sq];

    if (piece == WKING) {
        castle &= BK|BQ;
        if ((move.from_sq == E1) && (move.to_sq == G1)) {
            remove_piece(WROOK, H1);
            add_piece(WROOK, F1);
            hash_key ^= RANDOM_ARRAY[64 * hash_piece[WROOK] + H1];
            hash_key ^= RANDOM_ARRAY[64 * hash_piece[WROOK] + F1];
        } else if ((move.from_sq == E1) && (move.to_sq == C1)) {
            remove_piece(WROOK, A1);
            add_piece(WROOK, D1);
            hash_key ^= RANDOM_ARRAY[64 * hash_piece[WROOK] + A1];
            hash_key ^= RANDOM_ARRAY[64 * hash_piece[WROOK] + D1];
        }
    } else if (piece == BKING) {
        castle &= WK|WQ;
        if ((move.from_sq == E8) && (move.to_sq == G8)) {
            remove_piece(BROOK, H8);
            add_piece(BROOK, F8);
            hash_key ^= RANDOM_ARRAY[64 * hash_piece[BROOK] + H8];
            hash_key ^= RANDOM_ARRAY[64 * hash_piece[BROOK] + F8];
        } else if ((move.from_sq == E8) && (move.to_sq == C8)) {
            remove_piece(BROOK, A8);
            add_piece(BROOK, D8);
            hash_key ^= RANDOM_ARRAY[64 * hash_piece[BROOK] + A8];
            hash_key ^= RANDOM_ARRAY[64 * hash_piece[BROOK] + D8];
        }
    }

    if (move.is_ep) {
        remove_piece(move.captured, side == WHITE ? en_passant-8 : en_passant+8);
        hash_key ^= RANDOM_ARRAY[64 * hash_piece[move.captured] + (side == WHITE ? en_passant-8 : en_passant+8)];
    }
    
    else if (move.captured != None) {
        move_count[0] = 0;
        remove_piece(move.captured, move.to_sq);
        hash_key ^= RANDOM_ARRAY[64 * hash_piece[move.captured] + move.to_sq];
    }

    if (move.promoted != None) {
        add_piece(move.promoted, move.to_sq);
        remove_piece(piece, move.from_sq);
        hash_key ^= RANDOM_ARRAY[64 * hash_piece[move.promoted] + move.to_sq];
        hash_key ^= RANDOM_ARRAY[64 * hash_piece[piece] + move.from_sq];
    }
    
    else {
        remove_piece(piece, move.from_sq);
        add_piece(piece, move.to_sq);
        hash_key ^= RANDOM_ARRAY[64 * hash_piece[piece] + move.to_sq];
        hash_key ^= RANDOM_ARRAY[64 * hash_piece[piece] + move.from_sq];
    }

    en_passant = no_sq;
    if (piece == WPAWN or piece == BPAWN) {
        move_count[0] = 0;
        if (abs(move.from_sq - move.to_sq) == 16) {
            en_passant = side == WHITE ? move.from_sq + 8 : move.from_sq - 8;
            hash_key ^= RANDOM_ARRAY[772 + square_file(en_passant)];
        }
    }

    update_occupancy();
    side ^= 1;
    hash_key ^= castlingKey[castle];
    hash_key ^= RANDOM_ARRAY[780]; // side

    // legality check-up
    bool illegal = is_square_attacked(ls1b_index(bitboards[side == BLACK ? WKING : BKING]), side);
    if (illegal) {
        unmake();
        return false;
    }
    return true;
}

void Board::unmake() {

    Move move = move_stack.back();
    move_stack.pop_back();
    State state = state_history.back();
    state_history.pop_back();
    en_passant = state.en_passant;
    castle = state.castle;
    move_count[0] = state.movecount[0];
    move_count[1] = state.movecount[1];
    hash_key = hash_history.back();
    hash_history.pop_back();
    side ^= 1;

    int8_t captured = move.captured;
    int8_t piece = board[move.to_sq];

    if (move.promoted != None) {
        remove_piece(piece, move.to_sq);
        add_piece(side == WHITE ? WPAWN : BPAWN, move.from_sq);
        if (move.captured != None) {
            add_piece(captured, move.to_sq);
        }
        update_occupancy();
        return;
    }

    remove_piece(piece, move.to_sq);
    add_piece(piece, move.from_sq);

    if (move.is_ep) {
        if (side == WHITE) {
            add_piece(BPAWN, en_passant-8);
        } else {
            add_piece(WPAWN, en_passant+8);
        }
    } else if (move.captured != None) {
        add_piece(captured, move.to_sq);
    }

    if (piece == WKING) {
        if ((move.from_sq == E1) && (move.to_sq == G1)) {
            add_piece(WROOK, H1);
            remove_piece(WROOK, F1);
        } else if ((move.from_sq == E1) && (move.to_sq == C1)) {
            add_piece(WROOK, A1);
            remove_piece(WROOK, D1);
        }
    } else if (piece == BKING) {
        if ((move.from_sq == E8) && (move.to_sq == G8)) {
            add_piece(BROOK, H8);
            remove_piece(BROOK, F8);
        } else if ((move.from_sq == E8) && (move.to_sq == C8)) {
            add_piece(BROOK, A8);
            remove_piece(BROOK, D8);
        }
    }

    update_occupancy();
    
}

void Board::make_null() {
    move_stack.push_back(nullmove);
    state_history.push_back(State(en_passant, castle, move_count));
    hash_history.push_back(hash_key);
    if (side == BLACK) {
        move_count[1] += 1;
    }
    move_count[0] += 1;
    side ^= 1;
    hash_key ^= RANDOM_ARRAY[780];
    if (en_passant != no_sq) {
        hash_key ^= RANDOM_ARRAY[772 + square_file(en_passant)];
        en_passant = no_sq;
    }
}

void Board::unmake_null() {
    move_stack.pop_back();
    State state = state_history.back();
    state_history.pop_back();
    en_passant = state.en_passant;
    castle = state.castle;
    move_count[0] = state.movecount[0];
    move_count[1] = state.movecount[1];
    hash_key = hash_history.back();
    hash_history.pop_back();
    side ^= 1;
}

bool Board::is_repetition() {
    if (hash_history.size() <= move_count[1] + 1) {
        return false;
    }
    return count(hash_history.begin() + move_count[1], hash_history.end(), hash_history.back()) >= 2;
}

vector<Move> Board::generate_legal_moves() {

    vector<Move> moves;

    for (const Move move : generate_moves()) {
        if (make(move)) {
            unmake();
            moves.push_back(move);
        }
    }

    return moves;   
}

bool Board::make_str_move(string str_move) {

    for (char& c : str_move) {
        c = std::tolower(static_cast<unsigned char>(c));
    }

    for (const Move move : generate_moves()) {
        if (str_move == move.to_string()) {
            return make(move);
        }
    }
    return false;
}