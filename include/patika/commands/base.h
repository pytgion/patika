#ifndef PATIKA_COMMANDS_BASE_H
#define PATIKA_COMMANDS_BASE_H

#include "../types.h"
#include "../enums.h"

// inline command size threshold (32 bytes)
#define PATIKA_INLINE_COMMAND_SIZE 32

/**
 * @brief Base command structure
 * @details Commands ≤32 bytes use inline storage, larger ones use payload pointer
 */
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
            AgentID agent_id;
            int32_t tile_q, tile_r;
        } add_guard_tile;

        struct {
            BuildingID barrack_id;
            int32_t tile_q, tile_r;
        } add_barrack_guard_tile;

        struct {
            int32_t q, r;
            uint8_t state;
        } set_tile;

        struct {
            AgentID agent_id;
            BuildingID barrack_id;
        } bind_barrack;

        struct {
            AgentID agent_id;
        } compute_next;

        struct {
            BuildingID barrack_id;
        } clear_barrack_guard_tiles;

        struct {
            void *payload;
        } large_command;
    };
} PatikaCommand;

#endif
