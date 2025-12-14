#include "internal/patika_internal.h"
#include <stddef.h>
#include <stdlib.h>

void map_init(MapGrid *map, uint8_t type, uint32_t width, uint32_t height)
{
    map->type = type;
    map->width = width;
    map->height = height;

    if (type == MAP_TYPE_HEXAGONAL)
    {
        // For hexagonal maps, width represents the radius
        // Total tiles = 3*r^2 + 3*r + 1
        int radius = (int)width;
        int tile_count = (3 * radius * radius) + (3 * radius) + 1;
        map->tiles = calloc(tile_count, sizeof(MapTile));
        map->agent_grid = calloc(tile_count, sizeof(AgentID)); // TODO: for beta there will be only one agent per tile, it will be changed
        for (int i = 0; i < tile_count; i++)
        {
            map->agent_grid[i] = PATIKA_INVALID_AGENT_ID; //means empty
        }
        // Store dimensions as diameter for indexing
        map->width = (radius * 2) + 1;
        map->height = (radius * 2) + 1;
    }
    else
    {
        map->tiles = calloc((unsigned long)width * height, sizeof(MapTile));
        map->agent_grid = calloc((unsigned long)width * height, sizeof(AgentID)); // maybe
    }
}

void map_destroy(MapGrid *map)
{
    free(map->tiles);
    free(map->agent_grid);
}

int map_in_bounds(MapGrid *map, int32_t q, int32_t r)
{
    if (map->type == MAP_TYPE_RECTANGULAR)
    {
        return q >= 0 && q < (int32_t)map->width && r >= 0 && r < (int32_t)map->height;
    }
    else // MAP_TYPE_HEXAGONAL
    {
        // Get radius from stored dimensions
        int radius = (map->width - 1) / 2;

        // Hexagonal constraint: |q + r| <= radius
        return abs(q) <= radius && abs(r) <= radius && abs(q + r) <= radius;
    }
}

MapTile *map_get(MapGrid *map, int32_t q, int32_t r)
{
    if (!map_in_bounds(map, q, r))
        return NULL;

    if (map->type == MAP_TYPE_RECTANGULAR)
    {
        return &map->tiles[(r * map->width) + q];
    }
    else // MAP_TYPE_HEXAGONAL
    {
        // Offset coordinates to array space (0-based indexing)
        int radius = (map->width - 1) / 2;
        int offset_q = q + radius;
        int offset_r = r + radius;

        return &map->tiles[offset_r * map->width + offset_q];
    }
}

void map_set_tile_state(MapGrid *map, int32_t q, int32_t r, uint8_t state)
{
    MapTile *tile = map_get(map, q, r);
    if (tile)
    {
        tile->state = state;
    }
}
