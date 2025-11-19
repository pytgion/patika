/**
 * @file test_map.c
 * @brief Unit tests for hexagonal and rectangular map grids
 */

#include "internal/patika_internal.h"
#include "unity.h"

static MapGrid hex_map;
static MapGrid rect_map;

void setUp(void)
{
    map_init(&hex_map, MAP_TYPE_HEXAGONAL, 3, 0); // radius 3
    map_init(&rect_map, MAP_TYPE_RECTANGULAR, 10, 10);
}

void tearDown(void)
{
    map_destroy(&hex_map);
    map_destroy(&rect_map);
}

// ============================================================================
// Hexagonal Map Tests
// ============================================================================

void test_hex_map_init(void)
{
    TEST_ASSERT_NOT_NULL(hex_map.tiles);
    TEST_ASSERT_EQUAL(MAP_TYPE_HEXAGONAL, hex_map.type);
    TEST_ASSERT_EQUAL_UINT32(7, hex_map.width); // diameter = 2*radius + 1
    TEST_ASSERT_EQUAL_UINT32(7, hex_map.height);
}

void test_hex_map_center_in_bounds(void)
{
    TEST_ASSERT_TRUE(map_in_bounds(&hex_map, 0, 0));
}

void test_hex_map_edges_in_bounds(void)
{
    // Test cardinal directions at radius 3
    TEST_ASSERT_TRUE(map_in_bounds(&hex_map, 3, 0));  // right
    TEST_ASSERT_TRUE(map_in_bounds(&hex_map, -3, 0)); // left
    TEST_ASSERT_TRUE(map_in_bounds(&hex_map, 0, 3));  // down
    TEST_ASSERT_TRUE(map_in_bounds(&hex_map, 0, -3)); // up
    TEST_ASSERT_TRUE(map_in_bounds(&hex_map, 3, -3)); // diagonal
    TEST_ASSERT_TRUE(map_in_bounds(&hex_map, -3, 3)); // diagonal
}

void test_hex_map_outside_bounds(void)
{
    TEST_ASSERT_FALSE(map_in_bounds(&hex_map, 4, 0));
    TEST_ASSERT_FALSE(map_in_bounds(&hex_map, 0, 4));
    TEST_ASSERT_FALSE(map_in_bounds(&hex_map, 2, 2)); // q + r > radius
}

void test_hex_map_get_valid_tile(void)
{
    MapTile *tile = map_get(&hex_map, 0, 0);
    TEST_ASSERT_NOT_NULL(tile);
    TEST_ASSERT_EQUAL_UINT8(0, tile->state);
}

void test_hex_map_get_out_of_bounds(void)
{
    MapTile *tile = map_get(&hex_map, 10, 10);
    TEST_ASSERT_NULL(tile);
}

void test_hex_map_set_tile_state(void)
{
    map_set_tile_state(&hex_map, 1, 1, 1);
    MapTile *tile = map_get(&hex_map, 1, 1);

    TEST_ASSERT_NOT_NULL(tile);
    TEST_ASSERT_EQUAL_UINT8(1, tile->state);
}

void test_hex_map_unique_tiles(void)
{
    // Set different states to different tiles
    map_set_tile_state(&hex_map, 0, 0, 1);
    map_set_tile_state(&hex_map, 1, 0, 2);
    map_set_tile_state(&hex_map, 0, 1, 3);

    TEST_ASSERT_EQUAL_UINT8(1, map_get(&hex_map, 0, 0)->state);
    TEST_ASSERT_EQUAL_UINT8(2, map_get(&hex_map, 1, 0)->state);
    TEST_ASSERT_EQUAL_UINT8(3, map_get(&hex_map, 0, 1)->state);
}

// ============================================================================
// Rectangular Map Tests
// ============================================================================

void test_rect_map_init(void)
{
    TEST_ASSERT_NOT_NULL(rect_map.tiles);
    TEST_ASSERT_EQUAL(MAP_TYPE_RECTANGULAR, rect_map.type);
    TEST_ASSERT_EQUAL_UINT32(10, rect_map.width);
    TEST_ASSERT_EQUAL_UINT32(10, rect_map.height);
}

void test_rect_map_corners_in_bounds(void)
{
    TEST_ASSERT_TRUE(map_in_bounds(&rect_map, 0, 0));
    TEST_ASSERT_TRUE(map_in_bounds(&rect_map, 9, 0));
    TEST_ASSERT_TRUE(map_in_bounds(&rect_map, 0, 9));
    TEST_ASSERT_TRUE(map_in_bounds(&rect_map, 9, 9));
}

void test_rect_map_negative_out_of_bounds(void)
{
    TEST_ASSERT_FALSE(map_in_bounds(&rect_map, -1, 0));
    TEST_ASSERT_FALSE(map_in_bounds(&rect_map, 0, -1));
}

void test_rect_map_exceeds_dimensions(void)
{
    TEST_ASSERT_FALSE(map_in_bounds(&rect_map, 10, 0));
    TEST_ASSERT_FALSE(map_in_bounds(&rect_map, 0, 10));
    TEST_ASSERT_FALSE(map_in_bounds(&rect_map, 10, 10));
}

void test_rect_map_get_and_set(void)
{
    map_set_tile_state(&rect_map, 5, 5, 42);
    MapTile *tile = map_get(&rect_map, 5, 5);

    TEST_ASSERT_NOT_NULL(tile);
    TEST_ASSERT_EQUAL_UINT8(42, tile->state);
}

void test_rect_map_indexing(void)
{
    // Test that different coordinates map to different tiles
    for (int r = 0; r < 5; r++)
    {
        for (int q = 0; q < 5; q++)
        {
            uint8_t value = (uint8_t)(r * 5 + q);
            map_set_tile_state(&rect_map, q, r, value);
        }
    }

    // Verify
    for (int r = 0; r < 5; r++)
    {
        for (int q = 0; q < 5; q++)
        {
            uint8_t expected = (uint8_t)(r * 5 + q);
            TEST_ASSERT_EQUAL_UINT8(expected, map_get(&rect_map, q, r)->state);
        }
    }
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void test_map_set_out_of_bounds_no_crash(void)
{
    // Should not crash, just silently fail
    map_set_tile_state(&hex_map, 100, 100, 1);
    map_set_tile_state(&rect_map, 100, 100, 1);
    // If we get here, test passes
    TEST_PASS();
}

void test_map_occupancy_tracking(void)
{
    MapTile *tile = map_get(&hex_map, 0, 0);
    TEST_ASSERT_NOT_NULL(tile);

    tile->occupancy = 5;
    TEST_ASSERT_EQUAL_UINT8(5, map_get(&hex_map, 0, 0)->occupancy);
}

int main(void)
{
    UNITY_BEGIN();

    // Hexagonal tests
    RUN_TEST(test_hex_map_init);
    RUN_TEST(test_hex_map_center_in_bounds);
    RUN_TEST(test_hex_map_edges_in_bounds);
    RUN_TEST(test_hex_map_outside_bounds);
    RUN_TEST(test_hex_map_get_valid_tile);
    RUN_TEST(test_hex_map_get_out_of_bounds);
    RUN_TEST(test_hex_map_set_tile_state);
    RUN_TEST(test_hex_map_unique_tiles);

    // Rectangular tests
    RUN_TEST(test_rect_map_init);
    RUN_TEST(test_rect_map_corners_in_bounds);
    RUN_TEST(test_rect_map_negative_out_of_bounds);
    RUN_TEST(test_rect_map_exceeds_dimensions);
    RUN_TEST(test_rect_map_get_and_set);
    RUN_TEST(test_rect_map_indexing);

    // Edge cases
    RUN_TEST(test_map_set_out_of_bounds_no_crash);
    RUN_TEST(test_map_occupancy_tracking);

    return UNITY_END();
}
