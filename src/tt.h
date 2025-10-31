struct Entry {
    U64 key;
    uint depth;
    int value;
    int8_t flag;
    Move move;
    Entry(U64 key = 0ULL, int depth = -1, int value = 0, int8_t flag = -1, Move move = nullmove);
};

struct ProbeEntry {
    Move move;
    double value;
    ProbeEntry(Move move=nullmove, double value=0.5);
    bool is_none();
};

class TT {

    public :

        static inline const int size = 5 * 1024 * 1024;
        Entry tt[size];

        void clear();
        int score_to_tt(int score, uint ply);
        int score_from_tt(int score, uint ply, int rule50);
        ProbeEntry probe(Board &board, uint depth, int alpha, int beta, uint ply);
        void save(Board &board, uint depth, int8_t flag, int value, uint ply, Move move=nullmove, bool timeout=false);
        int hashfull();
};

extern TT tt;