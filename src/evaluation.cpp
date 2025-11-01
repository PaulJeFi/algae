#include "main.h"
#include "move.h"
#include "board.h"
#include "evaluation.h"

int mop_up(Board &board) {
    
    int material[2] = {0, 0}; // material score for [WHITE, BLACK]
    int value = 0;
    U64 bitboard = board.occupancy[BOTH];
    uint8_t square = no_sq;

    while (bitboard) {

        square = ls1b_index(bitboard);
        value = mop_up_values[board.board[square]];
        if (value > 0) {
            material[0] += value;
        } else {
            material[1] += value;
        }
        bitboard = remove_square(bitboard, square);
    }

    if (material[0] == material[1]) {
            return 0;
    }
    int8_t winner = 1;
    if (material[0] < material[1]) {
        winner = -1;
    }

    uint8_t wk = ls1b_index(board.bitboards[WKING]);
    uint8_t bk = ls1b_index(board.bitboards[BKING]);

    return (int) winner * (CMD[winner == 1 ? wk : bk]) + 1.6 * (14 - square_manhattan_distance(wk, bk));

}

int evaluate(Board &board) {

    // TODO : bishop and queen mobility, king safety, bishop pair

    int8_t s2m = board.side == WHITE ? 1 : -1;

    int score = 0;
    int phase = 24;
    int EG = mop_up(board);
    int MG = 0;
    int file_;
    uint8_t square = no_sq;
    int8_t piece;

    U64 bitboard = board.occupancy[BOTH];
    while (bitboard) {
        square = ls1b_index(bitboard);
        piece = board.board[square];

        if (piece == WPAWN) {

            EG += EG_VALUES[WPAWN];
            MG += MG_VALUES[WPAWN];
            EG += PST_EG[WPAWN][flip[square]];
            MG += PST_MG[WPAWN][flip[square]];
            phase -= PHASE_VALUES[WPAWN];

            file_ = square_file(square);

            // doubled pawns
            if (popcount(board.bitboards[WPAWN] & FILES[file_]) > 1) {
                MG -= 5;
                EG -= 10;
            }

            // isolated pawn
            if ((board.bitboards[WPAWN] & ISOLATED_MASK[file_]) == 0) {
                MG -= 5;
                EG -= 10;
            }


        } else if (piece == WKNIGHT) {
            
            EG += EG_VALUES[WKNIGHT];
            MG += MG_VALUES[WKNIGHT];
            EG += PST_EG[WKNIGHT][flip[square]];
            MG += PST_MG[WKNIGHT][flip[square]];
            phase -= PHASE_VALUES[WKNIGHT];

        } else if (piece == WBISHOP) {
            
            EG += EG_VALUES[WBISHOP];
            MG += MG_VALUES[WBISHOP];
            EG += PST_EG[WBISHOP][flip[square]];
            MG += PST_MG[WBISHOP][flip[square]];
            phase -= PHASE_VALUES[WBISHOP];

        } else if (piece == WROOK) {
            
            EG += EG_VALUES[WROOK];
            MG += MG_VALUES[WROOK];
            EG += PST_EG[WROOK][flip[square]];
            MG += PST_MG[WROOK][flip[square]];
            phase -= PHASE_VALUES[WROOK];

            file_ = square_file(square);
                    
            // open-files
            if (((board.bitboards[WPAWN] || board.bitboards[BPAWN]) & FILES[file_]) == 0) {
                score += 15;
            }
            
            // semi-open files
            else if ((board.bitboards[WPAWN] & FILES[file_]) == 0) {
                score += 10;
            }

        } else if (piece == WQUEEN) {
            
            EG += EG_VALUES[WQUEEN];
            MG += MG_VALUES[WQUEEN];
            EG += PST_EG[WQUEEN][flip[square]];
            MG += PST_MG[WQUEEN][flip[square]];
            phase -= PHASE_VALUES[WQUEEN];

        } else if (piece == WKING) {
            
            EG += EG_VALUES[WKING];
            MG += MG_VALUES[WKING];
            EG += PST_EG[WKING][flip[square]];
            MG += PST_MG[WKING][flip[square]];
            phase -= PHASE_VALUES[WKING];

        } else if (piece == BPAWN) {
            
            EG -= EG_VALUES[WPAWN];
            MG -= MG_VALUES[WPAWN];
            EG -= PST_EG[WPAWN][square];
            MG -= PST_MG[WPAWN][square];
            phase -= PHASE_VALUES[WPAWN];

            file_ = square_file(square);

            // doubled pawns
            if (popcount(board.bitboards[BPAWN] & FILES[file_]) > 1) {
                MG += 5;
                EG += 10;
            }

            // isolated pawn
            if ((board.bitboards[BPAWN] & ISOLATED_MASK[file_]) == 0) {
                MG += 5;
                EG += 10;
            }

        } else if (piece == BKNIGHT) {
            
            EG -= EG_VALUES[WKNIGHT];
            MG -= MG_VALUES[WKNIGHT];
            EG -= PST_EG[WKNIGHT][square];
            MG -= PST_MG[WKNIGHT][square];
            phase -= PHASE_VALUES[WKNIGHT];

        } else if (piece == BBISHOP) {
            
            EG -= EG_VALUES[WBISHOP];
            MG -= MG_VALUES[WBISHOP];
            EG -= PST_EG[WBISHOP][square];
            MG -= PST_MG[WBISHOP][square];
            phase -= PHASE_VALUES[WBISHOP];
            
        } else if (piece == BROOK) {
            
            EG -= EG_VALUES[WROOK];
            MG -= MG_VALUES[WROOK];
            EG -= PST_EG[WROOK][square];
            MG -= PST_MG[WROOK][square];
            phase -= PHASE_VALUES[WROOK];

            file_ = square_file(square);
                    
            // open-files
            if (((board.bitboards[WPAWN] || board.bitboards[BPAWN]) & FILES[file_]) == 0) {
                score -= 15;
            }
            
            // semi-open files
            else if ((board.bitboards[BPAWN] & FILES[file_]) == 0) {
                score -= 10;
            }

        } else if (piece == BQUEEN) {
            
            EG -= EG_VALUES[WQUEEN];
            MG -= MG_VALUES[WQUEEN];
            EG -= PST_EG[WQUEEN][square];
            MG -= PST_MG[WQUEEN][square];
            phase -= PHASE_VALUES[WQUEEN];

        } else if (piece == BKING) {
            
            EG -= EG_VALUES[WKING];
            MG -= MG_VALUES[WKING];
            EG -= PST_EG[WKING][square];
            MG -= PST_MG[WKING][square];
            phase -= PHASE_VALUES[WKING];

        }

        bitboard = remove_square(bitboard, square);
    }

    // Bishop pair
    if (popcount(board.bitboards[WBISHOP]) >= 2) {
        score += 20;
    } if (popcount(board.bitboards[BBISHOP]) >= 2) {
        score -= 20;
    }

    // General positional eval

    // Minor pieces developed
    if (board.board[B1] != WKNIGHT) {
        MG += 8;
    } if (board.board[C1] != WBISHOP) {
        MG += 8;
    } if (board.board[F1] != WBISHOP) {
        MG += 8;
    } if (board.board[G1] != WKNIGHT) {
        MG += 8;
    } if (board.board[B8] != BKNIGHT) {
        MG -= 8;
    } if (board.board[C8] != BBISHOP) {
        MG -= 8;
    } if (board.board[F8] != BBISHOP) {
        MG -= 8;
    } if (board.board[G8] != BKNIGHT) {
        MG -= 8;
    }

    // Trapped bishop
    if ((board.board[A7] == WBISHOP)  &&  (board.board[B6] == BPAWN)) {
        MG -= 120;
    } if ((board.board[H7] == WBISHOP)  &&  (board.board[G6] == BPAWN)) {
        MG -= 120;
    } if ((board.board[A2] == BBISHOP)  &&  (board.board[B3] == WPAWN)) {
        MG += 120;
    } if ((board.board[H2] == BBISHOP)  &&  (board.board[G3] == WPAWN)) {
        MG += 120;
    }

    // Central pawn control
    if (((board.board[E4] == WPAWN) || (board.board[E5] == WPAWN)) &&
       ((board.board[D4] == WPAWN) || (board.board[D5] == WPAWN))) {
        MG += 15;
    } if (((board.board[E4] == BPAWN)  || (board.board[E5] == BPAWN)) &&
       ((board.board[D4] == BPAWN) || (board.board[D5] == BPAWN))) {
        MG -= 15;
    }

    // Undevelopped central pawn
    if ((board.board[E2] == WPAWN) && (board.board[E3] != None)) {
        MG -= 15;
    } if ((board.board[D2] == WPAWN) && (board.board[D3] != None)) {
        MG -= 15;
    } if ((board.board[E7] == BPAWN) && (board.board[E6] != None)) {
        MG += 15;
    } if ((board.board[D7] == BPAWN) && (board.board[D6] != None)) {
        MG += 15;
    }

    // Tapered eval
    phase = (int)((phase * 256 + 12) / 24);
    score += (int) (((MG * (256 - phase)) + (EG * phase)) / 256);

    return s2m * score;
}