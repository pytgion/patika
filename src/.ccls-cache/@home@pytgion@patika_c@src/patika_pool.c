#include "internal/patika_internal.h"
#include <stdlib.h>

void barrack_pool_init(BarrackPool* pool, uint16_t capacity) {
    pool->slots = calloc(capacity, sizeof(BarrackSlot));
    pool->capacity = capacity;
    pool->next_id = 0;
}

void barrack_pool_destroy(BarrackPool* pool) {
    free(pool->slots);
}

BarrackSlot* barrack_pool_get(BarrackPool* pool, BarrackID id) {
    if (id >= pool->capacity) return NULL;
    BarrackSlot* slot = &pool->slots[id];
    return slot->active ? slot : NULL;
}

