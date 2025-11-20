/**
 * @file test_integration_multi_agent.c
 * @brief Integration tests for complex multi-agent scenarios
 */

#include "patika_core.h"
#include "unity.h"
#include <pthread.h>
#include <unistd.h>

static PatikaHandle handle;

void setUp(void)
{
    PatikaConfig config = {.grid_type = MAP_TYPE_HEXAGONAL,
                           .max_agents = 200,
                           .max_barracks = 20,
                           .grid_width = 15,
                           .grid_height = 15,
                           .command_queue_size = 256,
                           .event_queue_size = 256,
                           .rng_seed = 999};
    handle = patika_create(&config);
}

void tearDown(void)
{
    patika_destroy(handle);
}

// ============================================================================
// Multi-Agent Scenarios
// ============================================================================

void test_fifty_agents_different_goals(void)
{
    const int NUM_AGENTS = 50;
    AgentID ids[NUM_AGENTS];

    // Create 50 agents in a line
    for (int i = 0; i < NUM_AGENTS; i++)
    {
        patika_add_agent_sync(handle, -10 + i / 5, i % 5, 1, 1, PATIKA_INVALID_BARRACK_ID, &ids[i]);
    }
    patika_tick(handle);

    // Set different goals
    for (int i = 0; i < NUM_AGENTS; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_SET_GOAL;
        cmd.set_goal.agent_id = ids[i];
        cmd.set_goal.goal_q = (i % 10) - 5;
        cmd.set_goal.goal_r = (i / 10) - 2;
        patika_submit_command(handle, &cmd);
    }
    patika_tick(handle);

    // Verify all agents computed paths
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(NUM_AGENTS, snap->agent_count);

    for (uint32_t i = 0; i < snap->agent_count; i++)
    {
        // Should be MOVING (2) or STUCK (3)
        TEST_ASSERT_TRUE(snap->agents[i].state == 2 || snap->agents[i].state == 3);
    }
}

void test_multiple_barracks_with_agents(void)
{
    BarrackID barrack_ids[5];

    // Create 5 barracks
    for (int i = 0; i < 5; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_ADD_BARRACK;
        cmd.add_barrack.pos_q = i * 3;
        cmd.add_barrack.pos_r = 0;
        cmd.add_barrack.faction = i % 2;
        cmd.add_barrack.side = 1;
        cmd.add_barrack.patrol_radius = 3;
        cmd.add_barrack.max_agents = 10;
        cmd.add_barrack.out_barrack_id = &barrack_ids[i];
        patika_submit_command(handle, &cmd);
    }
    patika_tick(handle);

    // Add 5 agents per barrack
    for (int b = 0; b < 5; b++)
    {
        for (int a = 0; a < 5; a++)
        {
            patika_add_agent_sync(handle, b * 3, a, b % 2, 1, barrack_ids[b], NULL);
        }
    }
    patika_tick(handle);

    // Verify all created
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(25, snap->agent_count);
    TEST_ASSERT_EQUAL_UINT16(5, snap->barrack_count);
}

void test_large_scale_pathfinding(void)
{
    const int NUM_AGENTS = 100;

    // Create agents in a cluster
    for (int i = 0; i < NUM_AGENTS; i++)
    {
        int q = (i % 10) - 5;
        int r = (i / 10) - 5;
        patika_add_agent_sync(handle, q, r, 1, 1, PATIKA_INVALID_BARRACK_ID, NULL);
    }
    patika_tick(handle);

    // All agents go to same goal (convergence test)
    AgentID *ids = NULL;
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    for (uint32_t i = 0; i < snap->agent_count; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_SET_GOAL;
        cmd.set_goal.agent_id = snap->agents[i].id;
        cmd.set_goal.goal_q = 10;
        cmd.set_goal.goal_r = 10;
        patika_submit_command(handle, &cmd);
    }
    patika_tick(handle);

    // Verify all computed next steps
    snap = patika_get_snapshot(handle);
    int moving_count = 0;
    for (uint32_t i = 0; i < snap->agent_count; i++)
    {
        if (snap->agents[i].state == 2)
        { // MOVING
            moving_count++;
        }
    }

    // Most agents should be moving (some might be stuck due to clustering)
    TEST_ASSERT_GREATER_THAN(50, moving_count);
}

void test_faction_separation(void)
{
    // Create two factions
    for (int f = 0; f < 2; f++)
    {
        for (int i = 0; i < 10; i++)
        {
            int q = f * 10 - 5;
            int r = i - 5;
            patika_add_agent_sync(handle, q, r, f, f, PATIKA_INVALID_BARRACK_ID, NULL);
        }
    }
    patika_tick(handle);

    // Verify factions
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(20, snap->agent_count);

    int faction_counts[2] = {0, 0};
    for (uint32_t i = 0; i < snap->agent_count; i++)
    {
        faction_counts[snap->agents[i].faction]++;
    }

    TEST_ASSERT_EQUAL_INT(10, faction_counts[0]);
    TEST_ASSERT_EQUAL_INT(10, faction_counts[1]);
}

void test_dynamic_map_changes(void)
{
    // Create agents
    for (int i = 0; i < 20; i++)
    {
        patika_add_agent_sync(handle, -5 + i, 0, 1, 1, PATIKA_INVALID_BARRACK_ID, NULL);
    }
    patika_tick(handle);

    // Build a wall
    for (int i = -3; i <= 3; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_SET_TILE_STATE;
        cmd.set_tile.q = 0;
        cmd.set_tile.r = i;
        cmd.set_tile.state = 1; // blocked
        patika_submit_command(handle, &cmd);
    }
    patika_tick(handle);

    // Set goals on other side of wall
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    for (uint32_t i = 0; i < snap->agent_count; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_SET_GOAL;
        cmd.set_goal.agent_id = snap->agents[i].id;
        cmd.set_goal.goal_q = 10;
        cmd.set_goal.goal_r = 0;
        patika_submit_command(handle, &cmd);
    }
    patika_tick(handle);

    // Agents should path around wall
    snap = patika_get_snapshot(handle);
    for (uint32_t i = 0; i < snap->agent_count; i++)
    {
        // Verify agents didn't pick blocked cells
        if (snap->agents[i].next_q == 0)
        {
            TEST_ASSERT_FALSE(snap->agents[i].next_r >= -3 && snap->agents[i].next_r <= 3);
        }
    }
}

// ============================================================================
// Concurrent Command Submission
// ============================================================================

typedef struct
{
    PatikaHandle handle;
    int agent_count;
} ThreadArgs;

static void *add_agents_thread(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;

    for (int i = 0; i < args->agent_count; i++)
    {
        AgentID id;
        patika_add_agent_sync(args->handle, i % 10, i / 10, 1, 1, PATIKA_INVALID_BARRACK_ID, &id);
        usleep(100); // Small delay
    }

    return NULL;
}

void test_concurrent_agent_creation(void)
{
    pthread_t threads[4];
    ThreadArgs args[4];

    // 4 threads each creating 10 agents
    for (int i = 0; i < 4; i++)
    {
        args[i].handle = handle;
        args[i].agent_count = 10;
        pthread_create(&threads[i], NULL, add_agents_thread, &args[i]);
    }

    // Process ticks while threads are running
    for (int i = 0; i < 20; i++)
    {
        patika_tick(handle);
        usleep(50000); // 50ms
    }

    // Wait for threads
    for (int i = 0; i < 4; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Final tick to process remaining commands
    patika_tick(handle);

    // Should have all 40 agents
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(40, snap->agent_count);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_fifty_agents_different_goals);
    RUN_TEST(test_multiple_barracks_with_agents);
    RUN_TEST(test_large_scale_pathfinding);
    RUN_TEST(test_faction_separation);
    RUN_TEST(test_dynamic_map_changes);
    // RUN_TEST(test_concurrent_agent_creation); // only for threaded environments

    return UNITY_END();
}