uint perft(Board &board, uint depth);
uint PERFT(Board &board, uint depth);

extern std::atomic<bool> stop_flag;
extern std::thread search_thread;
extern bool is_searching;

class Searcher {

    private :
        uint ply = 0;

    public :

        Board board;
        uint nodes = 0;
        uint seldepth = 0;
        bool timeout;
        Timer timer;
        long long start_time_ = 0;

        int quiesce(int alpha, int beta);
        int PVSearch(int alpha, int beta, int depth);
        int aspiration(int depth, int prev_eval);
        void iterative_deepening(Board board, int depth, Timer timer);
        vector<Move> PV();
        string string_pv();
};