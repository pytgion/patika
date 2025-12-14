#ifndef PATIKA_ENUMS_H
#define PATIKA_ENUMS_H

#ifdef __cplusplus
extern "C" {
    #endif

    /* ============================================================================
     * Error Codes
     * ========================================================================= */

    /**
     * @brief Error codes returned by public API functions
     */
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

    /* ============================================================================
     * Command & Event Types
     * ========================================================================= */

    /**
     * @brief Command types accepted by the simulation
     */
    // patika/enums.h
    typedef enum
    {
        // Agent lifecycle
        CMD_ADD_AGENT = 0,
        CMD_ADD_AGENT_WITH_BEHAVIOR = 1,
        CMD_REMOVE_AGENT = 2,

        // Agent control
        CMD_SET_GOAL = 3,
        CMD_SET_BEHAVIOR = 4,
        CMD_COMPUTE_NEXT = 5,
        CMD_BIND_BARRACK = 6,

        // Agent guards
        CMD_AGENT_ADD_GUARD_TILE = 7,
        CMD_AGENT_ADD_GUARD_TILES = 8,
        CMD_AGENT_REMOVE_GUARD_TILE = 9,
        CMD_AGENT_CLEAR_GUARD_TILES = 10,

        // Barrack lifecycle
        CMD_ADD_BARRACK = 11,
        CMD_REMOVE_BARRACK = 12,

        // Barrack guards
        CMD_BARRACK_ADD_GUARD_TILE = 13,
        CMD_BARRACK_ADD_GUARD_TILES = 14,
        CMD_BARRACK_REMOVE_GUARD_TILE = 15,
        CMD_BARRACK_CLEAR_GUARD_TILES = 16,

        // Building
        CMD_ADD_BUILDING = 17,
        CMD_REMOVE_BUILDING = 18,

        // Map
        CMD_SET_TILE_STATE = 19,

        // Debug
        CMD_DEBUG_DUMP_STATE = 20,

    } CommandType;

    /**
     * @brief Event types emitted by the simulation
     */
    typedef enum
    {
        EVENT_REACHED_GOAL = 0,
        EVENT_STUCK = 1,
        EVENT_BLOCKED = 2,
        EVENT_REPLAN_NEEDED = 3,
        EVENT_AGENT_REMOVED = 4
    } EventType;

    /* ============================================================================
     * Map & Building Types
     * ========================================================================= */

    /**
     * @brief Map type identifiers
     */
    typedef enum
    {
        MAP_TYPE_HEXAGONAL = 0,
        MAP_TYPE_RECTANGULAR = 1
    } GridType;

    /**
     * @brief Building type identifiers
     */
    typedef enum
    {
        BUILDING_TOWER = 0,
        BUILDING_BARRACK = 1,
        BUILDING_IMMUNITY = 2,
        BUILDING_WALL = 3,
        BUILDING_TRAP = 4
    } BuildingType;

    /* ============================================================================
     * Agent Behavior & State
     * ========================================================================= */

    /**
     * @brief High-level agent behavior (what is the agent doing?)
     */
    typedef enum
    {
        BEHAVIOR_IDLE = 0,
        BEHAVIOR_PATROL = 1,
        BEHAVIOR_EXPLORE = 2,
        BEHAVIOR_GUARD = 3,
        BEHAVIOR_FLEE = 4,
    } AgentBehavior;

    /**
     * @brief Low-level agent state (how is the agent doing it?)
     */
    typedef enum
    {
        STATE_IDLE = 0,
        STATE_CALCULATING = 1,
        STATE_MOVING = 2,
        STATE_INTERACTING = 3,
        STATE_REMOVE_QUEUE = 4
    } AgentState;

    // TODO: Exploration mode
    #ifdef __cplusplus
}
#endif

#endif /* PATIKA_ENUMS_H */
