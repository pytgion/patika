#include "unity.h"
#include "patika.h"
#include <stdlib.h>
#include <string.h>

static PatikaHandle ctx = NULL;

void setUp(void) {
    PatikaConfig config = {
        .grid_type = MAP_TYPE_HEXAGONAL,
        .max_agents = 500,
        .max_barracks = 20,
        .grid_width = 30,
        .grid_height = 30,
        .sector_size = 0,
        .command_queue_size = 1024,
        .event_queue_size = 1024,
        .rng_seed = 67890
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

void test_spawn_50_agents(void) {
    for (int i = 0; i < 50; i++) {
        AddAgentPayload *payload = malloc(sizeof(AddAgentPayload));
        int q = (i % 10) - 5;
        int r = (i / 10) - 5;
        payload->start_q = q;
        payload->start_r = r;
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
    TEST_ASSERT_EQUAL(50, snap->agent_count);
    
    PatikaStats stats = patika_get_stats(ctx);
    TEST_ASSERT_EQUAL(50, stats.active_agents);
}

void test_batch_commands(void) {
    for (int i = 0; i < 10; i++) {
        AddAgentPayload *payload = malloc(sizeof(AddAgentPayload));
        payload->start_q = i;
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
    }
    
    patika_tick(ctx);
    
    const PatikaSnapshot *snap = patika_get_snapshot(ctx);
    
    PatikaCommand cmds[10];
    for (int i = 0; i < 10; i++) {
        cmds[i].type = CMD_SET_GOAL;
        cmds[i].set_goal.agent_id = snap->agents[i].id;
        cmds[i].set_goal.goal_q = 10;
        cmds[i].set_goal.goal_r = 10;
    }
    
    PatikaError err = patika_submit_commands(ctx, cmds, 10);
    TEST_ASSERT_EQUAL(PATIKA_OK, err);
    
    patika_tick(ctx);
}

void test_rapid_spawn_destroy(void) {
    for (int iter = 0; iter < 10; iter++) {
        for (int i = 0; i < 20; i++) {
            AddAgentPayload *payload = malloc(sizeof(AddAgentPayload));
            payload->start_q = i % 5;
            payload->start_r = i / 5;
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
        }
        
        patika_tick(ctx);
        
        const PatikaSnapshot *snap = patika_get_snapshot(ctx);
        
        for (uint32_t i = 0; i < snap->agent_count; i++) {
            PatikaCommand cmd = {0};
            cmd.type = CMD_REMOVE_AGENT;
            cmd.remove_agent.agent_id = snap->agents[i].id;
            patika_submit_command(ctx, &cmd);
        }
        
        patika_tick(ctx);
    }
    
    const PatikaSnapshot *snap = patika_get_snapshot(ctx);
    TEST_ASSERT_EQUAL(0, snap->agent_count);
}

void test_agent_movement_simulation(void) {
    for (int i = 0; i < 5; i++) {
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
    }
    
    patika_tick(ctx);
    
    const PatikaSnapshot *snap = patika_get_snapshot(ctx);
    
    for (uint32_t i = 0; i < snap->agent_count; i++) {
        PatikaCommand cmd = {0};
        cmd.type = CMD_SET_GOAL;
        cmd.set_goal.agent_id = snap->agents[i].id;
        cmd.set_goal.goal_q = 10;
        cmd.set_goal.goal_r = 10;
        patika_submit_command(ctx, &cmd);
    }
    
    for (int t = 0; t < 20; t++) {
        patika_tick(ctx);
    }
    
    snap = patika_get_snapshot(ctx);
    TEST_ASSERT_EQUAL(5, snap->agent_count);
}

void test_queue_capacity(void) {
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
    
    int success = 0;
    for (int i = 0; i < 2000; i++) {
        goal_cmd.set_goal.goal_q = i;
        goal_cmd.set_goal.goal_r = i;
        if (patika_submit_command(ctx, &goal_cmd) == PATIKA_OK) {
            success++;
        }
    }
    
    TEST_ASSERT_TRUE(success > 0);
}

void test_stats_tracking(void) {
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
    
    for (int i = 0; i < 5; i++) {
        patika_tick(ctx);
    }
    
    PatikaStats stats = patika_get_stats(ctx);
    TEST_ASSERT_EQUAL(5, stats.total_ticks);
    TEST_ASSERT_TRUE(stats.commands_processed > 0);
}

void test_snapshot_consistency(void) {
    for (int i = 0; i < 3; i++) {
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
        patika_submit_command(ctx, &cmd);
    }
    
    patika_tick(ctx);
    
    const PatikaSnapshot *snap1 = patika_get_snapshot(ctx);
    uint64_t v1 = snap1->version;
    uint32_t count1 = snap1->agent_count;
    
    const PatikaSnapshot *snap2 = patika_get_snapshot(ctx);
    
    TEST_ASSERT_EQUAL(v1, snap2->version);
    TEST_ASSERT_EQUAL(count1, snap2->agent_count);
}

void test_rectangular_map(void) {
    patika_destroy(ctx);
    
    PatikaConfig config = {
        .grid_type = MAP_TYPE_RECTANGULAR,
        .max_agents = 100,
        .max_barracks = 10,
        .grid_width = 50,
        .grid_height = 50,
        .sector_size = 0,
        .command_queue_size = 256,
        .event_queue_size = 256,
        .rng_seed = 11111
    };
    ctx = patika_create(&config);
    TEST_ASSERT_NOT_NULL(ctx);
    
    AddAgentPayload *payload = malloc(sizeof(AddAgentPayload));
    payload->start_q = 10;
    payload->start_r = 10;
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
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_spawn_50_agents);
    RUN_TEST(test_batch_commands);
    RUN_TEST(test_rapid_spawn_destroy);
    RUN_TEST(test_agent_movement_simulation);
    RUN_TEST(test_queue_capacity);
    RUN_TEST(test_stats_tracking);
    RUN_TEST(test_snapshot_consistency);
    RUN_TEST(test_rectangular_map);
    
    return UNITY_END();
}