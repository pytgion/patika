#ifndef PATIKA_COMMANDS_BARRACK_H
#define PATIKA_COMMANDS_BARRACK_H

#include "../types.h"
#include <stdint.h>

/**
 * @brief Payload for CMD_ADD_BARRACK
 */
typedef struct {
    int32_t pos_q, pos_r;
    uint8_t faction;
    uint8_t side;
    uint8_t behavior;
    uint8_t patrol_radius;
    uint16_t max_agents;
    BuildingID *out_barrack_id;
} AddBarrackPayload;

#endif
