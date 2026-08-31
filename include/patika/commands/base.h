#ifndef PATIKA_COMMANDS_BASE_H
#define PATIKA_COMMANDS_BASE_H

#include "../types.h"
#include "../enums.h"
#include "agent.h"
#include "barrack.h"

#define PATIKA_INLINE_COMMAND_SIZE 56

typedef struct PatikaCommand
{
    CommandType type;

    union {
        uint8_t inline_data[PATIKA_INLINE_COMMAND_SIZE];

        struct {
            AgentID agent_id;
        } remove_agent;

        struct {
            AgentID agent_id;
            int32_t goal_q, goal_r;
        } set_goal;

        struct {
            AgentID    agent_id;
            AgentBehavior behavior;
        } set_behavior;

        struct {
            int32_t q, r;
            uint8_t state;
        } set_tile;

        struct {
            BuildingID barrack_id;
        } remove_barrack;

        AddAgentPayload             add_agent;
        AddAgentWithBehaviorPayload add_agent_with_behavior;
        AddBarrackPayload           add_barrack;
    };
} PatikaCommand;

#endif
