#include "internal/patika_internal.h"

void pcg32_init(PCG32* rng, uint64_t seed) {
    rng->state = seed;
}

uint32_t pcg32_next(PCG32* rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + 1442695040888963407ULL;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
