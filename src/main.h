#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstdint>
#include <climits>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

using namespace std;
using std::string;

#define U64 unsigned long long
typedef unsigned int uint;

const int8_t None = -1;

const uint8_t WHITE = 0;
const uint8_t BLACK = 1;
const uint8_t BOTH  = 2;

const U64 FULL        = 0xffffffffffffffffULL;
const U64 EMPTY       = 0ULL;
const U64 NOT_A_FILE  = 0xfefefefefefefefeULL;
const U64 NOT_H_FILE  = 0x7f7f7f7f7f7f7f7fULL;
const U64 NOT_HG_FILE = 0x3f3f3f3f3f3f3f3fULL;
const U64 NOT_AB_FILE = 0xfcfcfcfcfcfcfcfcULL;

const U64 FILES[8] = {
    0x101010101010101ULL, 0x202020202020202ULL, 0x404040404040404ULL, 0x808080808080808ULL,
    0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL
};

const U64 ISOLATED_MASK[8] = {
    0x202020202020202ULL, 0x505050505050505ULL, 0xa0a0a0a0a0a0a0aULL, 0x1414141414141414ULL,
    0x2828282828282828ULL, 0x5050505050505050ULL, 0xa0a0a0a0a0a0a0a0ULL, 0x4040404040404040ULL
};

static inline uint8_t popcount(U64 x) {
    return __builtin_popcountll(x);
}

static inline uint8_t ls1b_index(U64 b) {
    return b ? __builtin_ctzll(b) : -1;
}

static inline bool contains_square(U64 &bitboard, int square) {
    return (bitboard & (1ULL << square)) != 0;
}

static inline U64 add_square(U64 bitboard, int square) {
    return bitboard | (1ULL << square);
}

static inline U64 remove_square(U64 bitboard, int square) {
    return bitboard & ~(1ULL << square);
}

enum Square {
    A1 = 0, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

const uint8_t no_sq = 64;

const string SQUARES_NAMES[65] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "NONE"
};

static inline uint8_t square_file(int sq) {
    return sq & 7;
}

static inline uint8_t square_rank(int sq) {
    return sq >> 3;
}

static inline uint8_t square_manhattan_distance(int sq1, int sq2) {
    return abs(square_file(sq1) - square_file(sq2)) + abs(square_rank(sq1) - square_rank(sq2));
}

const uint8_t castle_mask[64] = { // castle_right &= castle_mask[from] & castle_mask[to]
   13, 15, 15, 15, 12, 15, 15, 14,
   15, 15, 15, 15, 15, 15, 15, 15,  
   15, 15, 15, 15, 15, 15, 15, 15,  
   15, 15, 15, 15, 15, 15, 15, 15,  
   15, 15, 15, 15, 15, 15, 15, 15,  
   15, 15, 15, 15, 15, 15, 15, 15,  
   15, 15, 15, 15, 15, 15, 15, 15,  
    7, 15, 15, 15,  3, 15, 15, 11
};

// Castling rights encoding :
const uint8_t WK = 0b0001;
const uint8_t WQ = 0b0010;
const uint8_t BK = 0b0100;
const uint8_t BQ = 0b1000;

// Pieces encoding
enum PIECES {
    WPAWN= 0, WKNIGHT, WBISHOP, WROOK, WQUEEN, WKING,
    BPAWN,    BKNIGHT, BBISHOP, BROOK, BQUEEN, BKING
};

const string pieces_char = "PNBRQKpnbrqk";
const string STARTING_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

const uint8_t CMD[64] = {
    6,  5,  4,  3,  3,  4,  5,  6,
    5,  4,  3,  2,  2,  3,  4,  5,
    4,  3,  2,  1,  1,  2,  3,  4,
    3,  2,  1,  0,  0,  1,  2,  3,
    3,  2,  1,  0,  0,  1,  2,  3,
    4,  3,  2,  1,  1,  2,  3,  4,
    5,  4,  3,  2,  2,  3,  4,  5,
    6,  5,  4,  3,  3,  4,  5,  6
};

const uint8_t BOUND_EXACT = 0;
const uint8_t BOUND_LOWER = 1; // alpha
const uint8_t BOUND_UPPER  = 2; // beta

const int VALUE_MATE = 32000;
const int MAX_PLY = 256;

const int VALUE_MATE_IN_MAX_PLY  = VALUE_MATE - MAX_PLY;
const int VALUE_MATED_IN_MAX_PLY = -VALUE_MATE_IN_MAX_PLY;

const int VALUE_TB                 = VALUE_MATE_IN_MAX_PLY - 1;
const int VALUE_TB_WIN_IN_MAX_PLY  = VALUE_TB - MAX_PLY;
const int VALUE_TB_LOSS_IN_MAX_PLY = -VALUE_TB_WIN_IN_MAX_PLY;

static inline int mate_in(int ply) {
    return VALUE_MATE - ply;
};

static inline int mated_in(int ply) {
    return -VALUE_MATE + ply;
}

const int MAX_HISTORY = 1000;

const int MVV_LVA[12][12] = {
    // Victim     P    N    B    R    Q    K    P    N    B    R    Q    K  / Attacker
                {105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605}, //   P
                {104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604}, //   N
                {103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603}, //   B
                {102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602}, //   R
                {101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601}, //   Q
                {100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600}, //   K
                {105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605}, //   P
                {104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604}, //   N
                {103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603}, //   B
                {102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602}, //   R
                {101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601}, //   Q
                {100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600}  //   K
};

struct Timer
{
    long long optimum = LONG_MAX;
    long long maximum = LONG_MAX;
};

static inline long long get_time() {
    clock_t currentTime = clock();
    return static_cast<long long>(currentTime) * 1000 / CLOCKS_PER_SEC;
}

static inline std::vector<string> split(string str, string sep=" ") {
    std::vector<string> splitted;
    size_t pos = 0;
    while ((pos = str.find(sep)) != string::npos) {
        splitted.push_back(str.substr(0, pos));
        str.erase(0, pos + sep.length());
    }
    if (!str.empty()) {
        splitted.push_back(str);
    }
    return splitted;
}

static inline string display_eval(int evalu) {
    if ((evalu >= VALUE_MATE - MAX_PLY) || (-evalu >= VALUE_MATE - MAX_PLY)) {
        return "mate " + to_string((int)(((evalu > 0) ? 1 : -1) * ceil((VALUE_MATE - ceil(abs(evalu))) / 2)));
    }
    return "cp " + to_string(evalu);
}