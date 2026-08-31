/**
 * @file test_benchmark.c
 * @brief Tick-rate benchmark: 50/100/200 agents on a 256×256 rectangular map.
 *
 * Run 10,000 ticks per configuration and report:
 *   - Total wall time
 *   - Ticks per second
 *   - Average tick latency (ms)
 *
 * Build in Release (-O3) for meaningful numbers.
 */

#include "unity.h"
#include "patika.h"
#include <stdio.h>
#include <time.h>

void setUp(void)    {}
void tearDown(void) {}

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static void run_benchmark(const char *label, uint32_t agent_count)
{
    const uint32_t W = 256, H = 256;
    const int      TICKS = 10000;

    PatikaConfig cfg = {
        .grid_type          = MAP_TYPE_RECTANGULAR,
        .max_agents         = agent_count,
        .max_barracks       = 0,
        .grid_width         = W,
        .grid_height        = H,
        .command_queue_size = agent_count * 2 + 64,
        .event_queue_size   = agent_count * 2 + 64,
        .rng_seed           = 42
    };

    PatikaHandle ctx = patika_create(&cfg);
    TEST_ASSERT_NOT_NULL(ctx);

    /* Scatter agents across the map using a simple stride pattern. */
    for (uint32_t i = 0; i < agent_count; i++) {
        PatikaCommand cmd = {0};
        cmd.type              = CMD_ADD_AGENT;
        cmd.add_agent.start_q = (int32_t)((i * 17u) % W);
        cmd.add_agent.start_r = (int32_t)((i * 13u) % H);
        patika_submit_command(ctx, &cmd);
    }
    patika_tick(ctx);

    /* Add ~30% obstacle density in a band across the middle of the map. */
    for (uint32_t r = H / 4; r < H * 3 / 4; r++) {
        for (uint32_t q = 0; q < W; q++) {
            if ((q + r) % 3 == 0) {
                PatikaCommand cmd = {0};
                cmd.type        = CMD_SET_TILE_STATE;
                cmd.set_tile.q  = (int32_t)q;
                cmd.set_tile.r  = (int32_t)r;
                cmd.set_tile.state = 1;
                patika_submit_command(ctx, &cmd);
            }
        }
    }
    patika_tick(ctx);

    /* Give every agent a goal on the other side of the obstacle band. */
    const PatikaSnapshot *snap = patika_get_snapshot(ctx);
    for (uint32_t i = 0; i < snap->agent_count; i++) {
        PatikaCommand cmd = {0};
        cmd.type              = CMD_SET_GOAL;
        cmd.set_goal.agent_id = snap->agents[i].id;
        cmd.set_goal.goal_q   = (int32_t)((snap->agents[i].pos_q + W / 2) % W);
        cmd.set_goal.goal_r   = (int32_t)((snap->agents[i].pos_r + H / 2) % H);
        patika_submit_command(ctx, &cmd);
    }

    double t0      = now_sec();
    double worst   = 0.0;

    for (int t = 0; t < TICKS; t++) {
        double t_tick = now_sec();
        patika_tick(ctx);
        double dt = now_sec() - t_tick;
        if (dt > worst) worst = dt;
    }

    double elapsed = now_sec() - t0;

    printf("\n[BENCHMARK] %-24s  %5d ticks | %7.2f s | %8.0f ticks/s | avg %6.3f ms | worst %6.3f ms\n",
           label, TICKS, elapsed,
           (double)TICKS / elapsed,
           elapsed / TICKS * 1000.0,
           worst * 1000.0);

    /* Drain event queue so the SPSC isn't full. */
    PatikaEvent evts[64];
    while (patika_poll_events(ctx, evts, 64) > 0) {}

    patika_destroy(ctx);

    /* Just assert we completed without crashing. */
    TEST_ASSERT_TRUE(elapsed > 0.0);
}

void test_benchmark_50_agents(void)  { run_benchmark("50  agents, 256x256", 50);  }
void test_benchmark_100_agents(void) { run_benchmark("100 agents, 256x256", 100); }
void test_benchmark_200_agents(void) { run_benchmark("200 agents, 256x256", 200); }

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_benchmark_50_agents);
    RUN_TEST(test_benchmark_100_agents);
    RUN_TEST(test_benchmark_200_agents);
    return UNITY_END();
}
