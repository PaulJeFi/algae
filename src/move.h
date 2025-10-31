class Move {
    public :

        uint8_t from_sq;
        uint8_t to_sq;
        int8_t captured;
        int8_t promoted;
        bool is_ep;

        Move(uint8_t from_sq=no_sq, uint8_t to_sq=no_sq, int8_t captured=None, int8_t promoted=None, bool is_ep=false) {
            this->from_sq = from_sq;
            this->to_sq = to_sq;
            this->captured = captured;
            this->promoted = promoted;
            this->is_ep = is_ep;
        }

        string to_string() const {
            string move_str = SQUARES_NAMES[from_sq] + SQUARES_NAMES[to_sq];
            if (promoted != None) {
                move_str += std::tolower(pieces_char[promoted]);
            }
            if (move_str == string("NONENONE")) {
                return string("0000");
            }
            return move_str;
        }

        bool operator==(const Move other) const {
            return from_sq == other.from_sq &&
                   to_sq == other.to_sq &&
                   captured == other.captured &&
                   promoted == other.promoted &&
                   is_ep == other.is_ep;
        }
};

const Move nullmove;