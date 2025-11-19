/**
 * @file test_commands.c
 * @brief Unit tests for command processing
 */

#include "internal/patika_internal.h"
#include "patika_core.h"
#include "unity.h"

static PatikaHandle handle;

void setUp(void)
{
    PatikaConfig config = {.grid_type = MAP_TYPE_HEXAGONAL,
                           .max_agents = 100,
                           .max_barracks = 10,
                           .grid_width = 10,
                           .grid_height = 10,
                           .sector_size = 0,
                           .command_queue_size = 32,
                           .event_queue_size = 32,
                           .rng_seed = 42};
    handle = patika_create(&config);
}

void tearDown(void)
{
    patika_destroy(handle);
}

// ============================================================================
// CMD_ADD_AGENT Tests
// ============================================================================

void test_cmd_add_agent_basic(void)
{
    AgentID id = PATIKA_INVALID_AGENT_ID;

    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_AGENT;
    cmd.add_agent.start_q = 0;
    cmd.add_agent.start_r = 0;
    cmd.add_agent.faction = 1;
    cmd.add_agent.side = 2;
    cmd.add_agent.parent_barrack = PATIKA_INVALID_BARRACK_ID;
    cmd.add_agent.out_agent_id = &id;

    PatikaError err = patika_submit_command(handle, &cmd);
    TEST_ASSERT_EQUAL(PATIKA_OK, err);

    patika_tick(handle);

    TEST_ASSERT_NOT_EQUAL(PATIKA_INVALID_AGENT_ID, id);
}

void test_cmd_add_agent_multiple(void)
{
    AgentID ids[5];

    for (int i = 0; i < 5; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_ADD_AGENT;
        cmd.add_agent.start_q = i;
        cmd.add_agent.start_r = 0;
        cmd.add_agent.faction = 1;
        cmd.add_agent.side = 1;
        cmd.add_agent.out_agent_id = &ids[i];

        patika_submit_command(handle, &cmd);
    }

    patika_tick(handle);

    // All IDs should be valid and unique
    for (int i = 0; i < 5; i++)
    {
        TEST_ASSERT_NOT_EQUAL(PATIKA_INVALID_AGENT_ID, ids[i]);
        for (int j = i + 1; j < 5; j++)
        {
            TEST_ASSERT_NOT_EQUAL(ids[i], ids[j]);
        }
    }
}

void test_cmd_add_agent_verify_snapshot(void)
{
    AgentID id;

    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_AGENT;
    cmd.add_agent.start_q = 3;
    cmd.add_agent.start_r = -2;
    cmd.add_agent.faction = 5;
    cmd.add_agent.side = 7;
    cmd.add_agent.out_agent_id = &id;

    patika_submit_command(handle, &cmd);
    patika_tick(handle);

    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_NOT_NULL(snap);
    TEST_ASSERT_EQUAL_UINT32(1, snap->agent_count);

    AgentSnapshot *agent = &snap->agents[0];
    TEST_ASSERT_EQUAL(id, agent->id);
    TEST_ASSERT_EQUAL_INT32(3, agent->pos_q);
    TEST_ASSERT_EQUAL_INT32(-2, agent->pos_r);
    TEST_ASSERT_EQUAL_UINT8(5, agent->faction);
    TEST_ASSERT_EQUAL_UINT8(7, agent->side);
}

// ============================================================================
// CMD_REMOVE_AGENT Tests
// ============================================================================

void test_cmd_remove_agent(void)
{
    AgentID id;

    // Add agent
    PatikaCommand add_cmd = {0};
    add_cmd.type = CMD_ADD_AGENT;
    add_cmd.add_agent.out_agent_id = &id;
    patika_submit_command(handle, &add_cmd);
    patika_tick(handle);

    // Remove agent
    PatikaCommand rm_cmd = {0};
    rm_cmd.type = CMD_REMOVE_AGENT;
    rm_cmd.remove_agent.agent_id = id;
    patika_submit_command(handle, &rm_cmd);
    patika_tick(handle);

    // Verify removed from snapshot
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT32(0, snap->agent_count);
}

void test_cmd_remove_invalid_agent(void)
{
    PatikaCommand cmd = {0};
    cmd.type = CMD_REMOVE_AGENT;
    cmd.remove_agent.agent_id = 99999;

    // Should not crash
    patika_submit_command(handle, &cmd);
    patika_tick(handle);
    TEST_PASS();
}

// ============================================================================
// CMD_SET_GOAL Tests
// ============================================================================

void test_cmd_set_goal(void)
{
    AgentID id;

    // Add agent
    PatikaCommand add_cmd = {0};
    add_cmd.type = CMD_ADD_AGENT;
    add_cmd.add_agent.start_q = 0;
    add_cmd.add_agent.start_r = 0;
    add_cmd.add_agent.out_agent_id = &id;
    patika_submit_command(handle, &add_cmd);
    patika_tick(handle);

    // Set goal
    PatikaCommand goal_cmd = {0};
    goal_cmd.type = CMD_SET_GOAL;
    goal_cmd.set_goal.agent_id = id;
    goal_cmd.set_goal.goal_q = 5;
    goal_cmd.set_goal.goal_r = 5;
    patika_submit_command(handle, &goal_cmd);
    patika_tick(handle);

    // Verify goal set
    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    AgentSnapshot *agent = &snap->agents[1];
    TEST_ASSERT_EQUAL_INT32(5, agent->target_q);
    TEST_ASSERT_EQUAL_INT32(5, agent->target_r);
    TEST_ASSERT_EQUAL_UINT8(2, agent->state); // it returns 2 because tick processes pathfind and change state to MOVING
}

// ============================================================================
// CMD_ADD_BARRACK Tests
// ============================================================================

void test_cmd_add_barrack(void)
{
    BarrackID id;

    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_BARRACK;
    cmd.add_barrack.pos_q = 2;
    cmd.add_barrack.pos_r = 3;
    cmd.add_barrack.faction = 1;
    cmd.add_barrack.side = 1;
    cmd.add_barrack.patrol_radius = 5;
    cmd.add_barrack.max_agents = 20;
    cmd.add_barrack.out_barrack_id = &id;

    patika_submit_command(handle, &cmd);
    patika_tick(handle);

    TEST_ASSERT_NOT_EQUAL(PATIKA_INVALID_BARRACK_ID, id);

    const PatikaSnapshot *snap = patika_get_snapshot(handle);
    TEST_ASSERT_EQUAL_UINT16(1, snap->barrack_count);

    BarrackSnapshot *barrack = &snap->barracks[0];
    TEST_ASSERT_EQUAL_INT32(2, barrack->pos_q);
    TEST_ASSERT_EQUAL_INT32(3, barrack->pos_r);
    TEST_ASSERT_EQUAL_UINT8(1, barrack->faction);
    TEST_ASSERT_EQUAL_UINT8(5, barrack->patrol_radius);
}

// ============================================================================
// CMD_SET_TILE_STATE Tests
// ============================================================================

void test_cmd_set_tile_state(void)
{
    PatikaCommand cmd = {0};
    cmd.type = CMD_SET_TILE_STATE;
    cmd.set_tile.q = 1;
    cmd.set_tile.r = 1;
    cmd.set_tile.state = 1; // closed

    patika_submit_command(handle, &cmd);
    patika_tick(handle);

    // Verify tile state changed
    MapTile *tile = map_get(&handle->map, 1, 1);
    TEST_ASSERT_NOT_NULL(tile);
    TEST_ASSERT_EQUAL_UINT8(1, tile->state);
}

// ============================================================================
// Statistics Tests
// ============================================================================

void test_commands_increment_stats(void)
{
    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_AGENT;

    for (int i = 0; i < 10; i++)
    {
        patika_submit_command(handle, &cmd);
    }

    patika_tick(handle);

    PatikaStats stats = patika_get_stats(handle);
    TEST_ASSERT_EQUAL_UINT64(10, stats.commands_processed);
    TEST_ASSERT_EQUAL_UINT64(1, stats.total_ticks);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_cmd_add_agent_basic);
    RUN_TEST(test_cmd_add_agent_multiple);
    RUN_TEST(test_cmd_add_agent_verify_snapshot);
    RUN_TEST(test_cmd_remove_agent);
    RUN_TEST(test_cmd_remove_invalid_agent);
    RUN_TEST(test_cmd_set_goal);
    RUN_TEST(test_cmd_add_barrack);
    RUN_TEST(test_cmd_set_tile_state);
    RUN_TEST(test_commands_increment_stats);

    return UNITY_END();
}
