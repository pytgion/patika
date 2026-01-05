#include "internal/patika_internal.h"
#include <stdatomic.h>
#include <stdlib.h>

void mpsc_init(MPSCCommandQueue *q, uint32_t capacity) {
  q->buffer = (PatikaCommand *)calloc(capacity, sizeof(PatikaCommand));
  if (!q->buffer) {
    q->capacity = 0;
    return;
  }
  q->capacity = capacity;
  atomic_init(&q->head, 0);
  q->tail = 0;
}

void mpsc_destroy(MPSCCommandQueue *q) {
  free(q->buffer);
  q->buffer = NULL;
}

/**
 * @brief Pushes a command to the queue (thread-safe).
 * @return 0 on success, -1 if queue is full.
 */
PatikaError mpsc_push(MPSCCommandQueue *q, const PatikaCommand *cmd) {
  uint32_t current_head;
  uint32_t next_head;

  do {
    current_head = atomic_load_explicit(&q->head, memory_order_relaxed);
    next_head = (current_head + 1) % q->capacity;

    // Check if full
    if (next_head == q->tail) {
      return -1; // QUEUE_FULL
    }
  } while (!atomic_compare_exchange_weak_explicit(
      &q->head, &current_head, next_head, memory_order_release,
      memory_order_relaxed));

  // Write command to reserved slot
  q->buffer[current_head] = *cmd;

  return PATIKA_OK;
}

/**
 * @brief Pops a command from the queue (single consumer only).
 * @return 0 on success, -1 if queue is empty.
 */
PatikaError mpsc_pop(MPSCCommandQueue *q, PatikaCommand *out) {
  uint32_t head = atomic_load_explicit(&q->head, memory_order_acquire);

  if (q->tail == head) {
    return -1; // EMPTY
  }

  *out = q->buffer[q->tail];
  q->tail = (q->tail + 1) % q->capacity;

  return 0; // SUCCESS
}
