#include "main.h"
#include "move.h"
#include "board.h"
#include "tt.h"
#include "evaluation.h"
#include "ordering.h"
#include "search.h"

uint perft(Board &board, uint depth) {
    
    uint nodes = 0;

    if (depth == 0) {
        return 1;
    }

    for (const Move move : board.generate_moves()) {
        if (!board.make(move)) {
            continue;
        }
        nodes += perft(board, depth - 1);
        board.unmake();
    }
    return nodes;
}

uint PERFT(Board &board, uint depth) {

    uint nodes = 0;
    uint node = 0;

    if (depth == 0) {
        return 1;
    }

    for (const Move move : board.generate_moves()) {
        if (!board.make(move)) {
            continue;
        }
        node = perft(board, depth - 1);
        std::cout << move.to_string() << ": " << node << std::endl;
        nodes += node;
        board.unmake();
    }
    return nodes;
}

std::atomic<bool> stop_flag{false};
std::thread search_thread;
bool is_searching = false;

int Searcher::quiesce(int alpha, int beta) {

    nodes++;
    seldepth = max(seldepth, ply);

    // Timeout check-up
    if ((nodes & 1023) == 0) {
        if ((stop_flag.load(std::memory_order_relaxed)) || (get_time() - start_time_ >= timer.maximum)) {
            timeout = true;
            return 0;
        }
    }
    if ((stop_flag.load(std::memory_order_relaxed)) || (timeout)) {
        timeout = true;
        return 0;
    }

    // Mate distance pruning
    if (ply > 0) {
        alpha = max(mated_in(ply), alpha);
        beta  = min(mate_in(ply + 1), beta);
        if (alpha >= beta) {
            return alpha;
        }
    }

    int stand_pat = evaluate(board);
    if (stand_pat >= beta) {
        return beta;
    }

    // Delta pruning
    if (stand_pat < alpha - mop_up_values[WQUEEN]) {
        return alpha;
    }

    if (alpha < stand_pat) {
        alpha = stand_pat;
    }

    int score = alpha - 1;
    ply++;
    vector<Move> moves = board.generate_captures(); 
    moves = ordering(board, ply, moves);
    for (const Move move : moves) {
        if (!board.make(move)) {
            continue;
        }

        // TODO : SEE
        score = -quiesce(-beta, -alpha);
        board.unmake();

        if (score >= beta) {
            alpha = beta;
            break;
        } if (score > alpha) {
            alpha = score;
        }
    }

    ply--;
    return alpha;
}

int Searcher::PVSearch(int alpha, int beta, int depth) {
    
    nodes++;
    seldepth = max(seldepth, ply);

    // Timeout check-up
    if ((nodes & 1023) == 0) {
        if ((stop_flag.load(std::memory_order_relaxed)) || (get_time() - start_time_ >= timer.maximum)) {
            timeout = true;
            return 0;
        }
    }
    if ((stop_flag.load(std::memory_order_relaxed)) || (timeout)) {
        timeout = true;
        return 0;
    }

    // Initialize node
    depth = min(depth, MAX_PLY-1);
    int8_t bound = BOUND_LOWER;
    Move bestmove = nullmove;
    bool in_check = board.is_square_attacked(ls1b_index(board.bitboards[board.side == WHITE ? WKING : BKING]), board.side^1);

    // Check extension
    if (in_check && (depth <= 0)) {
        depth = 1;
    }

    int score;

    // Quiescence
    if ((depth <= 0) || (ply >= MAX_PLY)) {
        score = quiesce(alpha, beta);
        tt.save(board, depth, BOUND_EXACT, score, ply, nullmove, timeout);
        return score;
    }

    // TODO : insufficient material
    if (board.is_repetition() || board.move_count[0] >= 100) {
        alpha = 0;
        if (alpha >= beta) {
            return alpha;
        }
    }

    // Mate distance pruning
    if (ply > 0) {
        alpha = max(mated_in(ply), alpha);
        beta  = min(mate_in(ply + 1), beta);
        if (alpha >= beta) {
            return alpha;
        }
    }

    // TT probe
    bool probe_suceeded = false;
    ProbeEntry probe = tt.probe(board, depth, alpha, beta, ply);
    if (!probe.is_none()) {
        probe_suceeded = true;
        if ((probe.value != 0.5) && (ply != 0) && (beta-alpha == 1)) {
            return probe.value;
        }
        bestmove = probe.move;
    }

    int evalu = evaluate(board);

    // Futility pruning
    if (!in_check && (evalu - 120*depth >= beta) && (beta-alpha == 1) && probe_suceeded) {
        return quiesce(alpha, beta);//evalu;
    }

    // Razoring
    if (!in_check && (depth < 3) && (evalu + 120 < alpha) && (beta-alpha == 1)) {
        return quiesce(alpha, beta);
    }

    // Null move pruning
    if ((beta-alpha == 1) && !in_check && !(board.move_stack.back() == nullmove)
    && (evalu >= beta)
    && (popcount(board.occupancy[board.side]) - popcount(board.bitboards[board.side == WHITE ? WPAWN : BPAWN]) > 1)
    && (beta > VALUE_TB_LOSS_IN_MAX_PLY) && (ply > 0)) {
        

        int R = 3;
        ply += 1;
        board.make_null();
        int value = -PVSearch(-beta, -beta+1, depth-R);
        board.unmake_null();
        ply -= 1;
    
        // verification search
        if ((value >= beta) && (value < VALUE_TB_WIN_IN_MAX_PLY)) {
            return value;
        }
    }

    // Internal iterative reduction
    if (!probe_suceeded && (beta-alpha == 1) && (depth > 5) && (ply > 0)) {
        depth -= 3;
    }

    uint legal = 0;
    ply++;
    vector<Move> moves = board.generate_moves(); 
    moves = ordering(board, ply, moves, bestmove);
    for (const Move move : moves) {

        // Late move reduction
        //* TODO : TEST
        if ((depth > 3) && (beta-alpha == 1) && (move.captured == None) && (legal == 7)) {
            depth -= 3;
        }
        //*/

        if (!board.make(move)) {
            continue;
        }
        legal++;
        if (legal == 1) {
            score = -PVSearch(-beta, -alpha, depth-1);
            bestmove = move;
        } else {
            score = -PVSearch(-alpha-1, -alpha, depth-1);
            if ((score > alpha) && (beta - alpha > 1)) {
                score = -PVSearch(-beta, -alpha, depth-1); // beta-alpha > 1 : PV node -> re-search
            }
        }
        board.unmake();
        if (score >= beta) {
            alpha = beta;
            bound = BOUND_UPPER;
            bestmove = move;

            if (move.captured == None) {
                update_killers(move, ply);
                history[board.board[move.from_sq]][move.to_sq] += depth;
            }

            break;

        } if (score > alpha) {
            alpha = score;
            bound = BOUND_EXACT;
            bestmove = move;
        }
    }

    ply--;
    
    // Stalemate and checkmate detection
    if (legal == 0) {
        if (in_check) {
            alpha = -VALUE_MATE;
        } else {
            alpha = 0;
        }
    }

    tt.save(board, depth, bound, alpha, ply, bestmove, timeout);
    return alpha;
}

int Searcher::aspiration(int depth, int prev_eval) {

    int delta = 30;
    int alpha = prev_eval - delta;
    int beta = prev_eval + delta;
    int score = 0;

    while (true) {

        if (alpha < -3500) {
            alpha = -VALUE_MATE;
        }
        if (beta > 3500) {
            beta = VALUE_MATE;
        }

        score = PVSearch(alpha, beta, depth);

        if (timeout) {
            return 0;
        }

        if (score <= alpha) {
            beta = (int)((alpha + beta) / 2);
            alpha = max(alpha - delta, -VALUE_MATE);
            delta += (int)(delta / 2);
        } else if (score >= beta) {
            beta = min(beta + delta, VALUE_MATE);
            delta += (int)(delta / 2);
        } else {
            break;
        }
    }

    return score;
}

void Searcher::iterative_deepening(Board board, int depth, Timer timer) {
    vector<Move> moves = board.generate_legal_moves();
    Move bestmove = nullmove;

    if (moves.size() == 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "info depth 1 score cp " << evaluate(board) << std::endl;
        std::cout << "best move " << moves.back().to_string() << std::endl;
    }

    this->board = board;
    this->nodes = 0;
    this->ply = 0;
    this->timeout = false;
    this->seldepth = 0;

    long long start_time = get_time();
    if (timer.maximum != LONG_MAX) {
        depth = MAX_PLY;
    }
    long long elapsed = get_time() - start_time;

    Move second_m = nullmove;
    int prev_eval = VALUE_MATE;
    int evalu;

    for (int currdepth = 1; currdepth <= depth; currdepth++) {

        evalu = VALUE_MATE;
        this->timer.optimum = timer.optimum - elapsed;
        this->timer.maximum = timer.maximum - elapsed;
        this->start_time_ = get_time();
        this->nodes = 0;
        this->ply = 0;
        this->timeout = false;
        this->seldepth = 0;

        if (false) {//(currdepth >= 5) {
            evalu = aspiration(currdepth, prev_eval);
        } else {
            evalu = PVSearch(-VALUE_MATE, VALUE_MATE, currdepth);
        }

        elapsed = get_time() - start_time;
        prev_eval = evalu;

        vector<Move> pv = PV();
        if (pv.size() == 0) {
            std::cout << nullmove.to_string() << std::endl;
            continue;
            //return;
        }
        bestmove = pv.at(0);
        if (pv.size() > 1) {
            second_m = pv.at(1);
        }

        if ((this->timeout )) {
            std::cout << "bestmove " << bestmove.to_string() << " ponder " << second_m.to_string() << std::endl;
            return;
        }

        std::cout << "info depth " << currdepth << " seldepth " << this->seldepth << " score " << display_eval(evalu);
        std::cout << " nodes " << this->nodes << " nps " << (int)(1000*nodes / (elapsed+1));
        std::cout << " time " << (int)(elapsed) << " hashfull " << tt.hashfull() << " pv " << string_pv() << std::endl;

        if ((elapsed >= timer.optimum) || (currdepth >= depth)) {
            std::cout << "bestmove " << bestmove.to_string() << " ponder " << second_m.to_string() << std::endl;
            return;
        }

    }

    std::cout << "bestmove " << bestmove.to_string() << " ponder " << second_m.to_string() << std::endl;

}

vector<Move> Searcher::PV() {

    vector<Move> pv;
    Entry entry;

    while (true) {

        entry = tt.tt[board.hash_key & (tt.size - 1)];

        if ((entry.key == board.hash_key) && !(entry.move == nullmove)) {
            if (!board.make(entry.move)) {
                break;
            }

            pv.push_back(entry.move);
            
            if (board.is_repetition()) {
                break;
            }

        } else {
            break;
        }

    }

    for (size_t i = 0; i < pv.size(); i++) {
        board.unmake();
    }

    return pv;

}

string Searcher::string_pv() {

    vector<Move> pv = PV();

    if (pv.empty()) {
        return "";
    }
    
    string str_pv = pv[0].to_string();
    for (size_t i = 1; i < pv.size(); i++) {
        str_pv += " " + pv[i].to_string();
    }
    return str_pv;
}