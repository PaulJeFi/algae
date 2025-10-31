#include <immintrin.h>

extern U64 BISHOP_MASKS[64];
extern U64 ROOK_MASKS[64];
extern U64 BISHOP_ATTACKS[64][512];
extern U64 ROOK_ATTACKS[64][4096];

U64 ratt(uint8_t sq, U64 block);
U64 batt(uint8_t sq, U64 block);
U64 rmask(int sq);
U64 bmask(int sq);

U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask);

void init_sliders_attacks(int bishop);
void init_sliders();

#if defined(__BMI2__) || defined(USE_BMI2_FORCE)
#else
    extern U64 BISHOP_MAGICS[64];
    extern U64 ROOK_MAGICS[64];
    U64 random_U64();
    U64 random_U64_fewbits();
    U64 find_magic_number(int square, int relevant_bits, int bishop);
#endif