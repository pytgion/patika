#include "internal/patika_internal.h"
#include <stdlib.h>

void map_init(MapGrid *map, uint8_t type, uint32_t width, uint32_t height)
{
    map->type = type;
    map->width = width;
    map->height = height;
    map->tiles = calloc(width * height, sizeof(MapTile));
}

void map_destroy(MapGrid *map)
{
    free(map->tiles);
}

int map_in_bounds(MapGrid *map, int32_t q, int32_t r)
{
    if (map->type == MAP_TYPE_RECTANGULAR)
    {
        return q >= 0 && q < (int32_t)map->width && r >= 0 && r < (int32_t)map->height;
    }
    else
    {
        // for hexagonal maps, since they are not diamond shaped, we use width as radius.
        int internal_q = q + map->width;
        int internal_r = r + map->width;
        return internal_q >= 0 && internal_q < (int32_t)map->width && internal_r >= 0 &&
               internal_r < (int32_t)map->height;
    }
}

MapTile *map_get(MapGrid *map, int32_t q, int32_t r)
{
    if (!map_in_bounds(map, q, r))
        return NULL;
    return &map->tiles[r * map->width + q];
}

void map_set_tile_state(MapGrid *map, int32_t q, int32_t r, uint8_t state)
{
    MapTile *tile = map_get(map, q, r);
    if (tile)
    {
        tile->state = state;
    }
}