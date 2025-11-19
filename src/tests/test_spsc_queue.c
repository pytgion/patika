/**
 * @file test_spsc_queue.c
 * @brief Unit tests for Single-Producer Single-Consumer event queue
 */

#include "internal/patika_internal.h"
#include "unity.h"
#include <pthread.h>
#include <unistd.h>

static SPSCEventQueue queue;

void setUp(void)
{
    spsc_init(&queue, 16);
}

void tearDown(void)
{
    spsc_destroy(&queue);
}

// ============================================================================
// Basic Functionality Tests
// ============================================================================

void test_spsc_init(void)
{
    TEST_ASSERT_NOT_NULL(queue.buffer);
    TEST_ASSERT_EQUAL_UINT32(16, queue.capacity);
}

void test_spsc_push_single(void)
{
    PatikaEvent evt = {EVENT_REACHED_GOAL, 42, 5, 10};

    int result = spsc_push(&queue, &evt);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_spsc_push_and_pop(void)
{
    PatikaEvent evt = {EVENT_STUCK, 17, -3, 7};
    spsc_push(&queue, &evt);

    PatikaEvent out;
    int result = spsc_pop(&queue, &out);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL(EVENT_STUCK, out.type);
    TEST_ASSERT_EQUAL_UINT32(17, out.agent_id);
    TEST_ASSERT_EQUAL_INT32(-3, out.pos_q);
    TEST_ASSERT_EQUAL_INT32(7, out.pos_r);
}

void test_spsc_pop_empty(void)
{
    PatikaEvent out;
    int result = spsc_pop(&queue, &out);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_spsc_fifo_order(void)
{
    // Push events with sequential agent IDs
    for (uint32_t i = 0; i < 10; i++)
    {
        PatikaEvent evt = {EVENT_REACHED_GOAL, i, 0, 0};
        spsc_push(&queue, &evt);
    }

    // Verify FIFO order
    for (uint32_t i = 0; i < 10; i++)
    {
        PatikaEvent out;
        spsc_pop(&queue, &out);
        TEST_ASSERT_EQUAL_UINT32(i, out.agent_id);
    }
}

void test_spsc_full_queue(void)
{
    PatikaEvent evt = {EVENT_BLOCKED, 1, 0, 0};

    // Fill queue (capacity - 1)
    for (uint32_t i = 0; i < queue.capacity - 1; i++)
    {
        int result = spsc_push(&queue, &evt);
        TEST_ASSERT_EQUAL_INT(0, result);
    }

    // Next push should fail
    int result = spsc_push(&queue, &evt);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_spsc_wrap_around(void)
{
    PatikaEvent evt = {EVENT_REACHED_GOAL, 0, 0, 0};

    // Multiple cycles to ensure wrap-around works
    for (int cycle = 0; cycle < 3; cycle++)
    {
        for (uint32_t i = 0; i < queue.capacity - 1; i++)
        {
            evt.agent_id = cycle * 100 + i;
            spsc_push(&queue, &evt);
        }

        for (uint32_t i = 0; i < queue.capacity - 1; i++)
        {
            PatikaEvent out;
            spsc_pop(&queue, &out);
            TEST_ASSERT_EQUAL_UINT32(cycle * 100 + i, out.agent_id);
        }
    }
}

void test_spsc_all_event_types(void)
{
    EventType types[] = {EVENT_REACHED_GOAL, EVENT_STUCK, EVENT_BLOCKED, EVENT_REPLAN_NEEDED, EVENT_AGENT_REMOVED};

    for (int i = 0; i < 5; i++)
    {
        PatikaEvent evt = {types[i], i, i, i};
        spsc_push(&queue, &evt);
    }

    for (int i = 0; i < 5; i++)
    {
        PatikaEvent out;
        spsc_pop(&queue, &out);
        TEST_ASSERT_EQUAL(types[i], out.type);
    }
}

// ============================================================================
// Thread Safety Tests (Producer/Consumer)
// ============================================================================

typedef struct
{
    SPSCEventQueue *queue;
    int count;
    int *received;
} ConsumerArgs;

static void *producer_thread_spsc(void *arg)
{
    SPSCEventQueue *q = (SPSCEventQueue *)arg;

    for (int i = 0; i < 200; i++)
    {
        PatikaEvent evt = {EVENT_REACHED_GOAL, (uint32_t)i, 0, 0};

        // Retry on full
        while (spsc_push(q, &evt) != 0)
        {
            usleep(10);
        }
    }

    return NULL;
}

static void *consumer_thread_spsc(void *arg)
{
    ConsumerArgs *args = (ConsumerArgs *)arg;

    while (*args->received < args->count)
    {
        PatikaEvent evt;
        if (spsc_pop(args->queue, &evt) == 0)
        {
            (*args->received)++;
        }
        else
        {
            usleep(10);
        }
    }

    return NULL;
}

void test_spsc_producer_consumer_threads(void)
{
    pthread_t producer, consumer;
    int received = 0;
    ConsumerArgs args = {&queue, 200, &received};

    pthread_create(&consumer, NULL, consumer_thread_spsc, &args);
    pthread_create(&producer, NULL, producer_thread_spsc, &queue);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    TEST_ASSERT_EQUAL_INT(200, received);
}

void test_spsc_high_frequency(void)
{
    // Rapid push/pop cycles
    for (int i = 0; i < 1000; i++)
    {
        PatikaEvent evt = {EVENT_STUCK, (uint32_t)i, 0, 0};
        spsc_push(&queue, &evt);

        PatikaEvent out;
        spsc_pop(&queue, &out);
        TEST_ASSERT_EQUAL_UINT32((uint32_t)i, out.agent_id);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_spsc_init);
    RUN_TEST(test_spsc_push_single);
    RUN_TEST(test_spsc_push_and_pop);
    RUN_TEST(test_spsc_pop_empty);
    RUN_TEST(test_spsc_fifo_order);
    RUN_TEST(test_spsc_full_queue);
    RUN_TEST(test_spsc_wrap_around);
    RUN_TEST(test_spsc_all_event_types);
    RUN_TEST(test_spsc_producer_consumer_threads);
    RUN_TEST(test_spsc_high_frequency);

    return UNITY_END();
}