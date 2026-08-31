#ifndef PATIKA_COMMANDS_BARRACK_H
#define PATIKA_COMMANDS_BARRACK_H

#include "../types.h"
#include "../enums.h"
#include <stdint.h>

typedef struct {
    int32_t pos_q, pos_r;
    uint8_t group;
    uint8_t team;
    uint8_t behavior;
    uint8_t patrol_radius;
    uint16_t max_agents;
    BuildingID *out_barrack_id;
} AddBarrackPayload;

#endif
