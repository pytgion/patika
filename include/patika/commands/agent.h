#ifndef PATIKA_COMMANDS_AGENT_H
#define PATIKA_COMMANDS_AGENT_H

#include "../types.h"
#include "../enums.h"

typedef struct {
    uint8_t layer;
    uint8_t collision_mask;
} PatikaCollisionData;

typedef struct {
    int32_t start_q, start_r;
    uint8_t group;
    uint8_t team;
    BuildingID parent_barrack;
    AgentID *out_agent_id;
    PatikaCollisionData collision_data;
} AddAgentPayload;

typedef struct {
    int32_t start_q, start_r;
    uint8_t group;
    uint8_t team;
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
    } behavior_params;

    AgentID *out_agent_id;
} AddAgentWithBehaviorPayload;

#endif
