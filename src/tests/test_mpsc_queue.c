/**
 * @file test_mpsc_queue.c
 * @brief Unit tests for Multi-Producer Single-Consumer command queue
 */

#include "internal/patika_internal.h"
#include "unity.h"
#include <pthread.h>
#include <unistd.h>

static MPSCCommandQueue queue;

void setUp(void)
{
    mpsc_init(&queue, 8);
}

void tearDown(void)
{
    mpsc_destroy(&queue);
}

// ============================================================================
// Basic Functionality Tests
// ============================================================================

void test_mpsc_init(void)
{
    TEST_ASSERT_NOT_NULL(queue.buffer);
    TEST_ASSERT_EQUAL_UINT32(8, queue.capacity);
    TEST_ASSERT_EQUAL_UINT32(0, queue.tail);
}

void test_mpsc_push_single(void)
{
    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_AGENT;
    cmd.add_agent.start_q = 5;
    cmd.add_agent.start_r = 10;

    int result = mpsc_push(&queue, &cmd);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_mpsc_push_and_pop(void)
{
    PatikaCommand cmd = {0};
    cmd.type = CMD_SET_GOAL;
    cmd.set_goal.agent_id = 42;
    cmd.set_goal.goal_q = 7;
    cmd.set_goal.goal_r = 3;

    mpsc_push(&queue, &cmd);

    PatikaCommand out = {0};
    int result = mpsc_pop(&queue, &out);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL(CMD_SET_GOAL, out.type);
    TEST_ASSERT_EQUAL_UINT32(42, out.set_goal.agent_id);
    TEST_ASSERT_EQUAL_INT32(7, out.set_goal.goal_q);
    TEST_ASSERT_EQUAL_INT32(3, out.set_goal.goal_r);
}

void test_mpsc_pop_empty(void)
{
    PatikaCommand out = {0};
    int result = mpsc_pop(&queue, &out);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_mpsc_fifo_order(void)
{
    PatikaCommand cmds[5];

    // Push 5 commands with different data
    for (int i = 0; i < 5; i++)
    {
        cmds[i].type = CMD_ADD_AGENT;
        cmds[i].add_agent.start_q = i;
        mpsc_push(&queue, &cmds[i]);
    }

    // Pop and verify FIFO order
    for (int i = 0; i < 5; i++)
    {
        PatikaCommand out;
        mpsc_pop(&queue, &out);
        TEST_ASSERT_EQUAL_INT32(i, out.add_agent.start_q);
    }
}

void test_mpsc_full_queue(void)
{
    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_AGENT;

    // Fill queue (capacity - 1 due to ring buffer)
    for (uint32_t i = 0; i < queue.capacity - 1; i++)
    {
        int result = mpsc_push(&queue, &cmd);
        TEST_ASSERT_EQUAL_INT(0, result);
    }

    // Next push should fail
    int result = mpsc_push(&queue, &cmd);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_mpsc_wrap_around(void)
{
    PatikaCommand cmd = {0};

    // Fill and empty multiple times to test wrap-around
    for (int cycle = 0; cycle < 3; cycle++)
    {
        for (uint32_t i = 0; i < queue.capacity - 1; i++)
        {
            cmd.add_agent.start_q = (int32_t)(cycle * 100 + i);
            mpsc_push(&queue, &cmd);
        }

        for (uint32_t i = 0; i < queue.capacity - 1; i++)
        {
            PatikaCommand out;
            mpsc_pop(&queue, &out);
            TEST_ASSERT_EQUAL_INT32(cycle * 100 + i, out.add_agent.start_q);
        }
    }
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

typedef struct
{
    MPSCCommandQueue *queue;
    int start_value;
    int count;
} ProducerArgs;

static void *producer_thread(void *arg)
{
    ProducerArgs *args = (ProducerArgs *)arg;

    for (int i = 0; i < args->count; i++)
    {
        PatikaCommand cmd = {0};
        cmd.type = CMD_ADD_AGENT;
        cmd.add_agent.start_q = args->start_value + i;

        // Retry on full queue
        while (mpsc_push(args->queue, &cmd) != 0)
        {
            usleep(100);
        }
    }

    return NULL;
}

void test_mpsc_multiple_producers(void)
{
    const int NUM_PRODUCERS = 4;
    const int CMDS_PER_PRODUCER = 50;

    pthread_t threads[NUM_PRODUCERS];
    ProducerArgs args[NUM_PRODUCERS];

    // Start producer threads
    for (int i = 0; i < NUM_PRODUCERS; i++)
    {
        args[i].queue = &queue;
        args[i].start_value = i * 1000;
        args[i].count = CMDS_PER_PRODUCER;
        pthread_create(&threads[i], NULL, producer_thread, &args[i]);
    }

    // Consume all commands
    int received[NUM_PRODUCERS] = {};
    int total_received = 0;

    while (total_received < NUM_PRODUCERS * CMDS_PER_PRODUCER)
    {
        PatikaCommand cmd;
        if (mpsc_pop(&queue, &cmd) == 0)
        {
            int producer_id = cmd.add_agent.start_q / 1000;
            received[producer_id]++;
            total_received++;
        }
        else
        {
            usleep(100);
        }
    }

    // Wait for all threads
    for (int i = 0; i < NUM_PRODUCERS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Verify all commands received
    for (int i = 0; i < NUM_PRODUCERS; i++)
    {
        TEST_ASSERT_EQUAL_INT(CMDS_PER_PRODUCER, received[i]);
    }
}

void test_mpsc_concurrent_push_pop(void)
{
    pthread_t producer;
    ProducerArgs args = {&queue, 0, 100};

    pthread_create(&producer, NULL, producer_thread, &args);

    // Consumer (main thread) pops while producer pushes
    int received = 0;
    while (received < 100)
    {
        PatikaCommand cmd;
        if (mpsc_pop(&queue, &cmd) == 0)
        {
            received++;
        }
    }

    pthread_join(producer, NULL);
    TEST_ASSERT_EQUAL_INT(100, received);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_mpsc_init);
    RUN_TEST(test_mpsc_push_single);
    RUN_TEST(test_mpsc_push_and_pop);
    RUN_TEST(test_mpsc_pop_empty);
    RUN_TEST(test_mpsc_fifo_order);
    RUN_TEST(test_mpsc_full_queue);
    RUN_TEST(test_mpsc_wrap_around);
    RUN_TEST(test_mpsc_multiple_producers);
    RUN_TEST(test_mpsc_concurrent_push_pop);

    return UNITY_END();
}
