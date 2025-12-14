/**
 * @file test_barrack_pool.c
 * @brief Unit tests for barrack pool allocation
 */

#include "internal/patika_internal.h"
#include "unity.h"

static BarrackPool pool;

void setUp(void)
{
    barrack_pool_init(&pool, 5);
}

void tearDown(void)
{
    barrack_pool_destroy(&pool);
}

void test_barrack_pool_init(void)
{
    TEST_ASSERT_NOT_NULL(pool.slots);
    TEST_ASSERT_EQUAL_UINT16(5, pool.capacity);
    TEST_ASSERT_EQUAL_UINT16(0, pool.next_id);
}

void test_barrack_pool_allocate_single(void)
{
    BuildingID id = barrack_pool_allocate(&pool);

    TEST_ASSERT_NOT_EQUAL(PATIKA_INVALID_BARRACK_ID, id);
    TEST_ASSERT_EQUAL_UINT16(0, id);
    TEST_ASSERT_EQUAL_UINT16(1, pool.next_id);
}

void test_barrack_pool_allocate_multiple(void)
{
    for (uint16_t i = 0; i < 5; i++)
    {
        BuildingID id = barrack_pool_allocate(&pool);
        TEST_ASSERT_EQUAL_UINT16(i, id);
    }

    TEST_ASSERT_EQUAL_UINT16(5, pool.next_id);
}

void test_barrack_pool_allocate_capacity_exceeded(void)
{
    // Allocate all slots
    for (uint16_t i = 0; i < pool.capacity; i++)
    {
        BuildingID id = barrack_pool_allocate(&pool);
        TEST_ASSERT_NOT_EQUAL(PATIKA_INVALID_BARRACK_ID, id);
    }

    // Next allocation should fail
    BuildingID id = barrack_pool_allocate(&pool);
    TEST_ASSERT_EQUAL(PATIKA_INVALID_BARRACK_ID, id);
}

void test_barrack_pool_get_valid(void)
{
    BuildingID id = barrack_pool_allocate(&pool);
    BarrackSlot *slot = barrack_pool_get(&pool, id);

    TEST_ASSERT_NOT_NULL(slot);
    TEST_ASSERT_EQUAL(id, slot->id);
    TEST_ASSERT_EQUAL_UINT8(1, slot->active);
}

void test_barrack_pool_get_invalid_id(void)
{
    BarrackSlot *slot = barrack_pool_get(&pool, 999);
    TEST_ASSERT_NULL(slot);
}

void test_barrack_pool_get_inactive_slot(void)
{
    BuildingID id = barrack_pool_allocate(&pool);
    pool.slots[id].active = 0; // Manually deactivate

    BarrackSlot *slot = barrack_pool_get(&pool, id);
    TEST_ASSERT_NULL(slot);
}

void test_barrack_pool_sequential_allocation(void)
{
    BuildingID ids[3];

    for (int i = 0; i < 3; i++)
    {
        ids[i] = barrack_pool_allocate(&pool);
        TEST_ASSERT_EQUAL_UINT16(i, ids[i]);
    }

    // Verify all are active
    for (int i = 0; i < 3; i++)
    {
        BarrackSlot *slot = barrack_pool_get(&pool, ids[i]);
        TEST_ASSERT_NOT_NULL(slot);
        TEST_ASSERT_EQUAL_UINT8(1, slot->active);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_barrack_pool_init);
    RUN_TEST(test_barrack_pool_allocate_single);
    RUN_TEST(test_barrack_pool_allocate_multiple);
    RUN_TEST(test_barrack_pool_allocate_capacity_exceeded);
    RUN_TEST(test_barrack_pool_get_valid);
    RUN_TEST(test_barrack_pool_get_invalid_id);
    RUN_TEST(test_barrack_pool_get_inactive_slot);
    RUN_TEST(test_barrack_pool_sequential_allocation);

    return UNITY_END();
}