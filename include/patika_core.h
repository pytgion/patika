#ifndef PATIKA_TYPES_H
#define PATIKA_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
// Windows: Use __declspec for DLL export/import
#ifdef PATIKA_EXPORTS // ← CMake will define this when building the library
#define PATIKA_API __declspec(dllexport)
#else
#define PATIKA_API __declspec(dllimport)
#endif
#else
// Unix/Linux/macOS: Use visibility attribute (GCC/Clang)
#if defined(__GNUC__) || defined(__clang__)
#ifdef PATIKA_EXPORTS
#define PATIKA_API __attribute__((visibility("default")))
#else
#define PATIKA_API
#endif
#else
#define PATIKA_API
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    typedef uint32_t AgentID;
    typedef uint16_t BarrackID;

extern const uint32_t PATIKA_INVALID_AGENT_ID;
extern const uint16_t PATIKA_INVALID_BARRACK_ID;

    /**
 * @file patika_core.h
 * @brief Public types and data structures, and the API end for the Patika lib.
 *
 * @details These types describe configuration, command/event payloads,
 * runtime snapshots, and the opaque handle used to operate the simulation.
 */

    /* Include constants for AgentID/BarrackID definitions */

    /** @brief Opaque simulation context handle. */
    typedef struct PatikaContext *PatikaHandle;

    /** @addtogroup PatikaEnums
 *  @{
 */

    /**
 * @brief Error codes returned by public API functions.
 * @enum PatikaError
 */
    typedef enum
    {
        PATIKA_OK = 0,                /**< Operation succeeded or was queued. */
        PATIKA_ERR_QUEUE_FULL = 1,    /**< Command queue is full; producer must retry. */
        PATIKA_ERR_INVALID_ID = 2,    /**< Invalid or stale ID (generation mismatch). */
        PATIKA_ERR_OUT_OF_BOUNDS = 3, /**< Out-of-range coordinates or indices. */
        PATIKA_ERR_CAPACITY = 4,      /**< Pool capacity exceeded (agents/barracks). */
        PATIKA_ERR_BUSY = 5,          /**< Resource is busy (transient). */
        PATIKA_ERR_NULL_HANDLE = 6    /**< Null @ref PatikaHandle supplied. */
    } PatikaError;

    /**
 * @brief Command types accepted by the simulation.
 * @enum CommandType
 * @note Commands are produced by multiple threads and consumed by a single
 * simulation thread via an MPSC queue.
 */
    typedef enum
    {
        CMD_ADD_AGENT = 0,          /**< Allocate a new agent at a start cell. */
        CMD_REMOVE_AGENT = 1,       /**< Remove an agent from the pool. */
        CMD_SET_GOAL = 2,           /**< Set target cell for an agent. */
        CMD_BIND_BARRACK = 3,       /**< Attach an agent to a barrack. */
        CMD_SET_TILE_STATE = 4,     /**< Update map cell state (e.g., open/closed). */
        CMD_COMPUTE_NEXT = 5,       /**< Request next-step computation for an agent. */
        CMD_BATCH_COMPUTE_NEXT = 6, /**< Placeholder for batched next-step compute. */
        CMD_ADD_BARRACK = 7         /**< Create a new barrack at a cell. */
    } CommandType;

    /**
 * @brief Event types emitted by the simulation.
 * @enum EventType
 * @note Events are published by the simulation and consumed by exactly one
 * thread via an SPSC queue.
 */
    typedef enum
    {
        EVENT_REACHED_GOAL = 0,  /**< Agent reached its target. */
        EVENT_STUCK = 1,         /**< Agent is stuck; no valid neighbor step. */
        EVENT_BLOCKED = 2,       /**< Move was blocked by map state or occupancy. */
        EVENT_REPLAN_NEEDED = 3, /**< A replan was requested by local rules. */
        EVENT_AGENT_REMOVED = 4  /**< Agent was removed. */
    } EventType;

    /** @} */ /* end of PatikaEnums */

    /**
     * @brief Map type identifiers.
     * @enum GridType
     * @note Used in @ref PatikaConfig to select the map implementation.
     * @note MAP_TYPE_RECTANGULAR is still generates hexagonal grid, only uses wiodth/height to generate diamond shaped map.
     */
    typedef enum
    {
        MAP_TYPE_HEXAGONAL = 0,  /**< Hexagonal grid map. */
        MAP_TYPE_RECTANGULAR = 1 /**< Rectangular grid map (reserved). */
    } GridType;

    /* -----------------------------------------------------------
 * 2. DATA STRUCTURES (PATIKA_STRUCTS GROUP)
 * ----------------------------------------------------------- */
    /** @defgroup PatikaStructs Core Data Structures
 * @brief Configuration, command payloads, and runtime snapshots.
 * @{
 */

    /**
 * @brief Engine configuration at creation time.
 * @details Capacities are fixed after @ref patika_create . Queue sizes define
 * backpressure behavior between producers and the simulation thread.
 */
    typedef struct
    {
        uint8_t grid_type;           /**< Map type identifier (implementation-defined). */
        uint32_t max_agents;         /**< Agent pool capacity. */
        uint16_t max_barracks;       /**< Barrack pool capacity. */
        uint32_t grid_width;         /**< Map width in cells (q axis). */
        uint32_t grid_height;        /**< Map height in cells (r axis). */
        uint32_t sector_size;        /**< Optional sector side length in cells. */
        uint32_t command_queue_size; /**< MPSC command queue capacity (in commands). */
        uint32_t event_queue_size;   /**< SPSC event queue capacity (in events). */
        uint64_t rng_seed;           /**< Seed for the internal PCG32 generator. */
    } PatikaConfig;

    /**
 * @brief Command payload.
 * @details The active union member is selected by @ref CommandType in @ref
 * type. All coordinates are axial hex (q, r). IDs are opaque handles.
 */
    typedef struct PatikaCommand
    {
        CommandType type; /**< Discriminator selecting the union member. */
        union
        {
            /**
     * @brief Payload for @ref CMD_ADD_AGENT .
     */
            struct
            {
                int32_t start_q, start_r; /**< Spawn cell. */
                uint8_t faction;          /**< Faction identifier. */
                uint8_t side;             /**< Team/side identifier. */
                BarrackID parent_barrack; /**< Optional owning barrack. */
                AgentID *out_agent_id;    /**< Optional: set on processing; must remain valid
                                until after @ref patika_tick . */
            } add_agent;

            /** @brief Payload for @ref CMD_REMOVE_AGENT . */
            struct
            {
                AgentID agent_id;
            } remove_agent;

            /** @brief Payload for @ref CMD_SET_GOAL . */
            struct
            {
                AgentID agent_id;
                int32_t goal_q, goal_r;
            } set_goal;

            /** @brief Payload for @ref CMD_BIND_BARRACK . */
            struct
            {
                AgentID agent_id;
                BarrackID barrack_id;
            } bind_barrack;

            /** @brief Payload for @ref CMD_SET_TILE_STATE . */
            struct
            {
                int32_t q, r;
                uint8_t state;
            } set_tile;

            /** @brief Payload for @ref CMD_COMPUTE_NEXT . */
            struct
            {
                AgentID agent_id;
            } compute_next;

            /** @brief Payload for @ref CMD_BATCH_COMPUTE_NEXT (reserved). */
            struct
            {
                uint32_t dummy;
            } batch_compute;

            /**
     * @brief Payload for @ref CMD_ADD_BARRACK .
     */
            struct
            {
                int32_t pos_q, pos_r;      /**< Barrack cell. */
                uint8_t faction;           /**< Faction identifier. */
                uint8_t side;              /**< Team/side identifier. */
                uint8_t patrol_radius;     /**< Optional local patrol radius. */
                uint16_t max_agents;       /**< Max agents managed by this barrack. */
                BarrackID *out_barrack_id; /**< Optional: set on processing; must remain
                                    valid until after @ref patika_tick . */
            } add_barrack;
        };
    } PatikaCommand;

    /**
 * @brief Event payload emitted by the simulation.
 */
    typedef struct
    {
        EventType type;       /**< Event kind. */
        AgentID agent_id;     /**< Related agent, if any. */
        int32_t pos_q, pos_r; /**< Cell associated with the event. */
    } PatikaEvent;

    /**
 * @brief Per-agent snapshot data.
 * @details Snapshot arrays are rebuilt during @ref patika_tick and double
 * buffered; the pointer returned by @ref patika_get_snapshot remains valid
 * until the next tick flips the buffer.
 */
    typedef struct
    {
        AgentID id;                 /**< Stable agent ID (with generation). */
        uint8_t state;              /**< Implementation-defined agent state. */
        uint8_t faction;            /**< Faction identifier. */
        uint8_t side;               /**< Team/side identifier. */
        BarrackID parent_barrack;   /**< Owning barrack ID or invalid. */
        int32_t pos_q, pos_r;       /**< Current cell. */
        int32_t next_q, next_r;     /**< Next chosen cell (if any). */
        int32_t target_q, target_r; /**< Current goal cell. */
    } AgentSnapshot;

    /**
 * @brief Per-barrack snapshot data.
 */
    typedef struct
    {
        BarrackID id;          /**< Barrack ID. */
        uint8_t faction;       /**< Faction identifier. */
        uint8_t side;          /**< Team/side identifier. */
        uint8_t state;         /**< Implementation-defined barrack state. */
        int32_t pos_q, pos_r;  /**< Cell position. */
        uint8_t patrol_radius; /**< Patrol radius hint. */
        uint16_t agent_count;  /**< Number of active agents bound. */
    } BarrackSnapshot;

    /**
 * @brief Consistent snapshot view of the world.
 * @details Double-buffered. Readers should treat returned pointers as
 * read-only and copy data if they need to keep it after the next tick.
 */
    typedef struct
    {
        AgentSnapshot *agents;     /**< Array of @ref AgentSnapshot with @ref agent_count items. */
        uint32_t agent_count;      /**< Number of valid entries in @ref agents. */
        BarrackSnapshot *barracks; /**< Array of @ref BarrackSnapshot with @ref
                                barrack_count items. */
        uint16_t barrack_count;    /**< Number of valid entries in @ref barracks. */
        uint64_t version;          /**< Monotonic snapshot sequence number. */
    } PatikaSnapshot;

    /**
 * @brief Runtime statistics accumulated since creation.
 * @note Counters are incremented during @ref patika_tick .
 */
    typedef struct
    {
        uint64_t total_ticks;        /**< Number of ticks executed. */
        uint64_t commands_processed; /**< Commands consumed from the queue. */
        uint64_t events_emitted;     /**< Events pushed to the event queue. */
        uint64_t blocked_moves;      /**< Count of blocked move attempts. */
        uint64_t replan_count;       /**< Count of replan triggers. */
        uint32_t active_agents;      /**< Current number of active agents. */
        uint32_t active_barracks;    /**< Current number of active barracks. */
    } PatikaStats;

    /** @} */ /* PatikaStructs */

    PATIKA_API PatikaHandle patika_create(const PatikaConfig *config);

    PATIKA_API void patika_destroy(PatikaHandle handle);

    PATIKA_API PatikaError patika_submit_command(PatikaHandle handle, const PatikaCommand *cmd);

    PATIKA_API PatikaError patika_submit_commands(PatikaHandle handle, const PatikaCommand *cmds, uint32_t count);
    PATIKA_API void patika_tick(PatikaHandle handle);

    PATIKA_API uint32_t patika_poll_events(PatikaHandle handle, PatikaEvent *out_events, uint32_t max_events);

    PATIKA_API const PatikaSnapshot *patika_get_snapshot(PatikaHandle handle);

    PATIKA_API PatikaError patika_add_agent_sync(PatikaHandle handle,
                                                 int32_t start_q,
                                                 int32_t start_r,
                                                 uint8_t faction,
                                                 uint8_t side,
                                                 BarrackID parent_barrack,
                                                 AgentID *id_output);

#ifdef __cplusplus
}
#endif

#endif
