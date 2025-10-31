struct State {
    uint8_t en_passant;
    uint8_t castle;
    uint movecount[2];

    State(uint8_t en_passant, uint8_t castle, uint movecount[2]) {
        this->en_passant = en_passant;
        this->castle = castle;
        this->movecount[0] = movecount[0];
        this->movecount[1] = movecount[1];
    }
};

class Board {
    public :

        U64 bitboards[12];
        U64 occupancy[3];
        U64 hash_key;
        uint8_t side;
        uint8_t en_passant;
        uint8_t castle;
        uint move_count[2];
        vector<State> state_history;
        vector<Move> move_stack;
        vector<U64> hash_history;
        int8_t board[64] = {None};

        Board(string fen=STARTING_FEN);
        void update_occupancy();
        inline void add_piece(int8_t piece, uint8_t square);
        inline void remove_piece(int8_t piece, uint8_t square);
        void zobrist_key();
        bool is_square_attacked(uint8_t square, uint8_t by_side);
        vector<Move> generate_moves();
        vector<Move> generate_captures();
        bool make(Move move);
        void unmake();
        void make_null();
        void unmake_null();
        bool is_repetition();
        vector<Move> generate_legal_moves();
        bool make_str_move(string str_move);
};