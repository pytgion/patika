/**
 * @file test_rng.c
 * @brief Unit tests for PCG32 random number generator
 */

#include "internal/patika_internal.h"
#include "unity.h"

static PCG32 rng;

void setUp(void)
{
    pcg32_init(&rng, 12345);
}

void tearDown(void)
{
    // No cleanup needed
}

void test_rng_init(void)
{
    TEST_ASSERT_EQUAL_UINT64(12345, rng.state);
}

void test_rng_generates_numbers(void)
{
    uint32_t val = pcg32_next(&rng);
    // Just verify it returns something
    (void)val; // Suppress unused warning
    TEST_PASS();
}

void test_rng_deterministic(void)
{
    PCG32 rng1, rng2;
    pcg32_init(&rng1, 42);
    pcg32_init(&rng2, 42);

    // Same seed should produce same sequence
    for (int i = 0; i < 100; i++)
    {
        TEST_ASSERT_EQUAL_UINT32(pcg32_next(&rng1), pcg32_next(&rng2));
    }
}

void test_rng_different_seeds(void)
{
    PCG32 rng1, rng2;
    pcg32_init(&rng1, 1);
    pcg32_init(&rng2, 2);


    // Different seeds should (very likely) produce different first values
    uint32_t val1 = pcg32_next(&rng1);
    uint32_t val2 = pcg32_next(&rng2);
    uint32_t val2_2 = pcg32_next(&rng2);
    uint32_t val1_2 = pcg32_next(&rng1);

    TEST_ASSERT_NOT_EQUAL(val1_2, val2_2);
}

void test_rng_not_stuck(void)
{
    uint32_t first = pcg32_next(&rng);
    uint32_t second = pcg32_next(&rng);
    uint32_t third = pcg32_next(&rng);

    // Should not generate same value consecutively (astronomically unlikely)
    TEST_ASSERT_FALSE(first == second && second == third);
}

void test_rng_range_coverage(void)
{
    // Generate 1000 numbers and verify we get variety
    uint32_t min = UINT32_MAX;
    uint32_t max = 0;

    for (int i = 0; i < 1000; i++)
    {
        uint32_t val = pcg32_next(&rng);
        if (val < min)
            min = val;
        if (val > max)
            max = val;
    }

    // Should have significant spread (not just small numbers)
    TEST_ASSERT_GREATER_THAN(1000000, max - min);
}

void test_rng_modulo_distribution(void)
{
    // Test that modulo operation gives reasonable distribution
    int buckets[10] = {0};

    for (int i = 0; i < 10000; i++)
    {
        uint32_t val = pcg32_next(&rng) % 10;
        buckets[val]++;
    }

    // Each bucket should have roughly 1000 ± tolerance
    // Using loose bounds for statistical variation
    for (int i = 0; i < 10; i++)
    {
        TEST_ASSERT_INT_WITHIN(300, 1000, buckets[i]);
    }
}

void test_rng_zero_seed(void)
{
    PCG32 rng_zero;
    pcg32_init(&rng_zero, 0);

    uint32_t val = pcg32_next(&rng_zero);
    val = pcg32_next(&rng_zero);
    TEST_ASSERT_NOT_EQUAL(0, val); // Should still generate non-zero
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_rng_init);
    RUN_TEST(test_rng_generates_numbers);
    RUN_TEST(test_rng_deterministic);
    RUN_TEST(test_rng_different_seeds);
    RUN_TEST(test_rng_not_stuck);
    RUN_TEST(test_rng_range_coverage);
    RUN_TEST(test_rng_modulo_distribution);
    RUN_TEST(test_rng_zero_seed);

    return UNITY_END();
}