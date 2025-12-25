#include "main.h"
#include "move.h"
#include "board.h"
#include "tt.h"

Entry::Entry(U64 key, int depth, int value, int8_t flag, Move move) {
        this->key = key;
        this->depth = depth;
        this->value = value;
        this->flag = flag;
        this->move = move;
}

ProbeEntry::ProbeEntry(Move move, double value) {
        this->move = move;
        this->value = value;
}

bool ProbeEntry::is_none() {
    if ((move == nullmove) && (value == 0.5)) {
        return true;
    }
    return false;
}

void TT::clear() {
    for (int i=0; i<size; i++) {
        tt[i] = Entry();
    }
}

int TT::score_to_tt(int score, uint ply) {
    if (score >= VALUE_MATE - 2 * MAX_PLY) {
        return score + ply;
    } else if (score <= -(VALUE_MATE - 2 * MAX_PLY)) {
        return score - ply;
    }
    return score;
}

int TT::score_from_tt(int score, uint ply, int rule50) {

    if (score >= VALUE_MATE - 2 * MAX_PLY) { // win
        if ((score >= VALUE_MATE - MAX_PLY) && (VALUE_MATE - score > 99 - rule50)) {
            return VALUE_MATE - MAX_PLY - 1; // return only true mate score
        }
        return score - ply;
    }

    if (score <= -(VALUE_MATE - 2 * MAX_PLY)) { // loss
        if ((score <= -(VALUE_MATE - MAX_PLY)) && (VALUE_MATE + score > 99 - rule50)) {
            return -(VALUE_MATE - MAX_PLY) + 1; // return only true mate score
        }
        return score + ply;
    }

    return score;
}

ProbeEntry TT::probe(const Board &board, uint depth, int alpha, int beta, uint ply) {

    Entry &entry = tt[board.hash_key & (size-1)];
    int value;

    if (board.hash_key == entry.key) {

        if (depth <= entry.depth) {
            value = score_from_tt(entry.value, ply, board.move_count[0]);
            if (entry.flag == BOUND_EXACT) {
                return ProbeEntry(entry.move, value);
            } if ((entry.flag == BOUND_LOWER) && (value <= alpha)) {
                return ProbeEntry(entry.move, alpha);
            } if ((entry.flag == BOUND_UPPER) && (value >= beta)) {
                return ProbeEntry(entry.move, beta);
            }
        }
            
        return ProbeEntry(entry.move);
    }

    return ProbeEntry();
}

void TT::save(const Board &board, uint depth, int8_t flag, int value, uint ply, Move move, bool timeout) {

    if (timeout) {
        return;
    }

    Entry &entry = tt[board.hash_key & (size-1)];

    if ((board.hash_key == entry.key) && (entry.depth > depth) && (ply != 0)) {
        if (entry.move == nullmove) {
            entry.move = move;
        }
        return;
    }
    if ((board.hash_key != entry.key) || !(move == nullmove)) { // we can keep the old ttmove if we have the same hash but no new move
        entry.move  = move;
    }

    entry.key   = board.hash_key;
    entry.depth = depth;
    entry.value = score_to_tt(value, ply);
    entry.flag  = flag;
}

int TT::hashfull() {

    int counter = 0;
    for (int i=0; i < min(size, 1000); i++) {
        if (tt[i].flag != -1) {
            counter += 1;
        }
    }
    if (size >= 1000) {  
        return counter;
    }
    return (int) ceil(1000 * counter / size);
}

TT tt;