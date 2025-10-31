extern int history[12][64];
extern Move killers[MAX_PLY][2];

void reset_tables();
void update_killers(Move move, uint ply);
int score_move(Move move, Board &board, uint ply, Move &best_move);
vector<Move> ordering(Board &board, uint ply, vector<Move> &moves, Move best_move = nullmove);
