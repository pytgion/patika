/**
 * @file test_stress.c
 * @brief Stress tests: random goals, dynamic obstacles, rapid add/remove.
 *
 * Verifies the simulation does not crash, leak, or produce out-of-bounds
 * agent positions under sustained load and chaotic state changes.
 */

#include "unity.h"
#include "patika.h"
#include <stdint.h>

static PatikaHandle ctx = NULL;

/* Simple xorshift64 so tests are deterministic without linking a real RNG. */
static uint64_t xr_state = 0xDEADBEEFCAFEBABEull;
static uint32_t xr_next(void)
{
    xr_state ^= xr_state << 13;
    xr_state ^= xr_state >> 7;
    xr_state ^= xr_state << 17;
    return (uint32_t)(xr_state >> 32);
}
static int32_t xr_range(int32_t lo, int32_t hi) /* [lo, hi) */
{
    return lo + (int32_t)(xr_next() % (uint32_t)(hi - lo));
}

#define W 32
#define H 32

void setUp(void)
{
    xr_state = 0xDEADBEEFCAFEBABEull;

    PatikaConfig cfg = {
        .grid_type          = MAP_TYPE_RECTANGULAR,
        .max_agents         = 200,
        .max_barracks       = 0,
        .grid_width         = W,
        .grid_height        = H,
        .command_queue_size = 1024,
        .event_queue_size   = 1024,
        .rng_seed           = 999
    };
    ctx = patika_create(&cfg);
    TEST_ASSERT_NOT_NULL(ctx);
}

void tearDown(void)
{
    patika_destroy(ctx);
    ctx = NULL;
}

static void drain_events(void)
{
    PatikaEvent buf[64];
    while (patika_poll_events(ctx, buf, 64) > 0) {}
}

static void assert_agents_in_bounds(const PatikaSnapshot *snap)
{
    for (uint32_t i = 0; i < snap->agent_count; i++) {
        TEST_ASSERT_TRUE(snap->agents[i].pos_q >= 0 && snap->agents[i].pos_q < W);
        TEST_ASSERT_TRUE(snap->agents[i].pos_r >= 0 && snap->agents[i].pos_r < H);
    }
}

/* -------------------------------------------------------------------------
 * Test 1: random spawns + random goals + dynamic tile toggles
 * ------------------------------------------------------------------------- */

void test_stress_random_goals_dynamic_obstacles(void)
{
    /* Spawn 80 agents at random open positions. */
    for (int i = 0; i < 80; i++) {
        PatikaCommand cmd = {0};
        cmd.type              = CMD_ADD_AGENT;
        cmd.add_agent.start_q = xr_range(0, W);
        cmd.add_agent.start_r = xr_range(0, H);
        patika_submit_command(ctx, &cmd);
    }
    patika_tick(ctx);

    /* Assign random goals. */
    const PatikaSnapshot *snap = patika_get_snapshot(ctx);
    for (uint32_t i = 0; i < snap->agent_count; i++) {
        PatikaCommand cmd = {0};
        cmd.type              = CMD_SET_GOAL;
        cmd.set_goal.agent_id = snap->agents[i].id;
        cmd.set_goal.goal_q   = xr_range(0, W);
        cmd.set_goal.goal_r   = xr_range(0, H);
        patika_submit_command(ctx, &cmd);
    }

    /* Run 500 ticks; every 20 ticks toggle a random tile. */
    for (int t = 0; t < 500; t++) {
        if (t % 20 == 0) {
            PatikaCommand cmd = {0};
            cmd.type           = CMD_SET_TILE_STATE;
            cmd.set_tile.q     = xr_range(1, W - 1);
            cmd.set_tile.r     = xr_range(1, H - 1);
            cmd.set_tile.state = (uint8_t)((t / 20) % 2);
            patika_submit_command(ctx, &cmd);
        }
        patika_tick(ctx);
        drain_events();
    }

    snap = patika_get_snapshot(ctx);
    TEST_ASSERT_TRUE(snap->agent_count > 0);
    assert_agents_in_bounds(snap);
}

/* -------------------------------------------------------------------------
 * Test 2: rapid add/remove waves with goals in between
 * ------------------------------------------------------------------------- */

void test_stress_rapid_add_remove_with_goals(void)
{
    for (int wave = 0; wave < 30; wave++) {
        /* Add up to 10 agents per wave. */
        for (int i = 0; i < 10; i++) {
            PatikaCommand cmd = {0};
            cmd.type              = CMD_ADD_AGENT;
            cmd.add_agent.start_q = xr_range(0, W);
            cmd.add_agent.start_r = xr_range(0, H);
            patika_submit_command(ctx, &cmd);
        }
        patika_tick(ctx);

        /* Send goals to all living agents. */
        const PatikaSnapshot *snap = patika_get_snapshot(ctx);
        for (uint32_t i = 0; i < snap->agent_count; i++) {
            PatikaCommand cmd = {0};
            cmd.type              = CMD_SET_GOAL;
            cmd.set_goal.agent_id = snap->agents[i].id;
            cmd.set_goal.goal_q   = xr_range(0, W);
            cmd.set_goal.goal_r   = xr_range(0, H);
            patika_submit_command(ctx, &cmd);
        }

        for (int t = 0; t < 5; t++) {
            patika_tick(ctx);
            drain_events();
        }

        /* Remove the first half. */
        snap = patika_get_snapshot(ctx);
        for (uint32_t i = 0; i < snap->agent_count / 2; i++) {
            PatikaCommand cmd = {0};
            cmd.type                   = CMD_REMOVE_AGENT;
            cmd.remove_agent.agent_id  = snap->agents[i].id;
            patika_submit_command(ctx, &cmd);
        }
        patika_tick(ctx);
        drain_events();
    }

    const PatikaSnapshot *snap = patika_get_snapshot(ctx);
    assert_agents_in_bounds(snap);
}

/* -------------------------------------------------------------------------
 * Test 3: wall appears, goals become unreachable, wall disappears, replan
 * ------------------------------------------------------------------------- */

void test_stress_wall_appears_and_clears(void)
{
    /* Spawn agents on the left half. */
    for (int i = 0; i < 10; i++) {
        PatikaCommand cmd = {0};
        cmd.type              = CMD_ADD_AGENT;
        cmd.add_agent.start_q = xr_range(0, W / 2 - 2);
        cmd.add_agent.start_r = xr_range(0, H);
        patika_submit_command(ctx, &cmd);
    }
    patika_tick(ctx);

    /* Erect a solid wall at col 14 (runs top to bottom). */
    for (int r = 0; r < H; r++) {
        PatikaCommand cmd = {0};
        cmd.type           = CMD_SET_TILE_STATE;
        cmd.set_tile.q     = 14;
        cmd.set_tile.r     = r;
        cmd.set_tile.state = 1;
        patika_submit_command(ctx, &cmd);
    }
    patika_tick(ctx);

    /* Goals are on the right side — unreachable. */
    const PatikaSnapshot *snap = patika_get_snapshot(ctx);
    for (uint32_t i = 0; i < snap->agent_count; i++) {
        PatikaCommand cmd = {0};
        cmd.type              = CMD_SET_GOAL;
        cmd.set_goal.agent_id = snap->agents[i].id;
        cmd.set_goal.goal_q   = xr_range(16, W);
        cmd.set_goal.goal_r   = xr_range(0, H);
        patika_submit_command(ctx, &cmd);
    }
    for (int t = 0; t < 15; t++) { patika_tick(ctx); drain_events(); }

    /* Clear the wall and reassign the same goals. */
    for (int r = 0; r < H; r++) {
        PatikaCommand cmd = {0};
        cmd.type           = CMD_SET_TILE_STATE;
        cmd.set_tile.q     = 14;
        cmd.set_tile.r     = r;
        cmd.set_tile.state = 0;
        patika_submit_command(ctx, &cmd);
    }
    patika_tick(ctx);

    snap = patika_get_snapshot(ctx);
    for (uint32_t i = 0; i < snap->agent_count; i++) {
        PatikaCommand cmd = {0};
        cmd.type              = CMD_SET_GOAL;
        cmd.set_goal.agent_id = snap->agents[i].id;
        cmd.set_goal.goal_q   = xr_range(16, W);
        cmd.set_goal.goal_r   = xr_range(0, H);
        patika_submit_command(ctx, &cmd);
    }
    for (int t = 0; t < 60; t++) { patika_tick(ctx); drain_events(); }

    snap = patika_get_snapshot(ctx);
    TEST_ASSERT_TRUE(snap->agent_count > 0);
    assert_agents_in_bounds(snap);
}

/* -------------------------------------------------------------------------
 * Test 4: stats are sane after many ticks
 * ------------------------------------------------------------------------- */

void test_stress_stats_sane(void)
{
    for (int i = 0; i < 20; i++) {
        PatikaCommand cmd = {0};
        cmd.type              = CMD_ADD_AGENT;
        cmd.add_agent.start_q = xr_range(0, W);
        cmd.add_agent.start_r = xr_range(0, H);
        patika_submit_command(ctx, &cmd);
    }
    patika_tick(ctx);

    const int TICKS = 200;
    for (int t = 0; t < TICKS; t++) {
        patika_tick(ctx);
        drain_events();
    }

    PatikaStats stats = patika_get_stats(ctx);
    TEST_ASSERT_EQUAL_UINT32(TICKS + 1, stats.total_ticks);
    TEST_ASSERT_TRUE(stats.active_agents > 0 && stats.active_agents <= 20);
    TEST_ASSERT_TRUE(stats.commands_processed > 0);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_stress_random_goals_dynamic_obstacles);
    RUN_TEST(test_stress_rapid_add_remove_with_goals);
    RUN_TEST(test_stress_wall_appears_and_clears);
    RUN_TEST(test_stress_stats_sane);
    return UNITY_END();
}
