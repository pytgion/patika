#ifndef PATIKA_SNAPSHOT_H
#define PATIKA_SNAPSHOT_H

#include "types.h"
#include "enums.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct
    {
        AgentID       id;
        AgentBehavior behavior;
        AgentState    state;
        uint8_t       group;
        uint8_t       team;
        BuildingID    parent_barrack;
        int32_t       pos_q, pos_r;
        int32_t       next_q, next_r;
        int32_t       target_q, target_r;
    } AgentSnapshot;

    typedef struct
    {
        BuildingID id;
        uint8_t    group;
        uint8_t    team;
        uint8_t    state;
        int32_t    pos_q, pos_r;
        uint8_t    patrol_radius;
        uint16_t   agent_count;
    } BarrackSnapshot;

    typedef struct
    {
        AgentSnapshot   *agents;
        uint32_t         agent_count;
        BarrackSnapshot *barracks;
        uint16_t         barrack_count;
        uint64_t         version;
    } PatikaSnapshot;

    typedef struct
    {
        uint64_t total_ticks;
        uint64_t commands_processed;
        uint64_t events_emitted;
        uint64_t blocked_moves;
        uint64_t replan_count;
        uint32_t active_agents;
        uint32_t active_barracks;
    } PatikaStats;

#ifdef __cplusplus
}
#endif

#endif /* PATIKA_SNAPSHOT_H */
