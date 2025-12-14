#ifndef PATIKA_COMMANDS_AGENT_H
#define PATIKA_COMMANDS_AGENT_H

#include "../types.h"
#include "../enums.h"

/**
 * @brief Payload for CMD_ADD_AGENT
 */

// WARNING: do not use, not supported, use with behavior instead
typedef struct {
    int32_t start_q, start_r;
    uint8_t faction;
    uint8_t side;
    BarrackID parent_barrack;
    AgentID *out_agent_id;
} AddAgentPayload;

/**
 * @brief Payload for CMD_ADD_AGENT_WITH_BEHAVIOR
 */

typedef struct {
    int32_t start_q, start_r;
    uint8_t faction;
    uint8_t side;
    BarrackID parent_barrack;
    AgentBehavior initial_behavior;

    union {
        struct {
            int32_t center_q, center_r;
            int32_t radius;
        } patrol;

        struct {
            int32_t mode;
        } explore;

        struct {
            int32_t guard_q, guard_r;
            int32_t radius;
        } guard;
    } behavior_params;

    AgentID *out_agent_id;
} AddAgentWithBehaviorPayload;

#endif
