#include "main.h"
#include "move.h"
#include "board.h"
#include "tt.h"
#include "ordering.h"

int history[12][64] = {{0}};
Move killers[MAX_PLY][2] = {{nullmove}};

void reset_tables() {

    tt.clear();
    for (int i=0; i<12; i++) {
        for (int j=0; j<64; j++) {
            history[i][j] = 0;
        }
    }
    for (int i=0; i<MAX_PLY; i++) {
        killers[i][0] = nullmove;
        killers[i][1] = nullmove;
    }
}

void update_killers(Move move, uint ply) {
    if (killers[ply][0] == move) {
        return;
    } if (killers[ply][1] == move) {
        std::swap(killers[ply][0], killers[ply][1]);
        return;
    }
    killers[ply][1] = killers[ply][0];
    killers[ply][0] = move;
}

int score_move(Move move, Board &board, uint ply, Move &best_move) {

    int score = 0;

    if (move == best_move) {
        return VALUE_MATE;
    }

    if (move.is_ep) {
        return 105; // PxP
    }

    if (move.captured != None) {
        return MVV_LVA[board.board[move.from_sq]][move.captured];
    }

    if (killers[ply][0] == move) {
        score += 9000;
    } else if (killers[ply][1] == move) {
        score += 8000;
    }
    score +=history[board.board[move.from_sq]][move.to_sq];

    return score;
}

vector<Move> ordering(Board &board, uint ply, vector<Move> &moves, Move best_move) {

    sort(moves.begin(), moves.end(), 
         [&](const Move& a, const Move& b) {
             return score_move(a, board, ply, best_move) > 
                    score_move(b, board, ply, best_move);
         });
    return moves;
}