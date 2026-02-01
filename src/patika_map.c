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
//        int tile_count = (3 * radius * radius) + (3 * radius) + 1;
        /* we use diameter for simplicity, a squared area will be allocated in the heap, however edge cases must be handleded (literally, edge tiles must be observed)*/
        int tile_count = (2 * radius + 1) * (2 * radius + 1);
        map->tiles = calloc(tile_count, sizeof(MapTile));
        for (int i = 0; i < tile_count; i++)
        {
            map->tiles[i].state = 0; // default walkable
            map->tiles[i].occupancy = 0;
            map->tiles[i].sectorID = 0;
        }
        map->agent_grid = calloc(tile_count, sizeof(AgentID)); // TODO: for beta there is only one agent per tile, it will be changed
        for (int i = 0; i < tile_count; i++)
        {
            map->agent_grid[i] = PATIKA_INVALID_AGENT_ID; //means empty
        }
        // Store dimensions as diameter for indexing
        map->width = (radius * 2) + 1;
        map->height = (radius * 2) + 1;
    }
    else if (type == MAP_TYPE_RECTANGULAR)
    {
        int tile_count = (int)(width * height);
        map->tiles = calloc((unsigned long)width * height, sizeof(MapTile));
        map->agent_grid = calloc((unsigned long)width * height, sizeof(AgentID)); // maybe
        for (uint32_t i = 0; i < width * height; i++)
        {
            map->tiles[i].state = 0; // default walkable
            map->tiles[i].occupancy = 0;
            map->tiles[i].sectorID = 0;
        }
        for (int i = 0; i < tile_count; i++)
        {
            map->agent_grid[i] = PATIKA_INVALID_AGENT_ID;
        }
    }
    else 
    {
        PATIKA_LOG_ERROR("Unknown map type %d in map_init", type);
        map->tiles = NULL;
        map->agent_grid = NULL;
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
    else if (map->type == MAP_TYPE_HEXAGONAL)
    {
        // Get radius from stored dimensions
        int radius = (map->width - 1) / 2;

        // Hexagonal constraint: |q + r| <= radius
        return abs(q) <= radius && abs(r) <= radius && abs(q + r) <= radius;
    }
    else 
    {
        PATIKA_LOG_ERROR("Unknown map type %d in map_in_bounds", map->type);
        return 0;
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
    else if (map->type == MAP_TYPE_HEXAGONAL)
    {
        // Offset coordinates to array space (0-based indexing)
        int radius = (map->width - 1) / 2;
        int offset_q = q + radius;
        int offset_r = r + radius;

        return &map->tiles[offset_r * map->width + offset_q];
    }
    else 
    {
        PATIKA_LOG_ERROR("Unknown map type %d in map_get", map->type);
        return NULL;
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

/**
 * @brief Get agent_grid value at hex coordinates
 */
uint32_t map_get_agent_grid(MapGrid *map, int32_t q, int32_t r)
{
    if (!map_in_bounds(map, q, r))
        return PATIKA_INVALID_AGENT_ID;

    if (map->type == MAP_TYPE_RECTANGULAR)
    {
        return map->agent_grid[(r * map->width) + q];
    }
    else if (map->type == MAP_TYPE_HEXAGONAL)
    {
        int radius = (map->width - 1) / 2;
        int offset_q = q + radius;
        int offset_r = r + radius;
        return map->agent_grid[offset_r * map->width + offset_q];
    }
    else 
    {
        PATIKA_LOG_ERROR("Unknown map type %d in map_get_agent_grid", map->type);
        return PATIKA_INVALID_AGENT_ID;
    }
}

/**
 * @brief Set agent_grid value at hex coordinates
 */
void map_set_agent_grid(MapGrid *map, int32_t q, int32_t r, uint32_t value)
{
    if (!map_in_bounds(map, q, r))
        return;

    if (map->type == MAP_TYPE_RECTANGULAR)
    {
        map->agent_grid[(r * map->width) + q] = value;
    }
    else // MAP_TYPE_HEXAGONAL
    {
        int radius = (map->width - 1) / 2;
        int offset_q = q + radius;
        int offset_r = r + radius;
        map->agent_grid[offset_r * map->width + offset_q] = value;
    }
}

