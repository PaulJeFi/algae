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

    if (killers[ply][0] == move) {
        score += 9000;
    } else if (killers[ply][1] == move) {
        score += 8000;
    }
    score +=history[board.board[move.from_sq]][move.to_sq];

    return score;
}

void ordering(const Board &board, uint ply, vector<Move> &moves, const Move &best_move) {

    // Precompute scores to avoid recalculating in comparator frequently
    size_t n = moves.size();
    if (n <= 1) {
        return;
    }
    vector<int> scores;
    scores.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        scores.push_back(score_move(moves[i], board, ply, best_move));
    }

    // insertion sort
    for (size_t i = 1; i < n; ++i) {
        Move mv = moves[i];
        int sc = scores[i];
        size_t j = i;
        while (j > 0 && scores[j-1] < sc) {
            moves[j] = moves[j-1];
            scores[j] = scores[j-1];
            j--;
        }
        moves[j] = mv;
        scores[j] = sc;
    }
}