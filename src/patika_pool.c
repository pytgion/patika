#include "internal/patika_internal.h"
#include <stdint.h>
#include <stdlib.h>

void barrack_pool_init(BarrackPool *pool, uint16_t capacity)
{
    pool->slots = calloc(capacity, sizeof(BarrackSlot));
    pool->capacity = capacity;
    pool->next_id = 0;
}

void barrack_pool_destroy(BarrackPool *pool)
{
    free(pool->slots);
}

BarrackSlot *barrack_pool_get(BarrackPool *pool, BarrackID id)
{
    if (id >= pool->capacity)
        return NULL;
    BarrackSlot *slot = &pool->slots[id];
    return slot->active ? slot : NULL;
}

void agent_pool_init(AgentPool *pool, uint32_t capacity)
{
    pool->slots = calloc(capacity, sizeof(AgentSlot));
    pool->capacity = capacity;
    pool->active_count = 0;
    pool->free_head = 0;
    for (uint32_t i = 0; i < capacity - 1; i++)
    { // check for -2
        pool->slots[i].next_free_index = i + 1;
    }
    pool->slots[capacity - 1].next_free_index = PATIKA_INVALID_AGENT_ID;
}

BarrackID barrack_pool_allocate(BarrackPool *pool)
{
    if (pool->next_id >= pool->capacity)
        return PATIKA_INVALID_BARRACK_ID;
    BarrackID id = pool->next_id;
    pool->slots[id].active = 1;
    pool->next_id++;
    return id;
}

void agent_pool_destroy(AgentPool *pool)
{
    if (pool)
    {
        free(pool->slots);
    }
}

AgentID agent_pool_allocate(AgentPool *pool)
{
    if (pool->active_count >= pool->capacity)
        return PATIKA_INVALID_AGENT_ID;
    if (pool->free_head == PATIKA_INVALID_AGENT_ID)
        return PATIKA_INVALID_AGENT_ID;
    uint16_t index = pool->free_head;
    pool->free_head = pool->slots[index].next_free_index;
    pool->slots[index].generation++;
    pool->slots[index].active = 1;
    pool->active_count++;
    AgentID id = make_agent_id(index, pool->slots[index].generation);
    pool->slots[index].id = id;
    return id;
}

void agent_pool_free(AgentPool *pool, AgentID id)
{
    uint16_t index = agent_index(id);
    pool->slots[index].active = 0;
    pool->slots[index].next_free_index = pool->free_head;
    pool->free_head = index;
    pool->active_count--;
}

AgentSlot *agent_pool_get(AgentPool *pool, AgentID id)
{
    uint16_t index = agent_index(id);
    if (index >= pool->capacity)
        return NULL;

    AgentSlot *slot = &pool->slots[index];

    // check if generation ID's match
    if (agent_generation(id) != slot->generation)
    {
        return NULL;
    }

    return slot->active ? slot : NULL;
}
