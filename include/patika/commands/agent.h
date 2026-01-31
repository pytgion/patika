#ifndef PATIKA_COMMANDS_AGENT_H
#define PATIKA_COMMANDS_AGENT_H

#include "../types.h"
#include "../enums.h"

/**
 * @brief Payload for CMD_ADD_AGENT
 */

typedef struct {
    uint8_t layer;
    uint8_t collision_mask;
    uint8_t aggression_mask;
} PatikaCollisionData;

typedef struct {
    uint8_t type;
    union {
        struct {
            int32_t pos_q;
            int32_t pos_r;
        } location;
        struct {
            AgentID target_id;
        } agent;
        struct {
            BuildingID building;
        } entity;
    } data;
} AgentInteraction;

typedef struct {
    int32_t start_q, start_r;
    uint8_t faction;
    uint8_t side;
    BuildingID parent_barrack;
    AgentID *out_agent_id;
    PatikaCollisionData collision_data;
} AddAgentPayload;

/**
 * @brief Payload for CMD_ADD_AGENT_WITH_BEHAVIOR
 */

typedef struct {
    int32_t start_q, start_r;
    uint8_t faction;
    uint8_t side;
    BuildingID parent_barrack;
    PatikaCollisionData collision_data;
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
