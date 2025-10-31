#include "main.h"
#include "move.h"
#include "board.h"

Timer manage(int time_, Board &board, int inc, int movestogo) {
    // Logic from Smallbrain by Disservin
    Timer timer;

    int overhead;
    int ply = board.move_stack.size();

    if (movestogo == 0) {
        movestogo = 20;
        overhead = std::max(10 * (50 - ply / 2), 0);
    } else {
        overhead = std::max(10 * movestogo / 2, 0);
    }

    if (time_ - overhead >= 100)
        time_ -= overhead;

    timer.optimum = (int64_t)(time_ / (double)movestogo);
    timer.optimum += inc / 2;
    if (timer.optimum >= time_) {
        timer.optimum = (int64_t)std::max(1.0, time_ / 20.0);
    }

    timer.maximum = (int64_t)(timer.optimum * 1.05);
    if (timer.maximum >= time_) {
        timer.maximum = timer.optimum;
    } if ((timer.maximum == 0) || (timer.optimum == 0)) {
        timer.maximum = timer.optimum = 1;
    }

    if ((timer.optimum > timer.maximum)) {
        timer.optimum = timer.maximum;
    }
    
    return timer;

    /* Classical time management (unused)
    int Y = movestogo;
    if (Y == 0) {
        Y = max(10, 40 - (int)(board.move_stack.size()/2));
    }
    // return (int)floor(clamp(time_/20 + 2*inc, 0, time_/4));
    // return (int)floor(clamp(time_/Y + inc * Y/10, 0, time_/4));
    */
}