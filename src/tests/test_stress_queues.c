/**
 * @file test_stress_queues.c
 * @brief Stress tests for queue performance and thread safety
 */

#include "patika_core.h"
#include "unity.h"
#include <pthread.h>
#include <time.h>
#include <unistd.h>

static PatikaHandle handle;

void setUp(void)
{
    PatikaConfig config = {.grid_type = MAP_TYPE_HEXAGONAL,
                           .max_agents = 1000,
                           .max_barracks = 50,
                           .grid_width = 20,
                           .grid_height = 20,
                           .command_queue_size = 1024,
                           .event_queue_size = 1024,
                           .rng_seed = (uint64_t)time(NULL)};
    handle = patika_create(&config);
}

void tearDown(void)
{
    patika_destroy(handle);
}

// ============================================================================
// Queue Saturation Tests
// ============================================================================

void test_fill_command_queue(void)
{
    int submitted = 0;
    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_AGENT;

    // Try to fill queue
    for (int i = 0; i < 2000; i++)
    {
        if (patika_submit_command(handle, &cmd) == PATIKA_OK)
        {
            submitted++;
        }
        else
        {
            break; // Queue full
        }
    }

    // Should have submitted close to capacity
    TEST_ASSERT_GREATER_THAN(500, submitted);

    // Process all
    patika_tick(handle);

    // Verify stats
    PatikaStats stats = patika_get_stats(handle);
    TEST_ASSERT_EQUAL_UINT64(submitted, stats.commands_processed);
}

void test_rapid_command_submission(void)
{
    // Submit commands as fast as possible
    for (int batch = 0; batch < 10; batch++)
    {
        for (int i = 0; i < 50; i++)
        {
            AgentID id;
            patika_add_agent_sync(handle, i % 10, i / 10, 1, 1, PATIKA_INVALID_BARRACK_ID, &id);
        }
        patika_tick(handle);
    }

    // Should have processed all batches
    PatikaStats stats = patika_get_stats(handle);
    TEST_ASSERT_EQUAL_UINT64(10, stats.total_ticks);
    TEST_ASSERT_GREATER_OR_EQUAL(500, stats.commands_processed);
}

void test_event_queue_overflow_handling(void)
{
    // Create many agents in blocked positions to generate STUCK events
    for (int i = 0; i < 100; i++)
    {
        AgentID id;
        patika_add_agent_sync(handle, 0, 0, 1, 1, PATIKA_INVALID_BARRACK_ID, &id);
    }
    patika_tick(handle);

    // Block all neighbors
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

    // Set goals for all to trigger STUCK events
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

    // Poll events (might not get all if queue overflowed)
    PatikaEvent events[1000];
    uint32_t count = patika_poll_events(handle, events, 1000);

    // Should have gotten many STUCK events
    TEST_ASSERT_GREATER_THAN_UINT32(10, count);
}

// ============================================================================
// Multi-threaded Stress Tests
// ============================================================================

typedef struct
{
    PatikaHandle handle;
    int thread_id;
    int commands_per_thread;
    volatile int *total_submitted;
} StressThreadArgs;

static void *stress_producer(void *arg)
{
    StressThreadArgs *args = (StressThreadArgs *)arg;
    int submitted = 0;

    for (int i = 0; i < args->commands_per_thread; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_ADD_AGENT;
        cmd.add_agent.start_q = args->thread_id;
        cmd.add_agent.start_r = i % 10;
        cmd.add_agent.faction = args->thread_id % 2;
        cmd.add_agent.side = 1;

        // Retry on full queue with backoff
        int retries = 0;
        while (patika_submit_command(args->handle, &cmd) != PATIKA_OK)
        {
            usleep(100 * (retries + 1));
            retries++;
            if (retries > 100)
                break;
        }

        if (retries <= 100)
        {
            submitted++;
            __sync_fetch_and_add(args->total_submitted, 1);
        }
    }

    return (void *)(intptr_t)submitted;
}

void test_high_contention_mpsc_queue(void)
{
    const int NUM_THREADS = 8;
    const int CMDS_PER_THREAD = 100;

    pthread_t threads[NUM_THREADS];
    StressThreadArgs args[NUM_THREADS];
    volatile int total_submitted = 0;

    // Start producer threads
    for (int i = 0; i < NUM_THREADS; i++)
    {
        args[i].handle = handle;
        args[i].thread_id = i;
        args[i].commands_per_thread = CMDS_PER_THREAD;
        args[i].total_submitted = &total_submitted;
        pthread_create(&threads[i], NULL, stress_producer, &args[i]);
    }

    // Consumer thread (main) processes ticks
    int ticks = 0;
    while (ticks < 100)
    {
        patika_tick(handle);
        ticks++;
        usleep(1000); // 1ms between ticks
    }

    // Wait for producers
    int total_returned = 0;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        void *ret;
        pthread_join(threads[i], &ret);
        total_returned += (int)(intptr_t)ret;
    }

    // Final processing
    for (int i = 0; i < 10; i++)
    {
        patika_tick(handle);
    }

    TEST_ASSERT_GREATER_THAN(500, total_returned);

    PatikaStats stats = patika_get_stats(handle);
    TEST_ASSERT_GREATER_OR_EQUAL(500, stats.commands_processed);
}

void test_sustained_high_throughput(void)
{
    const int DURATION_TICKS = 100;
    const int COMMANDS_PER_TICK = 50;

    for (int tick = 0; tick < DURATION_TICKS; tick++)
    {
        // Submit burst of commands
        for (int i = 0; i < COMMANDS_PER_TICK; i++)
        {
            PatikaCommand cmd = {0};
            cmd.type = CMD_ADD_AGENT;
            cmd.add_agent.start_q = (tick % 10) - 5;
            cmd.add_agent.start_r = (i % 10) - 5;
            patika_submit_command(handle, &cmd);
        }

        patika_tick(handle);
    }

    PatikaStats stats = patika_get_stats(handle);
    TEST_ASSERT_EQUAL_UINT64(DURATION_TICKS, stats.total_ticks);
    TEST_ASSERT_GREATER_OR_EQUAL(DURATION_TICKS * COMMANDS_PER_TICK / 2, stats.commands_processed);
}

void test_queue_recovery_after_saturation(void)
{
    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_AGENT;

    // Saturate queue
    while (patika_submit_command(handle, &cmd) == PATIKA_OK)
    {
        // Keep submitting until full
    }

    // Process to drain
    patika_tick(handle);

    // Should be able to submit again
    PatikaError err = patika_submit_command(handle, &cmd);
    TEST_ASSERT_EQUAL(PATIKA_OK, err);
}

void test_alternating_burst_idle(void)
{
    for (int cycle = 0; cycle < 20; cycle++)
    {
        // Burst phase
        for (int i = 0; i < 100; i++)
        {
            AgentID id;
            patika_add_agent_sync(handle, i % 10, i / 10, 1, 1, PATIKA_INVALID_BARRACK_ID, &id);
        }
        patika_tick(handle);

        // Idle phase
        for (int i = 0; i < 5; i++)
        {
            patika_tick(handle);
        }
    }

    PatikaStats stats = patika_get_stats(handle);
    TEST_ASSERT_EQUAL_UINT64(120, stats.total_ticks); // 20 * 6
    TEST_ASSERT_GREATER_OR_EQUAL(1000, stats.commands_processed);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_fill_command_queue);
    RUN_TEST(test_rapid_command_submission);
    RUN_TEST(test_event_queue_overflow_handling);
    RUN_TEST(test_high_contention_mpsc_queue);
    RUN_TEST(test_sustained_high_throughput);
    RUN_TEST(test_queue_recovery_after_saturation);
    RUN_TEST(test_alternating_burst_idle);

    return UNITY_END();
}