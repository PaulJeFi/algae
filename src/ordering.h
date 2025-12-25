extern int history[12][64];
extern Move killers[MAX_PLY][2];

void reset_tables();
void update_killers(const Move &move, uint ply);
int score_move(const Move &move, const Board &board, uint ply, const Move &best_move);
void ordering(const Board &board, uint ply, vector<Move> &moves, const Move &best_move = nullmove);
