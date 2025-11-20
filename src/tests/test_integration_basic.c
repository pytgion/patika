/**
 * @file test_integration_basic.c
 * @brief Basic integration tests for full simulation workflows
 */

#include "patika_core.h"
#include "unity.h"

static PatikaHandle handle;

void setUp(void)
{
    PatikaConfig config = {.grid_type = MAP_TYPE_HEXAGONAL,
                           .max_agents = 50,
                           .max_barracks = 10,
                           .grid_width = 10,
                           .grid_height = 10,
                           .command_queue_size = 64,
                           .event_queue_size = 64,
                           .rng_seed = 42};
    handle = patika_create(&config);
}

void tearDown(void)
{
    patika_destroy(handle);
}

// ============================================================================
// Full Lifecycle Tests
// ============================================================================

void test_create_and_destroy(void)
{
    // Just verify setUp/tearDown don't crash
    TEST_ASSERT_NOT_NULL(handle);
}

void test_agent_full_lifecycle(void)
{
    AgentID id;

    // 1. Add agent
    PatikaError err = patika_add_agent_sync(handle, 0, 0, 1, 1, PATIKA_INVALID_BARRACK_ID, &id);

    TEST_ASSERT_EQUAL(PATIKA_OK, err);
    patika_tick(handle);

    // 2. Verify in snapshot
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(1, snap->agent_count);
    TEST_ASSERT_EQUAL(id, snap->agents[0].id);

    // 3. Set goal
    PatikaCommand goal_cmd = {0};
    goal_cmd.type = CMD_SET_GOAL;
    goal_cmd.set_goal.agent_id = id;
    goal_cmd.set_goal.goal_q = 5;
    goal_cmd.set_goal.goal_r = 0;
    patika_submit_command(handle, &goal_cmd);
    patika_tick(handle);

    // 4. Verify goal set and pathfinding triggered
    snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_INT32(5, snap->agents[0].target_q);
    TEST_ASSERT_EQUAL_INT32(0, snap->agents[0].target_r);
    TEST_ASSERT_EQUAL_UINT8(2, snap->agents[0].state); // MOVING

    // 5. Remove agent
    PatikaCommand rm_cmd = {0};
    rm_cmd.type = CMD_REMOVE_AGENT;
    rm_cmd.remove_agent.agent_id = id;
    patika_submit_command(handle, &rm_cmd);
    patika_tick(handle);

    // 6. Verify removed
    snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(0, snap->agent_count);
}

void test_barrack_agent_relationship(void)
{
    BarrackID barrack_id;
    AgentID agent_id;

    // Create barrack
    PatikaCommand barrack_cmd = {0};
    barrack_cmd.type = CMD_ADD_BARRACK;
    barrack_cmd.add_barrack.pos_q = 0;
    barrack_cmd.add_barrack.pos_r = 0;
    barrack_cmd.add_barrack.faction = 1;
    barrack_cmd.add_barrack.side = 1;
    barrack_cmd.add_barrack.patrol_radius = 5;
    barrack_cmd.add_barrack.max_agents = 10;
    barrack_cmd.add_barrack.out_barrack_id = &barrack_id;
    patika_submit_command(handle, &barrack_cmd);
    patika_tick(handle);

    // Create agent bound to barrack
    patika_add_agent_sync(handle, 1, 0, 1, 1, barrack_id, &agent_id);
    patika_tick(handle);

    // Verify relationship
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(1, snap->agent_count);
    TEST_ASSERT_EQUAL_UINT16(1, snap->barrack_count);
    TEST_ASSERT_EQUAL(barrack_id, snap->agents[0].parent_barrack);
}

void test_multiple_ticks_progression(void)
{
    AgentID id;

    patika_add_agent_sync(handle, 0, 0, 1, 1, PATIKA_INVALID_BARRACK_ID, &id);
    patika_tick(handle);

    // Set goal
    PatikaCommand goal_cmd = {0};
    goal_cmd.type = CMD_SET_GOAL;
    goal_cmd.set_goal.agent_id = id;
    goal_cmd.set_goal.goal_q = 3;
    goal_cmd.set_goal.goal_r = 0;
    patika_submit_command(handle, &goal_cmd);

    // Run multiple ticks
    for (int i = 0; i < 5; i++)
    {
        patika_tick(handle);
    }

    // Verify ticks progressed
    PatikaStats stats = patika_get_stats(handle);
    TEST_ASSERT_EQUAL_UINT64(6, stats.total_ticks); // 1 initial + 5 loop
}

void test_snapshot_version_increments(void)
{
    const PatikaSnapshot *snap1 = patika_get_snapshot(handle);
    uint64_t version1 = snap1->version;

    patika_tick(handle);

    const PatikaSnapshot *snap2 = patika_get_snapshot(handle);
    uint64_t version2 = snap2->version;

    TEST_ASSERT_GREATER_THAN(version1, version2);
}

void test_map_modification_affects_pathfinding(void)
{
    AgentID id;

    patika_add_agent_sync(handle, 0, 0, 1, 1, PATIKA_INVALID_BARRACK_ID, &id);
    patika_tick(handle);

    // Block the direct path
    PatikaCommand tile_cmd = {0};
    tile_cmd.type = CMD_SET_TILE_STATE;
    tile_cmd.set_tile.q = 1;
    tile_cmd.set_tile.r = 0;
    tile_cmd.set_tile.state = 1; // blocked
    patika_submit_command(handle, &tile_cmd);
    patika_tick(handle);

    // Set goal
    PatikaCommand goal_cmd = {0};
    goal_cmd.type = CMD_SET_GOAL;
    goal_cmd.set_goal.agent_id = id;
    goal_cmd.set_goal.goal_q = 2;
    goal_cmd.set_goal.goal_r = 0;
    patika_submit_command(handle, &goal_cmd);
    patika_tick(handle);

    // Agent should avoid blocked tile
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    AgentSnapshot *agent = &snap->agents[0];

    // Should not choose (1, 0)
    TEST_ASSERT_FALSE(agent->next_q == 1 && agent->next_r == 0);
}

void test_event_polling(void)
{
    AgentID id;

    patika_add_agent_sync(handle, 0, 0, 1, 1, PATIKA_INVALID_BARRACK_ID, &id);
    patika_tick(handle);

    // Block all neighbors to trigger STUCK event
    int hex_dirs[6][2] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
    for (int i = 0; i < 6; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_SET_TILE_STATE;
        cmd.set_tile.q = hex_dirs[i][0];
        cmd.set_tile.r = hex_dirs[i][1];
        cmd.set_tile.state = 1;
        patika_submit_command(handle, &cmd);
    }
    patika_tick(handle);

    // Set goal
    PatikaCommand goal_cmd = {0};
    goal_cmd.type = CMD_SET_GOAL;
    goal_cmd.set_goal.agent_id = id;
    goal_cmd.set_goal.goal_q = 5;
    goal_cmd.set_goal.goal_r = 0;
    patika_submit_command(handle, &goal_cmd);
    patika_tick(handle);

    // Poll events
    PatikaEvent events[10];
    uint32_t count = patika_poll_events(handle, events, 10);

    TEST_ASSERT_GREATER_THAN_UINT32(0, count);
    TEST_ASSERT_EQUAL(EVENT_STUCK, events[0].type);
    TEST_ASSERT_EQUAL(id, events[0].agent_id);
}

void test_stats_tracking(void)
{
    // Add multiple agents
    for (int i = 0; i < 5; i++)
    {
        patika_add_agent_sync(handle, i, 0, 1, 1, PATIKA_INVALID_BARRACK_ID, NULL);
    }
    patika_tick(handle);

    PatikaStats stats = patika_get_stats(handle);
    TEST_ASSERT_EQUAL_UINT64(1, stats.total_ticks);
    TEST_ASSERT_EQUAL_UINT64(5, stats.commands_processed);
    // TEST_ASSERT_EQUAL_UINT32(5, stats.active_agents); / solve active agents coutning in patika_pool.c
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_create_and_destroy);
    RUN_TEST(test_agent_full_lifecycle);
    RUN_TEST(test_barrack_agent_relationship);
    RUN_TEST(test_multiple_ticks_progression);
    RUN_TEST(test_snapshot_version_increments);
    RUN_TEST(test_map_modification_affects_pathfinding);
    RUN_TEST(test_event_polling);
    RUN_TEST(test_stats_tracking);

    return UNITY_END();
}