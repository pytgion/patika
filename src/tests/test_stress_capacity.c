/**
 * @file test_stress_capacity.c
 * @brief Stress tests for capacity limits and memory management
 */

#include "patika_core.h"
#include "unity.h"
#include <stdlib.h>

static PatikaHandle handle;

void setUp(void)
{
    PatikaConfig config = {.grid_type = MAP_TYPE_HEXAGONAL,
                           .max_agents = 1000,
                           .max_barracks = 100,
                           .grid_width = 25,
                           .grid_height = 25,
                           .command_queue_size = 512,
                           .event_queue_size = 512,
                           .rng_seed = 987654321};
    handle = patika_create(&config);
}

void tearDown(void)
{
    patika_destroy(handle);
}

// ============================================================================
// Agent Capacity Tests
// ============================================================================

void test_max_agent_capacity(void)
{
    int created = 0;

    // Try to create max agents
    for (int i = 0; i < 1000; i++)
    {
        AgentID id;
        PatikaError err = patika_add_agent_sync(handle, i % 20, i / 20, 1, 1, PATIKA_INVALID_BARRACK_ID, &id);
        patika_tick(handle);

        if (err == PATIKA_OK && id != PATIKA_INVALID_AGENT_ID)
        {
            created++;
        }
    }

    TEST_ASSERT_EQUAL_INT(1000, created);

    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(1000, snap->agent_count);
}

void test_agent_creation_after_removal(void)
{
    AgentID ids[100];

    // Create 100 agents
    for (int i = 0; i < 100; i++)
    {
        patika_add_agent_sync(handle, i % 10, i / 10, 1, 1, PATIKA_INVALID_BARRACK_ID, &ids[i]);
    }
    patika_tick(handle);

    // Remove 50 agents
    for (int i = 0; i < 50; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_REMOVE_AGENT;
        cmd.remove_agent.agent_id = ids[i];
        patika_submit_command(handle, &cmd);
    }
    patika_tick(handle);

    // Create 50 more (should reuse freed slots)
    for (int i = 0; i < 50; i++)
    {
        AgentID id;
        patika_add_agent_sync(handle, 0, 0, 1, 1, PATIKA_INVALID_BARRACK_ID, &id);
    }
    patika_tick(handle);

    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(100, snap->agent_count);
}

void test_rapid_create_destroy_cycle(void)
{
    for (int cycle = 0; cycle < 50; cycle++)
    {
        AgentID ids[20];

        // Create
        for (int i = 0; i < 20; i++)
        {
            patika_add_agent_sync(handle, i, 0, 1, 1, PATIKA_INVALID_BARRACK_ID, &ids[i]);
        }
        patika_tick(handle);

        // Destroy
        for (int i = 0; i < 20; i++)
        {
            PatikaCommand cmd = {0};
            cmd.type = CMD_REMOVE_AGENT;
            cmd.remove_agent.agent_id = ids[i];
            patika_submit_command(handle, &cmd);
        }
        patika_tick(handle);
    }

    // Should have no memory leaks and clean state
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(0, snap->agent_count);
}

void test_generation_overflow_resilience(void)
{
    // Create and destroy same slot many times to increment generation
    for (int i = 0; i < 100; i++)
    {
        AgentID id;
        patika_add_agent_sync(handle, 0, 0, 1, 1, PATIKA_INVALID_BARRACK_ID, &id);
        patika_tick(handle);

        PatikaCommand cmd = {0};
        cmd.type = CMD_REMOVE_AGENT;
        cmd.remove_agent.agent_id = id;
        patika_submit_command(handle, &cmd);
        patika_tick(handle);
    }

    // Should still work
    AgentID final_id;
    patika_add_agent_sync(handle, 0, 0, 1, 1, PATIKA_INVALID_BARRACK_ID, &final_id);
    patika_tick(handle);

    TEST_ASSERT_NOT_EQUAL(PATIKA_INVALID_AGENT_ID, final_id);
}

// ============================================================================
// Barrack Capacity Tests
// ============================================================================

void test_max_barrack_capacity(void)
{
    BarrackID ids[100];
    int created = 0;

    for (int i = 0; i < 100; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_ADD_BARRACK;
        cmd.add_barrack.pos_q = i % 10;
        cmd.add_barrack.pos_r = i / 10;
        cmd.add_barrack.faction = 1;
        cmd.add_barrack.side = 1;
        cmd.add_barrack.patrol_radius = 3;
        cmd.add_barrack.max_agents = 10;
        cmd.add_barrack.out_barrack_id = &ids[i];

        patika_submit_command(handle, &cmd);
        patika_tick(handle);

        if (ids[i] != PATIKA_INVALID_BARRACK_ID)
        {
            created++;
        }
    }

    TEST_ASSERT_EQUAL_INT(100, created);
}

void test_agents_per_barrack_limit(void)
{
    BarrackID barrack_id;

    // Create barrack with capacity 20
    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_BARRACK;
    cmd.add_barrack.pos_q = 0;
    cmd.add_barrack.pos_r = 0;
    cmd.add_barrack.faction = 1;
    cmd.add_barrack.side = 1;
    cmd.add_barrack.patrol_radius = 5;
    cmd.add_barrack.max_agents = 20;
    cmd.add_barrack.out_barrack_id = &barrack_id;
    patika_submit_command(handle, &cmd);
    patika_tick(handle);

    // Create 20 agents bound to barrack
    for (int i = 0; i < 20; i++)
    {
        patika_add_agent_sync(handle, i % 5, i / 5, 1, 1, barrack_id, NULL);
    }
    patika_tick(handle);

    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(20, snap->agent_count);
}

// ============================================================================
// Large Map Tests
// ============================================================================

void test_large_map_coverage(void)
{
    // Spread agents across entire map
    for (int q = -20; q <= 20; q++)
    {
        for (int r = -20; r <= 20; r++)
        {
            if (abs(q + r) <= 20)
            { // Hex constraint
                patika_add_agent_sync(handle, q, r, 1, 1, PATIKA_INVALID_BARRACK_ID, NULL);
            }
        }
    }
    patika_tick(handle);

    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_GREATER_THAN_UINT32(500, snap->agent_count);
}

void test_pathfinding_on_populated_map(void)
{
    // Create 500 agents
    for (int i = 0; i < 500; i++)
    {
        int q = (i % 20) - 10;
        int r = (i / 20) - 12;
        patika_add_agent_sync(handle, q, r, 1, 1, PATIKA_INVALID_BARRACK_ID, NULL);
    }
    patika_tick(handle);

    // Set goals for all
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    for (uint32_t i = 0; i < snap->agent_count; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_SET_GOAL;
        cmd.set_goal.agent_id = snap->agents[i].id;
        cmd.set_goal.goal_q = 15;
        cmd.set_goal.goal_r = 15;
        patika_submit_command(handle, &cmd);
    }
    patika_tick(handle);

    // Most should have computed paths
    snap = patika_get_snapshot(handle);
    int computed = 0;
    for (uint32_t i = 0; i < snap->agent_count; i++)
    {
        if (snap->agents[i].state >= 2)
        { // MOVING or STUCK
            computed++;
        }
    }

    TEST_ASSERT_GREATER_THAN(400, computed);
}

// ============================================================================
// Memory Pressure Tests
// ============================================================================

void test_snapshot_consistency_under_load(void)
{
    // Create many agents
    for (int i = 0; i < 300; i++)
    {
        patika_add_agent_sync(handle, i % 15, i / 15, 1, 1, PATIKA_INVALID_BARRACK_ID, NULL);
    }
    patika_tick(handle);

    // Get snapshot multiple times - should be consistent
    const PatikaSnapshot *snap1 = patika_get_snapshot(handle);
    const PatikaSnapshot *snap2 = patika_get_snapshot(handle);

    TEST_ASSERT_TRUE(snap1 == snap2); // Same snapshot
    TEST_ASSERT_EQUAL_UINT32(300, snap1->agent_count);

    patika_tick(handle);

    // After tick, new snapshot
    const PatikaSnapshot *snap3 = patika_get_snapshot(handle);
    TEST_ASSERT_TRUE(snap1 != snap3); // Different buffer
    TEST_ASSERT_GREATER_THAN(snap1->version, snap3->version);
}

void test_sustained_maximum_load(void)
{
    // Run at max capacity for many ticks
    for (int i = 0; i < 1000; i++)
    {
        patika_add_agent_sync(handle, i % 20, i / 20, 1, 1, PATIKA_INVALID_BARRACK_ID, NULL);
    }

    for (int tick = 0; tick < 100; tick++)
    {
        patika_tick(handle);
    }

    PatikaStats stats = patika_get_stats(handle);
    TEST_ASSERT_EQUAL_UINT64(100, stats.total_ticks);
    TEST_ASSERT_EQUAL_UINT32(1000, stats.active_agents);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_max_agent_capacity);
    RUN_TEST(test_agent_creation_after_removal);
    RUN_TEST(test_rapid_create_destroy_cycle);
    RUN_TEST(test_generation_overflow_resilience);
    RUN_TEST(test_max_barrack_capacity);
    RUN_TEST(test_agents_per_barrack_limit);
    RUN_TEST(test_large_map_coverage);
    RUN_TEST(test_pathfinding_on_populated_map);
    RUN_TEST(test_snapshot_consistency_under_load);
    RUN_TEST(test_sustained_maximum_load);

    return UNITY_END();
}
