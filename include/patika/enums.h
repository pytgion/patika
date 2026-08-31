#ifndef PATIKA_ENUMS_H
#define PATIKA_ENUMS_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum
    {
        PATIKA_OK = 0,
        PATIKA_ERR_QUEUE_FULL = 1,
        PATIKA_ERR_INVALID_ID = 2,
        PATIKA_ERR_OUT_OF_BOUNDS = 3,
        PATIKA_ERR_CAPACITY = 4,
        PATIKA_ERR_BUSY = 5,
        PATIKA_ERR_NULL_HANDLE = 6,
        PATIKA_ERR_INVALID_COMMAND_TYPE = 7
    } PatikaError;

    typedef enum
    {
        /* Agent lifecycle */
        CMD_ADD_AGENT              = 0,
        CMD_ADD_AGENT_WITH_BEHAVIOR = 1,
        CMD_REMOVE_AGENT           = 2,

        /* Agent control */
        CMD_SET_GOAL               = 3,
        CMD_SET_BEHAVIOR           = 4,

        /* Barrack lifecycle */
        CMD_ADD_BARRACK            = 11,
        CMD_REMOVE_BARRACK         = 12,

        /* Map */
        CMD_SET_TILE_STATE         = 19,
    } CommandType;

    typedef enum
    {
        EVENT_REACHED_GOAL  = 0,
        EVENT_STUCK         = 1,
        EVENT_BLOCKED       = 2,
        EVENT_REPLAN_NEEDED = 3,
        EVENT_AGENT_REMOVED = 4
    } EventType;

    typedef enum
    {
        MAP_TYPE_HEXAGONAL  = 0,
        MAP_TYPE_RECTANGULAR = 1
    } GridType;

    typedef enum
    {
        BEHAVIOR_IDLE    = 0,
        BEHAVIOR_PATROL  = 1,
        BEHAVIOR_EXPLORE = 2,
    } AgentBehavior;

    typedef enum
    {
        STATE_IDLE         = 0,
        STATE_CALCULATING  = 1,  /* running per-agent A* (legacy / no sector system) */
        STATE_MOVING       = 2,  /* moving one tile (per-agent A* path)              */
        STATE_INTERACTING  = 3,
        STATE_REMOVE_QUEUE = 4,
    } AgentState;

#ifdef __cplusplus
}
#endif

#endif /* PATIKA_ENUMS_H */
