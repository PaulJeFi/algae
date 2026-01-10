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

void update_killers(const Move &move, uint ply) {
    if (ply >= MAX_PLY) {
        return;
    }
    if (move.is_ep || (move.captured != None)) {
        return; // Because capture are threated before killers when scoring moves. See score_move.
    }
    if (killers[ply][0] == move) {
        return;
    } if (killers[ply][1] == move) {
        std::swap(killers[ply][0], killers[ply][1]);
        return;
    }
    killers[ply][1] = killers[ply][0];
    killers[ply][0] = move;
}

int score_move(const Move &move, const Board &board, uint ply, const Move &best_move) {

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

    if (ply < MAX_PLY) {
        if (killers[ply][0] == move) {
            score += 9000;
        } else if (killers[ply][1] == move) {
            score += 8000;
        }
    }
    score += history[board.board[move.from_sq]][move.to_sq];

    return score;
}

void ordering(const Board &board, uint ply, MoveList &moves, const Move &best_move) {

    size_t n = (size_t)moves.count;
    if (n <= 1) {
        return;
    }
    // compute scores array
    int scores[MAX_MOVES];
    for (size_t i = 0; i < n; ++i) {
        scores[i] = score_move(moves.moves[i], board, ply, best_move);
    }

    // insertion sort
    for (size_t i = 1; i < n; ++i) {
        Move mv = moves.moves[i];
        int sc = scores[i];
        size_t j = i;
        while (j > 0 && scores[j-1] < sc) {
            moves.moves[j] = moves.moves[j-1];
            scores[j] = scores[j-1];
            j--;
        }
        moves.moves[j] = mv;
        scores[j] = sc;
    }
}