/**
 * @file test_pathfinding.c
 * @brief Unit tests for pathfinding compute_next_step
 */

#include "internal/patika_internal.h"
#include "patika.h"
#include "unity.h"

static PatikaHandle handle;

void setUp(void)
{
    PatikaConfig config = {.grid_type = MAP_TYPE_HEXAGONAL,
        .max_agents = 10,
        .max_barracks = 5,
        .grid_width = 5, // radius 5
        .grid_height = 5,
        .command_queue_size = 16,
        .event_queue_size = 16,
        .rng_seed = 12345};
        handle = patika_create(&config);
}

void tearDown(void)
{
    patika_destroy(handle);
}

// ============================================================================
// Basic Pathfinding Tests
// ============================================================================

void test_pathfinding_direct_neighbor(void)
{
    AgentID id;

    // Add agent at (0, 0)
    PatikaCommand add_cmd = {0};
    add_cmd.type = CMD_ADD_AGENT;
    add_cmd.add_agent.start_q = 0;
    add_cmd.add_agent.start_r = 0;
    add_cmd.add_agent.out_agent_id = &id;
    patika_submit_command(handle, &add_cmd);
    patika_tick(handle);

    // Set goal to adjacent cell (1, 0)
    PatikaCommand goal_cmd = {0};
    goal_cmd.type = CMD_SET_GOAL;
    goal_cmd.set_goal.agent_id = id;
    goal_cmd.set_goal.goal_q = 1;
    goal_cmd.set_goal.goal_r = 0;
    patika_submit_command(handle, &goal_cmd);

    // Tick: Computes AND Moves to (1,0)
    patika_tick(handle);

    // Verify agent reached target
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    AgentSnapshot *agent = &snap->agents[0];

    // Position should match goal
    TEST_ASSERT_EQUAL_INT32(1, agent->pos_q);
    TEST_ASSERT_EQUAL_INT32(0, agent->pos_r);

    // Since it reached goal, it should be IDLE
    TEST_ASSERT_EQUAL_UINT8(STATE_IDLE, agent->state);
}

void test_pathfinding_at_goal(void)
{
    AgentID id;

    // Add agent and set goal at same position
    PatikaCommand add_cmd = {0};
    add_cmd.type = CMD_ADD_AGENT;
    add_cmd.add_agent.start_q = 2;
    add_cmd.add_agent.start_r = 2;
    add_cmd.add_agent.out_agent_id = &id;
    patika_submit_command(handle, &add_cmd);
    patika_tick(handle);

    PatikaCommand goal_cmd = {0};
    goal_cmd.type = CMD_SET_GOAL;
    goal_cmd.set_goal.agent_id = id;
    goal_cmd.set_goal.goal_q = 2;
    goal_cmd.set_goal.goal_r = 2;
    patika_submit_command(handle, &goal_cmd);
    patika_tick(handle);

    // Agent should do nothing and stay IDLE
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    AgentSnapshot *agent = &snap->agents[0];

    TEST_ASSERT_EQUAL_UINT8(STATE_IDLE, agent->state);
    TEST_ASSERT_EQUAL_INT32(2, agent->pos_q);
}

void test_pathfinding_blocked_all_neighbors(void)
{
    AgentID id;

    // Add agent at (0, 0)
    PatikaCommand add_cmd = {0};
    add_cmd.type = CMD_ADD_AGENT;
    add_cmd.add_agent.start_q = 0;
    add_cmd.add_agent.start_r = 0;
    add_cmd.add_agent.out_agent_id = &id;
    patika_submit_command(handle, &add_cmd);
    patika_tick(handle);

    // Block all 6 neighbors
    int hex_dirs[6][2] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
    for (int i = 0; i < 6; i++)
    {
        PatikaCommand tile_cmd = {0};
        tile_cmd.type = CMD_SET_TILE_STATE;
        tile_cmd.set_tile.q = hex_dirs[i][0];
        tile_cmd.set_tile.r = hex_dirs[i][1];
        tile_cmd.set_tile.state = 1; // blocked
        patika_submit_command(handle, &tile_cmd);
    }
    patika_tick(handle);

    // Set goal
    PatikaCommand goal_cmd = {0};
    goal_cmd.type = CMD_SET_GOAL;
    goal_cmd.set_goal.agent_id = id;
    goal_cmd.set_goal.goal_q = 3;
    goal_cmd.set_goal.goal_r = 0;
    patika_submit_command(handle, &goal_cmd);
    patika_tick(handle);

    // Agent should be STUCK (which usually sets state to IDLE in implementation)
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    AgentSnapshot *agent = &snap->agents[0];

    // Implementation sets state to IDLE when stuck, but emits event
    TEST_ASSERT_EQUAL_UINT8(STATE_IDLE, agent->state);

    // Verify EVENT_STUCK was emitted
    PatikaEvent events[10];
    uint32_t event_count = patika_poll_events(handle, events, 10);
    TEST_ASSERT_EQUAL_UINT32(1, event_count);
    TEST_ASSERT_EQUAL(EVENT_STUCK, events[0].type);
    TEST_ASSERT_EQUAL(id, events[0].agent_id);
}

void test_pathfinding_partial_blockage(void)
{
    AgentID id;

    // Add agent at (0, 0)
    PatikaCommand add_cmd = {0};
    add_cmd.type = CMD_ADD_AGENT;
    add_cmd.add_agent.start_q = 0;
    add_cmd.add_agent.start_r = 0;
    add_cmd.add_agent.out_agent_id = &id;
    patika_submit_command(handle, &add_cmd);
    patika_tick(handle);

    // Block 5 out of 6 neighbors, leave (1, 0) open
    int blocked_dirs[5][2] = {{1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
    for (int i = 0; i < 5; i++)
    {
        PatikaCommand tile_cmd = {0};
        tile_cmd.type = CMD_SET_TILE_STATE;
        tile_cmd.set_tile.q = blocked_dirs[i][0];
        tile_cmd.set_tile.r = blocked_dirs[i][1];
        tile_cmd.set_tile.state = 1;
        patika_submit_command(handle, &tile_cmd);
    }
    patika_tick(handle);

    // Set goal to (2, 0) - only path is through (1, 0)
    PatikaCommand goal_cmd = {0};
    goal_cmd.type = CMD_SET_GOAL;
    goal_cmd.set_goal.agent_id = id;
    goal_cmd.set_goal.goal_q = 2;
    goal_cmd.set_goal.goal_r = 0;
    patika_submit_command(handle, &goal_cmd);
    patika_tick(handle);

    // Agent should have moved to (1, 0)
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    AgentSnapshot *agent = &snap->agents[0];

    TEST_ASSERT_EQUAL_INT32(1, agent->pos_q);
    TEST_ASSERT_EQUAL_INT32(0, agent->pos_r);
    // And is ready to calc next step
    TEST_ASSERT_EQUAL_UINT8(STATE_CALCULATING, agent->state);
}

void test_pathfinding_multiple_agents_same_goal(void)
{
    AgentID ids[3];

    // Add 3 agents at different positions
    for (int i = 0; i < 3; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_ADD_AGENT;
        cmd.add_agent.start_q = i * 2;
        cmd.add_agent.start_r = 0;
        cmd.add_agent.out_agent_id = &ids[i];
        patika_submit_command(handle, &cmd);
    }
    patika_tick(handle);

    // Set same goal for all
    for (int i = 0; i < 3; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_SET_GOAL;
        cmd.set_goal.agent_id = ids[i];
        cmd.set_goal.goal_q = 5;
        cmd.set_goal.goal_r = 5;
        patika_submit_command(handle, &cmd);
    }
    patika_tick(handle);

    // All should be active
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    for (uint32_t i = 0; i < snap->agent_count; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(STATE_CALCULATING, snap->agents[i].state);
    }
}

void test_pathfinding_greedy_selection(void)
{
    AgentID id;

    // Add agent at (0, 0), goal at (2, 0)
    PatikaCommand add_cmd = {0};
    add_cmd.type = CMD_ADD_AGENT;
    add_cmd.add_agent.start_q = 0;
    add_cmd.add_agent.start_r = 0;
    add_cmd.add_agent.out_agent_id = &id;
    patika_submit_command(handle, &add_cmd);
    patika_tick(handle);

    PatikaCommand goal_cmd = {0};
    goal_cmd.type = CMD_SET_GOAL;
    goal_cmd.set_goal.agent_id = id;
    goal_cmd.set_goal.goal_q = 2;
    goal_cmd.set_goal.goal_r = 0;
    patika_submit_command(handle, &goal_cmd);
    patika_tick(handle);

    // Greedy should pick (1, 0) as it's closest to goal, so it moves there
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    AgentSnapshot *agent = &snap->agents[0];
    TEST_ASSERT_EQUAL_INT32(1, agent->pos_q);
    TEST_ASSERT_EQUAL_INT32(0, agent->pos_r);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_pathfinding_direct_neighbor);
    RUN_TEST(test_pathfinding_at_goal);
    RUN_TEST(test_pathfinding_blocked_all_neighbors);
    RUN_TEST(test_pathfinding_partial_blockage);
    RUN_TEST(test_pathfinding_multiple_agents_same_goal);
    RUN_TEST(test_pathfinding_greedy_selection);

    return UNITY_END();
}
