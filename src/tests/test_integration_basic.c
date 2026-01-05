#include "unity.h"
#include "patika.h"
#include <stdlib.h>
#include <string.h>

static PatikaHandle ctx = NULL;

void setUp(void) {
    PatikaConfig config = {
        .grid_type = MAP_TYPE_HEXAGONAL,
        .max_agents = 100,
        .max_barracks = 10,
        .grid_width = 20,
        .grid_height = 20,
        .sector_size = 0,
        .command_queue_size = 256,
        .event_queue_size = 256,
        .rng_seed = 12345
    };
    ctx = patika_create(&config);
    TEST_ASSERT_NOT_NULL(ctx);
}

void tearDown(void) {
    if (ctx) {
        patika_destroy(ctx);
        ctx = NULL;
    }
}

void test_context_creation(void) {
    TEST_ASSERT_NOT_NULL(ctx);
}

void test_add_agent(void) {
    AddAgentPayload *payload = malloc(sizeof(AddAgentPayload));
    payload->start_q = 0;
    payload->start_r = 0;
    payload->faction = 0;
    payload->side = 0;
    payload->parent_barrack = PATIKA_INVALID_BARRACK_ID;
    payload->out_agent_id = NULL;
    payload->collision_data.layer = 0;
    payload->collision_data.collision_mask = 0;
    payload->collision_data.aggression_mask = 0;
    
    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_AGENT;
    cmd.large_command.payload = payload;
    
    PatikaError err = patika_submit_command(ctx, &cmd);
    TEST_ASSERT_EQUAL(PATIKA_OK, err);
    
    patika_tick(ctx);
    
    const PatikaSnapshot *snap = patika_get_snapshot(ctx);
    TEST_ASSERT_NOT_NULL(snap);
    TEST_ASSERT_EQUAL(1, snap->agent_count);
}

void test_add_multiple_agents(void) {
    for (int i = 0; i < 5; i++) {
        AddAgentPayload *payload = malloc(sizeof(AddAgentPayload));
        payload->start_q = i;
        payload->start_r = i;
        payload->faction = 0;
        payload->side = 0;
        payload->parent_barrack = PATIKA_INVALID_BARRACK_ID;
        payload->out_agent_id = NULL;
        payload->collision_data.layer = 0;
        payload->collision_data.collision_mask = 0;
        payload->collision_data.aggression_mask = 0;
        
        PatikaCommand cmd = {0};
        cmd.type = CMD_ADD_AGENT;
        cmd.large_command.payload = payload;
        
        PatikaError err = patika_submit_command(ctx, &cmd);
        TEST_ASSERT_EQUAL(PATIKA_OK, err);
    }
    
    patika_tick(ctx);
    
    const PatikaSnapshot *snap = patika_get_snapshot(ctx);
    TEST_ASSERT_EQUAL(5, snap->agent_count);
}

void test_remove_agent(void) {
    AddAgentPayload *payload = malloc(sizeof(AddAgentPayload));
    payload->start_q = 0;
    payload->start_r = 0;
    payload->faction = 0;
    payload->side = 0;
    payload->parent_barrack = PATIKA_INVALID_BARRACK_ID;
    payload->out_agent_id = NULL;
    payload->collision_data.layer = 0;
    payload->collision_data.collision_mask = 0;
    payload->collision_data.aggression_mask = 0;
    
    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_AGENT;
    cmd.large_command.payload = payload;
    patika_submit_command(ctx, &cmd);
    
    patika_tick(ctx);
    
    const PatikaSnapshot *snap = patika_get_snapshot(ctx);
    TEST_ASSERT_EQUAL(1, snap->agent_count);
    AgentID agent_id = snap->agents[0].id;
    
    PatikaCommand remove_cmd = {0};
    remove_cmd.type = CMD_REMOVE_AGENT;
    remove_cmd.remove_agent.agent_id = agent_id;
    
    PatikaError err = patika_submit_command(ctx, &remove_cmd);
    TEST_ASSERT_EQUAL(PATIKA_OK, err);
    
    patika_tick(ctx);
    
    snap = patika_get_snapshot(ctx);
    TEST_ASSERT_EQUAL(0, snap->agent_count);
}

void test_set_goal(void) {
    AddAgentPayload *payload = malloc(sizeof(AddAgentPayload));
    payload->start_q = 0;
    payload->start_r = 0;
    payload->faction = 0;
    payload->side = 0;
    payload->parent_barrack = PATIKA_INVALID_BARRACK_ID;
    payload->out_agent_id = NULL;
    payload->collision_data.layer = 0;
    payload->collision_data.collision_mask = 0;
    payload->collision_data.aggression_mask = 0;
    
    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_AGENT;
    cmd.large_command.payload = payload;
    patika_submit_command(ctx, &cmd);
    
    patika_tick(ctx);
    
    const PatikaSnapshot *snap = patika_get_snapshot(ctx);
    AgentID agent_id = snap->agents[0].id;
    
    PatikaCommand goal_cmd = {0};
    goal_cmd.type = CMD_SET_GOAL;
    goal_cmd.set_goal.agent_id = agent_id;
    goal_cmd.set_goal.goal_q = 5;
    goal_cmd.set_goal.goal_r = 5;
    
    PatikaError err = patika_submit_command(ctx, &goal_cmd);
    TEST_ASSERT_EQUAL(PATIKA_OK, err);
    
    patika_tick(ctx);
    
    snap = patika_get_snapshot(ctx);
    TEST_ASSERT_EQUAL(5, snap->agents[0].target_q);
    TEST_ASSERT_EQUAL(5, snap->agents[0].target_r);
}

void test_set_tile_state(void) {
    PatikaCommand cmd = {0};
    cmd.type = CMD_SET_TILE_STATE;
    cmd.set_tile.q = 3;
    cmd.set_tile.r = 3;
    cmd.set_tile.state = 1;
    
    PatikaError err = patika_submit_command(ctx, &cmd);
    TEST_ASSERT_EQUAL(PATIKA_OK, err);
    
    patika_tick(ctx);
}

void test_poll_events(void) {
    PatikaEvent events[10];
    uint32_t count = patika_poll_events(ctx, events, 10);
    TEST_ASSERT_EQUAL(0, count);
}

void test_get_stats(void) {
    patika_tick(ctx);
    
    PatikaStats stats = patika_get_stats(ctx);
    TEST_ASSERT_EQUAL(1, stats.total_ticks);
}

void test_snapshot_version_increments(void) {
    const PatikaSnapshot *snap1 = patika_get_snapshot(ctx);
    uint64_t v1 = snap1->version;
    
    patika_tick(ctx);
    
    const PatikaSnapshot *snap2 = patika_get_snapshot(ctx);
    uint64_t v2 = snap2->version;
    
    TEST_ASSERT_TRUE(v2 > v1);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_context_creation);
    RUN_TEST(test_add_agent);
    RUN_TEST(test_add_multiple_agents);
    RUN_TEST(test_remove_agent);
    RUN_TEST(test_set_goal);
    RUN_TEST(test_set_tile_state);
    RUN_TEST(test_poll_events);
    RUN_TEST(test_get_stats);
    RUN_TEST(test_snapshot_version_increments);
    
    return UNITY_END();
}