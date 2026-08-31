#ifndef PATIKA_FLOWFIELD_H
#define PATIKA_FLOWFIELD_H

#include "types.h"
#include "enums.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * PatikaFlowField — BFS-computed direction grid for crowd navigation.
 *
 * Computed once per goal tile; shared by all agents heading to the same
 * destination.  cost[tile] is the BFS distance to the goal (UINT16_MAX =
 * unreachable).  dir[tile] is the direction index (into the grid's direction
 * table) that steps toward the goal, or 0xFF if the tile is the goal itself
 * or is unreachable.
 */
typedef struct {
    uint16_t *cost;           /* BFS distance per tile to goal */
    uint8_t  *dir;            /* direction index per tile, 0xFF = at goal/stuck */
    int32_t   goal_q, goal_r;
    uint32_t  width, height;
    GridType  grid_type;
} PatikaFlowField;

#ifdef __cplusplus
}
#endif

#endif /* PATIKA_FLOWFIELD_H */
