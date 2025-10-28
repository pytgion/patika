#include "internal/patika_internal.h"
#include <stdlib.h>

void map_init(MapGrid* map, uint32_t width, uint32_t height) {
    map->width = width;
    map->height = height;
    map->tiles = calloc(width * height, sizeof(MapTile));
}

void map_destroy(MapGrid* map) {
    free(map->tiles);
}

int map_in_bounds(MapGrid* map, int32_t q, int32_t r) {
    return q >= 0 && q < (int32_t)map->width && 
           r >= 0 && r < (int32_t)map->height;
}

MapTile* map_get(MapGrid* map, int32_t q, int32_t r) {
    if (!map_in_bounds(map, q, r)) return NULL;
    return &map->tiles[r * map->width + q];
}
