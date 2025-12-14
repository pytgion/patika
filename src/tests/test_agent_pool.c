/**
 * @file test_agent_pool.c
 * @brief Unit tests for agent pool allocation and management
 */

#include "internal/patika_internal.h"
#include "unity.h"
#include <string.h>

static AgentPool pool;

void setUp(void)
{
    agent_pool_init(&pool, 10);
}

void tearDown(void)
{
    agent_pool_destroy(&pool);
}

void test_agent_pool_init(void)
{
    TEST_ASSERT_NOT_NULL(pool.slots);
    TEST_ASSERT_EQUAL_UINT32(10, pool.capacity);
    TEST_ASSERT_EQUAL_UINT32(0, pool.active_count);
    TEST_ASSERT_EQUAL_UINT16(0, pool.free_head);
}

void test_agent_pool_allocate_single(void)
{
    AgentID id = agent_pool_allocate(&pool);

    TEST_ASSERT_NOT_EQUAL(PATIKA_INVALID_AGENT_ID, id);
    TEST_ASSERT_EQUAL_UINT16(0, agent_index(id));
    TEST_ASSERT_EQUAL_UINT16(1, agent_generation(id));
}

void test_agent_pool_allocate_multiple(void)
{
    AgentID ids[5];

    for (int i = 0; i < 5; i++)
    {
        ids[i] = agent_pool_allocate(&pool);
        TEST_ASSERT_NOT_EQUAL(PATIKA_INVALID_AGENT_ID, ids[i]);
        TEST_ASSERT_EQUAL_UINT16(i, agent_index(ids[i]));
    }

    // All IDs should be unique
    for (int i = 0; i < 5; i++)
    {
        for (int j = i + 1; j < 5; j++)
        {
            TEST_ASSERT_NOT_EQUAL(ids[i], ids[j]);
        }
    }
}

void test_agent_pool_allocate_capacity_exceeded(void)
{
    // Allocate all slots
    for (uint32_t i = 0; i < pool.capacity; i++)
    {
        AgentID id = agent_pool_allocate(&pool);
        TEST_ASSERT_NOT_EQUAL(PATIKA_INVALID_AGENT_ID, id);
    }

    // Next allocation should fail
    AgentID id = agent_pool_allocate(&pool);
    TEST_ASSERT_EQUAL(PATIKA_INVALID_AGENT_ID, id);
}

void test_agent_pool_get_valid(void)
{
    AgentID id = agent_pool_allocate(&pool);
    AgentSlot *slot = agent_pool_get(&pool, id);

    TEST_ASSERT_NOT_NULL(slot);
    TEST_ASSERT_EQUAL(id, slot->id);
    TEST_ASSERT_EQUAL_UINT8(1, slot->active);
}

void test_agent_pool_get_invalid_index(void)
{
    AgentID invalid_id = make_agent_id(999, 0);
    AgentSlot *slot = agent_pool_get(&pool, invalid_id);

    TEST_ASSERT_NULL(slot);
}

void test_agent_pool_get_wrong_generation(void)
{
    AgentID id = agent_pool_allocate(&pool);
    agent_pool_free(&pool, id);

    // Old ID with stale generation
    AgentSlot *slot = agent_pool_get(&pool, id);
    TEST_ASSERT_NULL(slot);
}

void test_agent_pool_free_and_reuse(void)
{
    AgentID id1 = agent_pool_allocate(&pool);
    uint16_t index1 = agent_index(id1);
    uint16_t gen1 = agent_generation(id1);

    agent_pool_free(&pool, id1);

    // Verify slot is inactive
    TEST_ASSERT_EQUAL_UINT8(0, pool.slots[index1].active);

    // Allocate again - should reuse same index with new generation
    AgentID id2 = agent_pool_allocate(&pool);
    uint16_t index2 = agent_index(id2);
    uint16_t gen2 = agent_generation(id2);

    TEST_ASSERT_EQUAL_UINT16(index1, index2);
    TEST_ASSERT_EQUAL_UINT16(gen1 + 1, gen2);
}

void test_agent_pool_generation_increment(void)
{
    uint16_t index = 0;

    for (int i = 0; i < 5; i++)
    {
        AgentID id = agent_pool_allocate(&pool);
        TEST_ASSERT_EQUAL_UINT16(index, agent_index(id));
        TEST_ASSERT_EQUAL_UINT16(i + 1, agent_generation(id));
        agent_pool_free(&pool, id);
    }
}

void test_agent_pool_make_and_parse_id(void)
{
    uint16_t index = 42;
    uint16_t gen = 17;

    AgentID id = make_agent_id(index, gen);

    TEST_ASSERT_EQUAL_UINT16(index, agent_index(id));
    TEST_ASSERT_EQUAL_UINT16(gen, agent_generation(id));
}

void test_agent_pool_multiple_free_reuse(void)
{
    // Allocate 5 agents
    AgentID ids[5];
    for (int i = 0; i < 5; i++)
    {
        ids[i] = agent_pool_allocate(&pool);
    }

    // Free agents 1 and 3
    agent_pool_free(&pool, ids[1]);
    agent_pool_free(&pool, ids[3]);

    // New allocations should reuse freed slots
    AgentID new_id1 = agent_pool_allocate(&pool);
    AgentID new_id2 = agent_pool_allocate(&pool);

    // Should get index 3 first (last freed), then index 1
    TEST_ASSERT_EQUAL_UINT16(3, agent_index(new_id1));
    TEST_ASSERT_EQUAL_UINT16(1, agent_index(new_id2));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_agent_pool_init);
    RUN_TEST(test_agent_pool_allocate_single);
    RUN_TEST(test_agent_pool_allocate_multiple);
    RUN_TEST(test_agent_pool_allocate_capacity_exceeded);
    RUN_TEST(test_agent_pool_get_valid);
    RUN_TEST(test_agent_pool_get_invalid_index);
    RUN_TEST(test_agent_pool_get_wrong_generation);
    RUN_TEST(test_agent_pool_free_and_reuse);
    RUN_TEST(test_agent_pool_generation_increment);
    RUN_TEST(test_agent_pool_make_and_parse_id);
    RUN_TEST(test_agent_pool_multiple_free_reuse);

    return UNITY_END();
}
