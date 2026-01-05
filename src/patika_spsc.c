#include "internal/patika_internal.h"
#include <stdatomic.h>
#include <stdlib.h>

void spsc_init(SPSCEventQueue *q, uint32_t capacity) {
  q->buffer = calloc(capacity, sizeof(PatikaEvent));
  if (!q->buffer) {
    q->capacity = 0;
    return;
  }
  q->capacity = capacity;
  atomic_init(&q->head, 0);
  atomic_init(&q->tail, 0);
}

void spsc_destroy(SPSCEventQueue *q) {
  free(q->buffer);
  q->buffer = NULL;
}

/**
 * @brief Pushes an event to the queue (single producer only).
 * @return 0 on success, -1 if queue is full.
 */
PatikaError spsc_push(SPSCEventQueue *q, const PatikaEvent *evt) {
  uint32_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
  uint32_t next_head = (head + 1) % q->capacity;
  uint32_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);

  if (next_head == tail) {
    return PATIKA_ERR_CAPACITY; // FULL
  }

  q->buffer[head] = *evt;
  atomic_store_explicit(&q->head, next_head, memory_order_release);

  return PATIKA_OK; // SUCCESS
}

/**
 * @brief Pops an event from the queue (single consumer only).
 * @return 0 on success, -1 if queue is empty.
 */
PatikaError spsc_pop(SPSCEventQueue *q, PatikaEvent *out) {
  uint32_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
  uint32_t head = atomic_load_explicit(&q->head, memory_order_acquire);

  if (tail == head) {
    return PATIKA_ERR_CAPACITY; // EMPTY
  }

  *out = q->buffer[tail];
  atomic_store_explicit(&q->tail, (tail + 1) % q->capacity,
                        memory_order_release);

  return PATIKA_OK; // SUCCESS
}
