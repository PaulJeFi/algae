#include <cstdlib>
#include "main.h"
#include "attacks.h"
#include "sliding_movegen.h"

U64 BISHOP_MASKS[64];
U64 ROOK_MASKS[64];
U64 BISHOP_ATTACKS[64][512];
U64 ROOK_ATTACKS[64][4096];

U64 ratt(uint8_t sq, U64 block) {
    // code by Tord Romstad
    U64 result = 0ULL;
    int rk = sq/8, fl = sq%8, r, f;
    for (r = rk+1; r <= 7; r++) {
        result |= (1ULL << (fl + r*8));
        if(block & (1ULL << (fl + r*8))) break;
    }
    for (r = rk-1; r >= 0; r--) {
        result |= (1ULL << (fl + r*8));
        if(block & (1ULL << (fl + r*8))) break;
    }
    for (f = fl+1; f <= 7; f++) {
        result |= (1ULL << (f + rk*8));
        if(block & (1ULL << (f + rk*8))) break;
    }
    for (f = fl-1; f >= 0; f--) {
        result |= (1ULL << (f + rk*8));
        if(block & (1ULL << (f + rk*8))) break;
    }
    return result;
}

U64 batt(uint8_t sq, U64 block) {
    // code by Tord Romstad
    U64 result = 0ULL;
    int rk = sq/8, fl = sq%8, r, f;
    for (r = rk+1, f = fl+1; r <= 7 && f <= 7; r++, f++) {
        result |= (1ULL << (f + r*8));
        if(block & (1ULL << (f + r * 8))) break;
    }
    for (r = rk+1, f = fl-1; r <= 7 && f >= 0; r++, f--) {
        result |= (1ULL << (f + r*8));
        if(block & (1ULL << (f + r * 8))) break;
    }
    for (r = rk-1, f = fl+1; r >= 0 && f <= 7; r--, f++) {
        result |= (1ULL << (f + r*8));
        if(block & (1ULL << (f + r * 8))) break;
    }
    for (r = rk-1, f = fl-1; r >= 0 && f >= 0; r--, f--) {
        result |= (1ULL << (f + r*8));
        if(block & (1ULL << (f + r * 8))) break;
    }
    return result;
}

U64 rmask(int sq) {
    // code by Tord Romstad
    U64 result = 0ULL;
    int rk = sq/8, fl = sq%8, r, f;
    for(r = rk+1; r <= 6; r++) result |= (1ULL << (fl + r*8));
    for(r = rk-1; r >= 1; r--) result |= (1ULL << (fl + r*8));
    for(f = fl+1; f <= 6; f++) result |= (1ULL << (f + rk*8));
    for(f = fl-1; f >= 1; f--) result |= (1ULL << (f + rk*8));
    return result;
}

U64 bmask(int sq) {
    // code by Tord Romstad
    U64 result = 0ULL;
    int rk = sq/8, fl = sq%8, r, f;
    for(r=rk+1, f=fl+1; r<=6 && f<=6; r++, f++) result |= (1ULL << (f + r*8));
    for(r=rk+1, f=fl-1; r<=6 && f>=1; r++, f--) result |= (1ULL << (f + r*8));
    for(r=rk-1, f=fl+1; r>=1 && f<=6; r--, f++) result |= (1ULL << (f + r*8));
    for(r=rk-1, f=fl-1; r>=1 && f>=1; r--, f--) result |= (1ULL << (f + r*8));
    return result;
}

U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask) {
    //from the BBC tutorial

    U64 occupancy = 0ULL;

    for (int count = 0; count < bits_in_mask; count++) {

        int square = ls1b_index(attack_mask);
        attack_mask = remove_square(attack_mask, square);

        if (index & (1 << count)) {
            occupancy |= (1ULL << square);
        }
    }

    return occupancy;
}

#if defined(__BMI2__) || defined(USE_BMI2_FORCE)
    
    /*
    #if !defined(__PEXT__) || !defined(_MSC_VER)
        static inline U64 _pext_u64(U64 val, U64 mask) {
            U64 res = 0ULL;
            for (U64 bb = 1; mask; bb += bb) {
                if (val & mask & -mask) {
                    res |= bb;
                }
                mask &= mask - 1;
            }
            return res;
        }
    #endif
    */

    void init_sliders_attacks(int bishop) {
        for (int square = 0; square < 64; square++) {
            
            U64 attack_mask = bishop ? BISHOP_MASKS[square] : ROOK_MASKS[square];
            int relevant_bits_count = popcount(attack_mask);
            int occupancy_indices = (1 << relevant_bits_count);
            
            for (int index = 0; index < occupancy_indices; index++) {
                if (bishop) {
                    U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                    BISHOP_ATTACKS[square][_pext_u64(occupancy, BISHOP_MASKS[square])] = batt(square, occupancy);
                } else {
                    U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                    ROOK_ATTACKS[square][_pext_u64(occupancy, ROOK_MASKS[square])] = ratt(square, occupancy);
                
                }
            }
        }
    }

    void init_sliders() {
        // std::cout << "info string using PEXT bitboards" << std::endl;
        for (int square = 0; square < 64; square++) {
            ROOK_MASKS[square] = rmask(square);
            BISHOP_MASKS[square] = bmask(square);
        }
        init_sliders_attacks(0);
        init_sliders_attacks(1);
    }

#else

    U64 BISHOP_MAGICS[64];
    U64 ROOK_MAGICS[64];

    U64 random_U64() {
        // code by Tord Romstad
        U64 u1, u2, u3, u4;
        u1 = (U64)(rand()) & 0xFFFF; u2 = (U64)(rand()) & 0xFFFF;
        u3 = (U64)(rand()) & 0xFFFF; u4 = (U64)(rand()) & 0xFFFF;
        return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
    }

    U64 random_U64_fewbits() {
        // code by Tord Romstad
        return random_U64() & random_U64() & random_U64();
    }

    U64 find_magic_number(int square, int relevant_bits, int bishop) {

        U64 occupancies[4096];
        U64 attacks[4096];
        U64 attack_mask = bishop ? bmask(square) : rmask(square);
        int occupancy_indices = 1 << relevant_bits;
        U64 magic_number;
        int fail = 0;

        for (int index=0; index < occupancy_indices; index++) {
            occupancies[index] = set_occupancy(index, relevant_bits, attack_mask);
            attacks[index] = bishop ? batt(square, occupancies[index]) : ratt(square, occupancies[index]);
        }

        for (int random_count=0; random_count < 1000000000; random_count++) {
            magic_number = random_U64_fewbits();

            if (popcount((attack_mask * magic_number) & 0xFF00000000000000ULL) < 6) {
                continue;
            }

            U64 used[4096] = {0ULL};
            int index;

            for (index = 0, fail = 0; !fail && index < occupancy_indices; index++) {

                int magic_index = (int)((occupancies[index] * magic_number) >> (64 - relevant_bits));

                if (used[magic_index] == 0ULL) {
                    used[magic_index] = attacks[index];
                } else if (used[magic_index] != attacks[index]) {
                    fail = 1;
                }
            }

            if (!fail) {
                return magic_number;
            }

        }

        printf("Failed\n");

        return 0ULL;
    }
    
    void init_sliders_attacks(int bishop) {
        for (int square = 0; square < 64; square++) {
            
            U64 attack_mask = bishop ? BISHOP_MASKS[square] : ROOK_MASKS[square];
            int relevant_bits_count = popcount(attack_mask);
            int occupancy_indices = (1 << relevant_bits_count);
            
            for (int index = 0; index < occupancy_indices; index++) {
                if (bishop) {
                    U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                    int magic_index = (occupancy * BISHOP_MAGICS[square]) >> (64 - BISHOP_RELEVANT_BITS[square]);
                    BISHOP_ATTACKS[square][magic_index] = batt(square, occupancy);
                } else {
                    U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                    int magic_index = (occupancy * ROOK_MAGICS[square]) >> (64 - ROOK_RELEVANT_BITS[square]);
                    ROOK_ATTACKS[square][magic_index] = ratt(square, occupancy);
                
                }
            }
        }
    }

    void init_sliders() {
        // std::cout << "info string using Magic bitboards" << std::endl;
        for (int square = 0; square < 64; square++) {
            ROOK_MAGICS[square] = find_magic_number(square, ROOK_RELEVANT_BITS[square], 0);
            BISHOP_MAGICS[square] = find_magic_number(square, BISHOP_RELEVANT_BITS[square], 1);
            ROOK_MASKS[square] = rmask(square);
            BISHOP_MASKS[square] = bmask(square);
        }
        init_sliders_attacks(0);
        init_sliders_attacks(1);
    }

#endif