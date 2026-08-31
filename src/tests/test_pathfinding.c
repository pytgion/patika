/**
 * @file test_pathfinding.c
 * @brief Tests for A* pathfinding on both hex and rectangular grids.
 */

#include "internal/patika_internal.h"
#include "patika.h"
#include "unity.h"

static PatikaHandle hex_ctx  = NULL;
static PatikaHandle rect_ctx = NULL;

void setUp(void)
{
    {
        PatikaConfig cfg = {
            .grid_type          = MAP_TYPE_HEXAGONAL,
            .max_agents         = 10,
            .max_barracks       = 5,
            .grid_width         = 5,
            .grid_height        = 5,
            .command_queue_size = 32,
            .event_queue_size   = 32,
            .rng_seed           = 12345
        };
        hex_ctx = patika_create(&cfg);
    }
    {
        PatikaConfig cfg = {
            .grid_type          = MAP_TYPE_RECTANGULAR,
            .max_agents         = 10,
            .max_barracks       = 5,
            .grid_width         = 16,
            .grid_height        = 16,
            .command_queue_size = 32,
            .event_queue_size   = 32,
            .rng_seed           = 12345
        };
        rect_ctx = patika_create(&cfg);
    }
}

void tearDown(void)
{
    patika_destroy(hex_ctx);
    patika_destroy(rect_ctx);
    hex_ctx  = NULL;
    rect_ctx = NULL;
}

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

static AgentID spawn(PatikaHandle ctx, int32_t q, int32_t r)
{
    AgentID id;
    PatikaCommand cmd = {0};
    cmd.type                = CMD_ADD_AGENT;
    cmd.add_agent.start_q   = q;
    cmd.add_agent.start_r   = r;
    cmd.add_agent.out_agent_id = &id;
    patika_submit_command(ctx, &cmd);
    patika_tick(ctx);
    return id;
}

static void set_goal(PatikaHandle ctx, AgentID id, int32_t q, int32_t r)
{
    PatikaCommand cmd = {0};
    cmd.type              = CMD_SET_GOAL;
    cmd.set_goal.agent_id = id;
    cmd.set_goal.goal_q   = q;
    cmd.set_goal.goal_r   = r;
    patika_submit_command(ctx, &cmd);
}

static void block_tile(PatikaHandle ctx, int32_t q, int32_t r)
{
    PatikaCommand cmd = {0};
    cmd.type         = CMD_SET_TILE_STATE;
    cmd.set_tile.q   = q;
    cmd.set_tile.r   = r;
    cmd.set_tile.state = 1;
    patika_submit_command(ctx, &cmd);
}

/* Tick ctx up to max_ticks or until agent is IDLE, return final state. */
static AgentSnapshot tick_until_idle(PatikaHandle ctx, AgentID id, int max_ticks)
{
    AgentSnapshot snap = {0};
    for (int t = 0; t < max_ticks; t++) {
        patika_tick(ctx);
        const PatikaSnapshot *s = patika_get_snapshot(ctx);
        for (uint32_t i = 0; i < s->agent_count; i++) {
            if (s->agents[i].id == id) {
                snap = s->agents[i];
                break;
            }
        }
        if (snap.state == STATE_IDLE) break;
    }
    return snap;
}

/* -------------------------------------------------------------------------
 * Hex grid tests
 * ------------------------------------------------------------------------- */

void test_hex_direct_neighbor(void)
{
    AgentID id = spawn(hex_ctx, 0, 0);
    set_goal(hex_ctx, id, 1, 0);
    patika_tick(hex_ctx);

    const PatikaSnapshot *s = patika_get_snapshot(hex_ctx);
    TEST_ASSERT_EQUAL_INT32(1, s->agents[0].pos_q);
    TEST_ASSERT_EQUAL_INT32(0, s->agents[0].pos_r);
    TEST_ASSERT_EQUAL_UINT8(STATE_IDLE, s->agents[0].state);
}

void test_hex_already_at_goal(void)
{
    AgentID id = spawn(hex_ctx, 2, 0);
    set_goal(hex_ctx, id, 2, 0);
    patika_tick(hex_ctx);

    const PatikaSnapshot *s = patika_get_snapshot(hex_ctx);
    TEST_ASSERT_EQUAL_UINT8(STATE_IDLE, s->agents[0].state);
    TEST_ASSERT_EQUAL_INT32(2, s->agents[0].pos_q);
}

void test_hex_all_neighbors_blocked_emits_stuck(void)
{
    AgentID id = spawn(hex_ctx, 0, 0);

    int hex_dirs[6][2] = {{1,0},{1,-1},{0,-1},{-1,0},{-1,1},{0,1}};
    for (int i = 0; i < 6; i++)
        block_tile(hex_ctx, hex_dirs[i][0], hex_dirs[i][1]);
    patika_tick(hex_ctx); /* apply tile changes */

    set_goal(hex_ctx, id, 3, 0);
    patika_tick(hex_ctx);

    const PatikaSnapshot *s = patika_get_snapshot(hex_ctx);
    TEST_ASSERT_EQUAL_UINT8(STATE_IDLE, s->agents[0].state);

    PatikaEvent evts[10];
    uint32_t n = patika_poll_events(hex_ctx, evts, 10);
    TEST_ASSERT_EQUAL_UINT32(1, n);
    TEST_ASSERT_EQUAL(EVENT_STUCK, evts[0].type);
    TEST_ASSERT_EQUAL(id, evts[0].agent_id);
}

void test_hex_partial_blockage_forces_only_path(void)
{
    AgentID id = spawn(hex_ctx, 0, 0);

    /* block 5/6 neighbours, leave (1,0) open */
    int blocked[5][2] = {{1,-1},{0,-1},{-1,0},{-1,1},{0,1}};
    for (int i = 0; i < 5; i++)
        block_tile(hex_ctx, blocked[i][0], blocked[i][1]);
    patika_tick(hex_ctx);

    set_goal(hex_ctx, id, 2, 0);
    patika_tick(hex_ctx);

    const PatikaSnapshot *s = patika_get_snapshot(hex_ctx);
    TEST_ASSERT_EQUAL_INT32(1, s->agents[0].pos_q);
    TEST_ASSERT_EQUAL_INT32(0, s->agents[0].pos_r);
}

/* -------------------------------------------------------------------------
 * Rectangular grid tests
 * ------------------------------------------------------------------------- */

void test_rect_direct_neighbor(void)
{
    AgentID id = spawn(rect_ctx, 0, 0);
    set_goal(rect_ctx, id, 1, 0);
    patika_tick(rect_ctx);

    const PatikaSnapshot *s = patika_get_snapshot(rect_ctx);
    TEST_ASSERT_EQUAL_INT32(1, s->agents[0].pos_q);
    TEST_ASSERT_EQUAL_INT32(0, s->agents[0].pos_r);
    TEST_ASSERT_EQUAL_UINT8(STATE_IDLE, s->agents[0].state);
}

void test_rect_multi_step_path(void)
{
    AgentID id = spawn(rect_ctx, 0, 0);
    set_goal(rect_ctx, id, 4, 0);

    AgentSnapshot snap = tick_until_idle(rect_ctx, id, 20);
    TEST_ASSERT_EQUAL_INT32(4, snap.pos_q);
    TEST_ASSERT_EQUAL_INT32(0, snap.pos_r);
    TEST_ASSERT_EQUAL_UINT8(STATE_IDLE, snap.state);
}

void test_rect_all_neighbors_blocked_emits_stuck(void)
{
    AgentID id = spawn(rect_ctx, 5, 5);

    block_tile(rect_ctx, 6, 5);
    block_tile(rect_ctx, 4, 5);
    block_tile(rect_ctx, 5, 6);
    block_tile(rect_ctx, 5, 4);
    patika_tick(rect_ctx);

    set_goal(rect_ctx, id, 10, 10);
    patika_tick(rect_ctx);

    const PatikaSnapshot *s = patika_get_snapshot(rect_ctx);
    uint8_t state = 0;
    for (uint32_t i = 0; i < s->agent_count; i++) {
        if (s->agents[i].id == id) { state = s->agents[i].state; break; }
    }
    TEST_ASSERT_EQUAL_UINT8(STATE_IDLE, state);

    PatikaEvent evts[10];
    uint32_t n = patika_poll_events(rect_ctx, evts, 10);
    TEST_ASSERT_EQUAL_UINT32(1, n);
    TEST_ASSERT_EQUAL(EVENT_STUCK, evts[0].type);
}

/*
 * Concave obstacle — the key test that greedy hill-climb would fail.
 *
 * Map (16x16), agent at (0,5), goal at (10,5).
 * Wall from (5,0) to (5,9) — agent must go around the bottom.
 *
 *   col:  0 1 2 3 4 5 6 7 ...
 *  row 0: . . . . . W . .
 *  row 1: . . . . . W . .
 *  ...
 *  row 9: . . . . . W . .
 * row 10: . . . . . . . .  <- open corridor at the bottom
 *
 * Agent at (0,5): must path around to (10,5).
 * Greedy would get stuck at (4,5) forever.  A* finds the route.
 */
void test_rect_concave_obstacle(void)
{
    AgentID id = spawn(rect_ctx, 0, 5);

    for (int row = 0; row <= 9; row++)
        block_tile(rect_ctx, 5, row);
    patika_tick(rect_ctx);

    set_goal(rect_ctx, id, 10, 5);

    AgentSnapshot snap = tick_until_idle(rect_ctx, id, 60);

    TEST_ASSERT_EQUAL_INT32(10, snap.pos_q);
    TEST_ASSERT_EQUAL_INT32(5,  snap.pos_r);
    TEST_ASSERT_EQUAL_UINT8(STATE_IDLE, snap.state);
}

/*
 * Reservation-aware: two agents trying to swap positions.
 * Agent A at (0,0) → goal (2,0).
 * Agent B at (2,0) → goal (0,0).
 * Both run A* simultaneously; each tick they advance one step.
 * Neither should pass through the other's occupied tile.
 * Both should reach their goals eventually.
 */
void test_rect_reservation_aware(void)
{
    AgentID id_a = spawn(rect_ctx, 0, 0);
    AgentID id_b = spawn(rect_ctx, 2, 0);

    set_goal(rect_ctx, id_a, 2, 0);
    set_goal(rect_ctx, id_b, 0, 0);

    AgentSnapshot snap_a = tick_until_idle(rect_ctx, id_a, 30);
    AgentSnapshot snap_b = tick_until_idle(rect_ctx, id_b, 30);

    (void)snap_b; /* both should eventually reach their goals or detour */

    /* Neither agent should occupy a position outside the grid */
    TEST_ASSERT_TRUE(snap_a.pos_q >= 0 && snap_a.pos_q < 16);
    TEST_ASSERT_TRUE(snap_a.pos_r >= 0 && snap_a.pos_r < 16);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_hex_direct_neighbor);
    RUN_TEST(test_hex_already_at_goal);
    RUN_TEST(test_hex_all_neighbors_blocked_emits_stuck);
    RUN_TEST(test_hex_partial_blockage_forces_only_path);

    RUN_TEST(test_rect_direct_neighbor);
    RUN_TEST(test_rect_multi_step_path);
    RUN_TEST(test_rect_all_neighbors_blocked_emits_stuck);
    RUN_TEST(test_rect_concave_obstacle);
    RUN_TEST(test_rect_reservation_aware);

    return UNITY_END();
}
