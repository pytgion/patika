/*
 * Author: Oguzhan Akyuz
 * Created: 28-10-2025
 * Mail:   pytgion@gmail.com
 * 
 * Updated: 28-10-2025
 */

/* Patika Internal Header
 * Shared types and function declarations across implementation files
 * DO NOT INCLUDE IN PUBLIC API
 */

#ifndef PATIKA_INTERNAL_H
#define PATIKA_INTERNAL_H

#include "../../include/patika_core.h"
#include <stdatomic.h>
#include <stdint.h>

#ifndef INT32_MAX
#define INT32_MAX 0x7FFFFFFF
#endif

/* ============================================================================
 *  FORWARD DECLARATIONS
 * ============================================================================ */
typedef struct MPSCCommandQueue MPSCCommandQueue;
typedef struct SPSCEventQueue SPSCEventQueue;
typedef struct AgentSlot AgentSlot;
typedef struct AgentPool AgentPool;
typedef struct BarrackSlot BarrackSlot;
typedef struct BarrackPool BarrackPool;
typedef struct MapTile MapTile;
typedef struct MapGrid MapGrid;
typedef struct PCG32 PCG32;

/* ============================================================================
 *  MPSC COMMAND QUEUE
 * ============================================================================ */
struct MPSCCommandQueue {
    PatikaCommand* buffer;
    uint32_t capacity;
    _Atomic uint32_t head;
    uint32_t tail;
};

void mpsc_init(MPSCCommandQueue* q, uint32_t capacity);
void mpsc_destroy(MPSCCommandQueue* q);
int mpsc_push(MPSCCommandQueue* q, const PatikaCommand* cmd);
int mpsc_pop(MPSCCommandQueue* q, PatikaCommand* out);

/* ============================================================================
 *  SPSC EVENT QUEUE
 * ============================================================================ */
struct SPSCEventQueue {
    PatikaEvent* buffer;
    uint32_t capacity;
    _Atomic uint32_t head;
    _Atomic uint32_t tail;
};

void spsc_init(SPSCEventQueue* q, uint32_t capacity);
void spsc_destroy(SPSCEventQueue* q);
int spsc_push(SPSCEventQueue* q, const PatikaEvent* evt);
int spsc_pop(SPSCEventQueue* q, PatikaEvent* out);

/* ============================================================================
 *  AGENT POOL
 * ============================================================================ */
struct AgentSlot {
    AgentID id;
    uint16_t generation;
    uint8_t active;
    uint8_t state;
    uint8_t faction;
    uint8_t side;
    BarrackID parent_barrack;
    int32_t pos_q, pos_r;
    int32_t next_q, next_r;
    int32_t target_q, target_r;
    int32_t last_dir_q, last_dir_r;
    uint16_t next_free_index;
};

struct AgentPool {
    AgentSlot* slots;
    uint32_t capacity;
    uint16_t free_head;
    uint32_t active_count;
};

void agent_pool_init(AgentPool* pool, uint32_t capacity);
void agent_pool_destroy(AgentPool* pool);
AgentID agent_pool_allocate(AgentPool* pool);
void agent_pool_free(AgentPool* pool, AgentID id);
AgentSlot* agent_pool_get(AgentPool* pool, AgentID id);

/* ID Helper Functions */
static inline AgentID make_agent_id(uint16_t index, uint16_t gen) {
    return ((uint32_t)gen << 16) | index;
}
static inline uint16_t agent_index(AgentID id) {
    return id & 0xFFFF;
}
static inline uint16_t agent_generation(AgentID id) {
    return id >> 16;
}

/* ============================================================================
 *  BARRACK POOL
 * ============================================================================ */
struct BarrackSlot {
    BarrackID id;
    uint8_t active;
    uint8_t faction;
    uint8_t side;
    uint8_t state;
    uint8_t patrol_radius;
    int32_t pos_q, pos_r;
    uint16_t max_agents;
    uint16_t agent_count;
    uint16_t first_agent_index;
};

struct BarrackPool {
    BarrackSlot* slots;
    uint16_t capacity;
    uint16_t next_id;
};

void barrack_pool_init(BarrackPool* pool, uint16_t capacity);
void barrack_pool_destroy(BarrackPool* pool);
BarrackSlot* barrack_pool_get(BarrackPool* pool, BarrackID id);

/* ============================================================================
 *  MAP GRID
 * ============================================================================ */
struct MapTile {
    uint8_t state;
    uint8_t occupancy;
};

struct MapGrid {
    MapTile* tiles;
    uint32_t width;
    uint32_t height;
};

void map_init(MapGrid* map, uint32_t width, uint32_t height);
void map_destroy(MapGrid* map);
int map_in_bounds(MapGrid* map, int32_t q, int32_t r);
MapTile* map_get(MapGrid* map, int32_t q, int32_t r);

/* ============================================================================
 *  RNG (PCG32)
 * ============================================================================ */
struct PCG32 {
    uint64_t state;
};

void pcg32_init(PCG32* rng, uint64_t seed);
uint32_t pcg32_next(PCG32* rng);

/* ============================================================================
 *  MAIN CONTEXT (opaque to public API)
 * ============================================================================ */
struct PatikaContext {
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

/* ============================================================================
 *  COMMAND PROCESSING
 * ============================================================================ */
void process_command(struct PatikaContext* ctx, const PatikaCommand* cmd);

/* ============================================================================
 *  PATHFINDING
 * ============================================================================ */
void compute_next_step(struct PatikaContext* ctx, AgentSlot* agent);

/* ============================================================================
 *  SNAPSHOT
 * ============================================================================ */
void update_snapshot(struct PatikaContext* ctx);

#endif /* PATIKA_INTERNAL_H */
