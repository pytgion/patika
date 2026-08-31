#include "internal/patika_internal.h"
#include <stdatomic.h>
#include <stdlib.h>

void mpsc_init(MPSCCommandQueue *q, uint32_t capacity) {
    q->buffer = calloc(capacity, sizeof(PatikaCommand));
    if (!q->buffer) {
        q->ready = NULL;
        q->capacity = 0;
        return;
    }

    q->ready = calloc(capacity, sizeof(_Atomic uint8_t));
    if (!q->ready) {
        free(q->buffer);
        q->buffer = NULL;
        q->capacity = 0;
        return;
    }

    q->capacity = capacity;
    atomic_init(&q->head, 0);
    q->tail = 0;

    for (uint32_t i = 0; i < capacity; i++) {
        atomic_init(&q->ready[i], 0);
    }
}

void mpsc_destroy(MPSCCommandQueue *q) {
    free(q->buffer);
    free(q->ready);
    q->buffer = NULL;
    q->ready = NULL;
}

/*
 * Thread-safe push (multiple producers allowed).
 *
 * The original implementation advanced `head` via CAS before writing the slot,
 * so a consumer could pop the slot before the producer finished writing it.
 *
 * Fix: write the slot first, then set a per-slot `ready` flag with release
 * semantics. The consumer spins on the flag before reading.
 */
PatikaError mpsc_push(MPSCCommandQueue *q, const PatikaCommand *cmd) {
    uint32_t current_head;
    uint32_t next_head;

    do {
        current_head = atomic_load_explicit(&q->head, memory_order_relaxed);
        next_head = (current_head + 1) % q->capacity;
        if (next_head == q->tail) {
            return -1; // QUEUE_FULL
        }
    } while (!atomic_compare_exchange_weak_explicit(
        &q->head, &current_head, next_head,
        memory_order_release, memory_order_relaxed));

    q->buffer[current_head] = *cmd;
    atomic_store_explicit(&q->ready[current_head], 1, memory_order_release);

    return PATIKA_OK;
}

/*
 * Single-consumer pop.
 *
 * Spins on the ready flag for the slot at `tail`. The window where head is
 * advanced but data is not yet written is nanoseconds (a struct copy), so
 * spinning here is safe and predictable.
 */
PatikaError mpsc_pop(MPSCCommandQueue *q, PatikaCommand *out) {
    uint32_t head = atomic_load_explicit(&q->head, memory_order_acquire);
    if (q->tail == head) {
        return -1; // EMPTY
    }

    while (!atomic_load_explicit(&q->ready[q->tail], memory_order_acquire)) {
        // producer claimed this slot but hasn't finished the write yet
    }

    *out = q->buffer[q->tail];
    atomic_store_explicit(&q->ready[q->tail], 0, memory_order_release);
    q->tail = (q->tail + 1) % q->capacity;

    return 0;
}
