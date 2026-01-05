#ifndef PATIKA_INTERNAL_H
#define PATIKA_INTERNAL_H

#include "../../include/patika.h"
#include "../../include/patika/patika_log.h"
#include <stdatomic.h>
#include <stdint.h>

#ifndef INT32_MAX
#define INT32_MAX 0x7FFFFFFF
#endif

#define PATIKA_INVALID_AGENT_INDEX 0xFFFFu
#define PATIKA_AGENT_DEFAULT_VIEW_RADIUS 1

#define AGENT_GRID_RESERVED_BIT  0x80000000  // Bit 31
#define AGENT_GRID_OCCUPIED_BIT  0x40000000  // Bit 30 (future use)
#define AGENT_GRID_AGENT_MASK    0x0000FFFF  // Lower 16 bits

#define AGENT_PROGRESS_MAX_DISTANCE 10000

#define PATIKA_INTERNAL_LOG_DEBUG(fmt, ...) PATIKA_LOG_DEBUG("[CORE] " fmt, ##__VA_ARGS__)
#define PATIKA_INTERNAL_LOG_INFO(fmt, ...) PATIKA_LOG_INFO("[CORE] " fmt, ##__VA_ARGS__)
#define PATIKA_INTERNAL_LOG_WARN(fmt, ...) PATIKA_LOG_WARN("[CORE] " fmt, ##__VA_ARGS__)
#define PATIKA_INTERNAL_LOG_ERROR(fmt, ...) PATIKA_LOG_ERROR("[CORE] " fmt, ##__VA_ARGS__)

typedef struct MPSCCommandQueue MPSCCommandQueue;
typedef struct SPSCEventQueue SPSCEventQueue;
typedef struct AgentSlot AgentSlot;
typedef struct AgentPool AgentPool;
typedef struct BarrackSlot BarrackSlot;
typedef struct BarrackPool BarrackPool;
typedef struct MapTile MapTile;
typedef struct MapGrid MapGrid;
typedef struct PCG32 PCG32;

struct MPSCCommandQueue
{
    PatikaCommand *buffer;
    uint32_t capacity;
    _Atomic uint32_t head;
    uint32_t tail;
};

void mpsc_init(MPSCCommandQueue *q, uint32_t capacity);
void mpsc_destroy(MPSCCommandQueue *q);
PatikaError mpsc_push(MPSCCommandQueue *q, const PatikaCommand *cmd);
PatikaError mpsc_pop(MPSCCommandQueue *q, PatikaCommand *out);

struct SPSCEventQueue
{
    PatikaEvent *buffer;
    uint32_t capacity;
    _Atomic uint32_t head;
    _Atomic uint32_t tail;
};

void spsc_init(SPSCEventQueue *q, uint32_t capacity);
void spsc_destroy(SPSCEventQueue *q);
PatikaError spsc_push(SPSCEventQueue *q, const PatikaEvent *evt);
PatikaError spsc_pop(SPSCEventQueue *q, PatikaEvent *out);

typedef struct {
    int32_t center_q, center_r;
    int32_t radius;
    int32_t current_target_q, current_target_r;
    uint16_t waypoint_index;
    float idle_timer;
} PatrolData;

typedef struct {
    int32_t mode;
    uint32_t cells_visited;
    int32_t last_target_q, last_target_r;
} ExploreData;

typedef struct {
    int32_t *guard_tiles_q;
    int32_t *guard_tiles_r;
    uint16_t tile_count;
    uint16_t tile_capacity;
} GuardData;

// TODO: consider optimizing for cache friendly structure
struct AgentSlot
{
    int32_t pos_q, pos_r;
    int32_t next_q, next_r;
    int32_t target_q, target_r;
    AgentID id;
    BuildingID parent_barrack;
    AgentInteraction interaction_data;

    uint16_t generation;
    uint16_t progress; // 0-10000
    uint16_t view_radius;
    uint16_t speed; // tick based
    uint16_t next_free_index;

    PatikaCollisionData collision_data;
    uint8_t state;
    uint8_t behavior;
    uint8_t faction;
    uint8_t side;
    uint8_t active;

    union {
        PatrolData patrol;
        ExploreData explore;
        GuardData guard;
    } behavior_data;

};

struct AgentPool
{
    AgentSlot *slots;
    uint32_t capacity;
    uint16_t free_head;
    uint32_t active_count;
};



void agent_pool_init(AgentPool *pool, uint32_t capacity);
void agent_pool_destroy(AgentPool *pool);
AgentID agent_pool_allocate(AgentPool *pool);
void agent_pool_free(AgentPool *pool, AgentID id);
AgentSlot *agent_pool_get(AgentPool *pool, AgentID id);

static inline AgentID make_agent_id(uint16_t index, uint16_t gen)
{
    return ((uint32_t)gen << 16) | index;
}
static inline uint16_t agent_index(AgentID id)
{
    return id & 0xFFFF;
}
static inline uint16_t agent_generation(AgentID id)
{
    return id >> 16;
}

struct BarrackSlot
{
    BuildingID id;
    uint8_t active;
    uint8_t faction;
    uint8_t side;
    uint8_t state;
    AgentBehavior behavior;
    uint8_t patrol_radius;
    int32_t pos_q, pos_r;
    uint16_t max_agents;
    uint16_t agent_count;
    uint16_t first_agent_index;
};

struct BarrackPool
{
    BarrackSlot *slots;
    uint16_t capacity;
    uint16_t next_id;
};

void barrack_pool_init(BarrackPool *pool, uint16_t capacity);
void barrack_pool_destroy(BarrackPool *pool);
BuildingID barrack_pool_allocate(BarrackPool *pool);
BarrackSlot *barrack_pool_get(BarrackPool *pool, BuildingID id);

struct MapTile
{
    // TODO: implement sectorID
    uint8_t state;
    uint8_t occupancy;
    uint8_t *sub_pos; // represents 8 equal partition of a hexagon, think twice before use it
    uint16_t sectorID;
};

struct MapGrid
{
    GridType type;
    MapTile *tiles;
    uint32_t width;
    uint32_t height;
    AgentID *agent_grid;
};

void map_init(MapGrid *map, uint8_t type, uint32_t width, uint32_t height);
void map_destroy(MapGrid *map);
int map_in_bounds(MapGrid *map, int32_t q, int32_t r);
MapTile *map_get(MapGrid *map, int32_t q, int32_t r);
void map_set_tile_state(MapGrid *map, int32_t q, int32_t r, uint8_t state);

uint32_t map_get_agent_grid(MapGrid *map, int32_t q, int32_t r);
void map_set_agent_grid(MapGrid *map, int32_t q, int32_t r, uint32_t value);

static inline AgentID map_extract_agent_id(uint32_t grid_value) {
    return grid_value & AGENT_GRID_AGENT_MASK;
}

static inline int map_is_tile_reserved(uint32_t grid_value) {
    return (grid_value & AGENT_GRID_RESERVED_BIT) != 0;
}

static inline int map_is_tile_occupied(uint32_t grid_value) {
    AgentID id = map_extract_agent_id(grid_value);
    return id != PATIKA_INVALID_AGENT_ID && !(grid_value & AGENT_GRID_RESERVED_BIT);
}

static inline int map_is_tile_empty(uint32_t grid_value) {
    return map_extract_agent_id(grid_value) == PATIKA_INVALID_AGENT_ID;
}
struct PCG32
{
    uint64_t state;
};

void pcg32_init(PCG32 *rng, uint64_t seed);
uint32_t pcg32_next(PCG32 *rng);

/* Collision */

/**
 * @brief Check if agent A can physically enter a tile where agent B exists
 * @return 1 if can enter (no collision), 0 if blocked
 */
int can_agent_enter(const AgentSlot *agent_A, const AgentSlot *agent_B);

/**
 * @brief Check if agent A should attack agent B (aggression check)
 * @return 1 if should attack, 0 if ignore
 */
int should_agent_attack(const AgentSlot *agent_A, const AgentSlot *agent_B);

/**
 * @brief Try to reserve a tile for agent movement
 * @return 1 if reservation successful, 0 if failed (blocked/occupied)
 */
int try_reserve_tile(struct PatikaContext *ctx, AgentSlot *agent, int32_t q, int32_t r);

/**
 * @brief Clear agent's reservation (used when agent cancels movement)
 */
void clear_tile_reservation(MapGrid *map, int32_t q, int32_t r, AgentID agent_id);

void process_movement(struct PatikaContext *ctx, AgentSlot *agent);


struct PatikaContext
{
    PatikaConfig config;
    MPSCCommandQueue cmd_queue;
    SPSCEventQueue event_queue;
    AgentPool agents;
    BarrackPool barracks;
    MapGrid map;
    PatikaSnapshot snapshots[2];
    _Atomic uint32_t snapshot_index;
    _Atomic uint64_t version;
    PCG32 rng;
    PatikaStats stats;
};
void process_command(struct PatikaContext *ctx, const PatikaCommand *cmd);

void compute_next_step(struct PatikaContext *ctx, AgentSlot *agent);

void update_snapshot(struct PatikaContext *ctx);

void compute_patrol(struct PatikaContext *ctx, AgentSlot *agent);

#endif
