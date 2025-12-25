#include "main.h"
#include "move.h"
#include "board.h"
#include "search.h"
#include "ordering.h"
#include "evaluation.h"
#include "timeman.h"
#include "uci.h"

void UCI() {
    // logic from Rice chess engine : https://github.com/rafid-dev/rice/blob/main/src/uci.cpp

    Board board = Board(STARTING_FEN);
    Searcher searcher;

    string command;
    string token;

    reset_tables();

    std::cout << "Algae 1.1 by Paul JF" << std::endl;

    while (std::getline(std::cin, command)) {
        std::istringstream is(command);

        token.clear();
        is >> std::skipws >> token;

        if (token == "stop") {

            stop_flag.store(true, std::memory_order_relaxed);
            
            if (search_thread.joinable()) {
                search_thread.join();
            }
            
            stop_flag.store(false, std::memory_order_relaxed);
            is_searching = false;
            continue;

        } else if (token == "quit") {

            stop_flag.store(true, std::memory_order_relaxed);
            if (search_thread.joinable()) {
                search_thread.join();
            }
            break;
    
        } else if (token == "isready") {

            stop_flag.store(true, std::memory_order_relaxed);
            if (search_thread.joinable()) {
                search_thread.join();
            }
            stop_flag.store(false, std::memory_order_relaxed);
            is_searching = false;

            std::cout << "readyok" << std::endl;
            continue;

        } else if (token == "ucinewgame") {

            stop_flag.store(true, std::memory_order_relaxed);
            if (search_thread.joinable()) {
                search_thread.join();
            }
            stop_flag.store(false, std::memory_order_relaxed);
            is_searching = false;
            board = Board(STARTING_FEN);
            continue;

        } else if (token == "uci") {

            std::cout << "id name Algae 1.1" << std::endl;
            std::cout << "id author Paul JF" << std::endl;
            std::cout << "uciok" << std::endl;
            continue;
        }

        /* Handle UCI position command */
        else if (token == "position") {

             stop_flag.store(true, std::memory_order_relaxed);
            if (search_thread.joinable()) {
                search_thread.join();
            }
            stop_flag.store(false, std::memory_order_relaxed);

            std::string option;
            is >> std::skipws >> option;
            if (option == "startpos") {
                board = Board();
            } else if (option == "fen") {
                std::string final_fen;
                is >> std::skipws >> option;
                final_fen = option;

                // Record side to move
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // Record castling
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // record enpassant square
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // record fifty move conter
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // Finally, apply the fen.
                board = Board(final_fen);
            }
            is >> std::skipws >> option;
            if (option == "moves") {
                std::string moveString;

                while (is >> moveString) {
                    board.make_str_move(moveString);
                }
            }
            continue;
        }

        /* Handle UCI go command */
        else if (token == "go") {
            is >> std::skipws >> token;

            // Initialize variables
            int depth = -1;
            int movestogo = 0;
            int wtime = -1;
            int btime = -1;
            int winc = 0;
            int binc = 0;
            int movetime = -1;
            Timer timer;
            int nodes = -1;
            bool timeset = false;
            bool perft_ = false;

            while (token != "none") {
                if (token == "infinite") {
                    depth = -1;
                    break;
                }
                if (token == "movestogo") {
                    is >> std::skipws >> token;
                    movestogo = stoi(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Depth
                if (token == "depth") {
                    is >> std::skipws >> token;
                    depth = std::stoi(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Perft
                if (token == "perft") {
                    perft_ = true;
                    is >> std::skipws >> token;
                    depth = std::stoi(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Time
                if (token == "wtime") {
                    is >> std::skipws >> token;
                    wtime = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }
                if (token == "btime") {
                    is >> std::skipws >> token;
                    btime = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Increment
                if (token == "winc") {
                    is >> std::skipws >> token;
                    winc = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }
                if (token == "binc") {
                    is >> std::skipws >> token;
                    binc = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }

                if (token == "movetime") {
                    is >> std::skipws >> token;
                    movetime = stod(token);
                    is >> std::skipws >> token;
                    continue;
                }
                if (token == "nodes") {
                    is >> std::skipws >> token;
                    nodes = stoi(token);
                    is >> std::skipws >> token;
                }
                token = "none";
            }

            if (nodes != -1) {}

            if (wtime != -1 || btime != -1 || movetime != -1) {
                timeset = true;
            }

            if (depth == -1) {
                depth = MAX_PLY;
            }
            
            if (movetime > 0) {
                timer.optimum = movetime;
                timer.maximum = movetime;
            } else if (board.side == WHITE && timeset) {
                timer = manage(wtime, board, winc, movestogo);
            } else if (board.side == BLACK && timeset) {
                timer = manage(btime, board, binc, movestogo);
            }

            if (perft_ == true) {
                int p = PERFT(board, depth);
                std::cout << "Nodes searched: " << p << std::endl;
                continue;
            }

            stop_flag.store(true, std::memory_order_relaxed);
            if (search_thread.joinable()) {
                search_thread.join();
            }
            
            stop_flag.store(false, std::memory_order_relaxed);
            is_searching = true;
            
            search_thread = std::thread([&searcher, board, depth, timer]() {
                searcher.iterative_deepening(board, depth, timer);
            });
        }

        else if (token == "setoption") {
            continue;
        }

        /* Debugging Commands */
        else if (token == "d") {
            // TODO
            continue;
        } else if (token == "eval") {
            
            std::cout << "static eval: " << evaluate(board) << std::endl;

        }
    }
    std::cout << std::endl;
}