#ifndef PATIKA_COMMANDS_GUARD_H
#define PATIKA_COMMANDS_GUARD_H

#include "../types.h"
#include <stdint.h>

#define PATIKA_MAX_GUARD_TILES_PER_COMMAND 16

/**
 * @brief Payload for batch guard tile commands
 * Used by both agents and barracks
 */
typedef struct {
    union {
        AgentID agent_id;
        BuildingID barrack_id;
    } target;

    uint8_t is_barrack;  // 0 = agent, 1 = barrack

    int32_t tiles_q[PATIKA_MAX_GUARD_TILES_PER_COMMAND];
    int32_t tiles_r[PATIKA_MAX_GUARD_TILES_PER_COMMAND];
    uint8_t tile_count;
} AddGuardTilesPayload;

#endif
