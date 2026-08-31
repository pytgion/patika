#include "internal/patika_internal.h"
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Heuristics
 * ------------------------------------------------------------------------- */

static inline int32_t hex_dist(int32_t q1, int32_t r1, int32_t q2, int32_t r2)
{
    return (abs(q1-q2) + abs(q1+r1-q2-r2) + abs(r1-r2)) / 2;
}

static inline int32_t manhattan(int32_t q1, int32_t r1, int32_t q2, int32_t r2)
{
    return abs(q1-q2) + abs(r1-r2);
}

/* -------------------------------------------------------------------------
 * Flat-index helpers
 * ------------------------------------------------------------------------- */

static inline uint32_t grid_to_idx(const MapGrid *map, int32_t q, int32_t r)
{
    if (map->type == MAP_TYPE_RECTANGULAR)
        return (uint32_t)(r * (int32_t)map->width + q);

    int32_t radius = (int32_t)(map->width - 1) / 2;
    return (uint32_t)((r + radius) * (int32_t)map->width + (q + radius));
}

static inline void idx_to_grid(const MapGrid *map, uint32_t idx, int32_t *q, int32_t *r)
{
    if (map->type == MAP_TYPE_RECTANGULAR) {
        *q = (int32_t)(idx % map->width);
        *r = (int32_t)(idx / map->width);
    } else {
        int32_t radius = (int32_t)(map->width - 1) / 2;
        *q = (int32_t)(idx % map->width) - radius;
        *r = (int32_t)(idx / map->width) - radius;
    }
}

/* -------------------------------------------------------------------------
 * Binary min-heap (keyed by f)
 *
 * Stores flat grid indices.  Lazy deletion: stale open-set entries are
 * detected in the main loop via state == ASTAR_CLOSED.
 * ------------------------------------------------------------------------- */

#define HEAP_PARENT(i) (((i) - 1) / 2)
#define HEAP_LEFT(i)   ((i) * 2 + 1)
#define HEAP_RIGHT(i)  ((i) * 2 + 2)

static void heap_push(uint32_t *heap, uint32_t *size,
                      uint32_t idx, const PathNode *nodes)
{
    uint32_t i = (*size)++;
    heap[i] = idx;
    while (i > 0) {
        uint32_t p = HEAP_PARENT(i);
        if (nodes[heap[p]].f <= nodes[heap[i]].f) break;
        uint32_t tmp = heap[p]; heap[p] = heap[i]; heap[i] = tmp;
        i = p;
    }
}

static uint32_t heap_pop(uint32_t *heap, uint32_t *size,
                         const PathNode *nodes)
{
    uint32_t result = heap[0];
    heap[0] = heap[--(*size)];
    uint32_t i = 0;
    for (;;) {
        uint32_t l = HEAP_LEFT(i), r = HEAP_RIGHT(i), s = i;
        if (l < *size && nodes[heap[l]].f < nodes[heap[s]].f) s = l;
        if (r < *size && nodes[heap[r]].f < nodes[heap[s]].f) s = r;
        if (s == i) break;
        uint32_t tmp = heap[s]; heap[s] = heap[i]; heap[i] = tmp;
        i = s;
    }
    return result;
}

/* -------------------------------------------------------------------------
 * A* core
 *
 * Returns 1 and sets agent->next_q/r + STATE_MOVING on success.
 * Returns 0 on no-path or resource exhaustion.
 * ------------------------------------------------------------------------- */

static int astar(struct PatikaContext *ctx, AgentSlot *agent,
                 int32_t goal_q, int32_t goal_r)
{
    MapGrid *map = &ctx->map;
    uint32_t grid_size   = map->width * map->height;
    uint32_t node_budget = ctx->config.max_path_nodes > 0
                           ? ctx->config.max_path_nodes
                           : grid_size;

    PathNode *nodes = ctx->astar_nodes;
    uint32_t *heap  = ctx->astar_heap;
    if (!nodes || !heap) return 0;

    /* Zero only the node states — heap is write-before-read. */
    memset(nodes, 0, grid_size * sizeof(PathNode));

    uint32_t heap_size = 0;
    uint32_t start_idx = grid_to_idx(map, agent->pos_q, agent->pos_r);
    uint32_t goal_idx  = grid_to_idx(map, goal_q, goal_r);

    const int (*dirs)[2];
    int dir_count;
    if (map->type == MAP_TYPE_RECTANGULAR) {
        dirs      = RECT_DIRS;
        dir_count = 4;
    } else {
        dirs      = HEX_DIRS;
        dir_count = 6;
    }

    nodes[start_idx].g      = 0;
    nodes[start_idx].f      = (map->type == MAP_TYPE_RECTANGULAR)
                              ? manhattan(agent->pos_q, agent->pos_r, goal_q, goal_r)
                              : hex_dist (agent->pos_q, agent->pos_r, goal_q, goal_r);
    nodes[start_idx].parent = start_idx;
    nodes[start_idx].state  = ASTAR_OPEN;
    heap_push(heap, &heap_size, start_idx, nodes);

    int      found          = 0;
    uint32_t expansions     = 0;

    while (heap_size > 0 && expansions < node_budget) {
        uint32_t cur = heap_pop(heap, &heap_size, nodes);

        if (nodes[cur].state == ASTAR_CLOSED) continue; // stale entry
        if (cur == goal_idx) { found = 1; break; }

        nodes[cur].state = ASTAR_CLOSED;
        expansions++;

        int32_t cq, cr;
        idx_to_grid(map, cur, &cq, &cr);

        for (int d = 0; d < dir_count; d++) {
            int32_t nq = cq + dirs[d][0];
            int32_t nr = cr + dirs[d][1];

            if (!map_in_bounds(map, nq, nr)) continue;

            MapTile *tile = map_get(map, nq, nr);
            if (!tile || tile->state != 0) continue;

            /* Treat tiles reserved/occupied by other agents as blocked.
             * Exception: allow entry to the goal tile so agents don't get
             * permanently stuck when the goal itself is contested. */
            uint32_t grid_val = map_get_agent_grid(map, nq, nr);
            AgentID  occupant = map_extract_agent_id(grid_val);
            if (occupant != PATIKA_INVALID_AGENT_ID &&
                occupant != agent->id            &&
                !(nq == goal_q && nr == goal_r))
            {
                continue;
            }

            uint32_t n_idx = grid_to_idx(map, nq, nr);
            if (nodes[n_idx].state == ASTAR_CLOSED) continue;

            int32_t new_g = nodes[cur].g + 1;
            if (nodes[n_idx].state == ASTAR_OPEN && new_g >= nodes[n_idx].g) continue;

            int32_t h = (map->type == MAP_TYPE_RECTANGULAR)
                        ? manhattan(nq, nr, goal_q, goal_r)
                        : hex_dist (nq, nr, goal_q, goal_r);

            nodes[n_idx].g      = new_g;
            nodes[n_idx].f      = new_g + h;
            nodes[n_idx].parent = cur;
            nodes[n_idx].state  = ASTAR_OPEN;
            heap_push(heap, &heap_size, n_idx, nodes);
        }
    }

    if (found) {
        /* Trace the path back to find the first step after start. */
        uint32_t step = goal_idx;
        while (nodes[step].parent != start_idx) {
            step = nodes[step].parent;
        }
        int32_t nq, nr;
        idx_to_grid(map, step, &nq, &nr);
        agent->next_q = nq;
        agent->next_r = nr;
        agent->state  = STATE_MOVING;
    }

    return found;
}

/* -------------------------------------------------------------------------
 * Public: compute_next_step
 * ------------------------------------------------------------------------- */

void compute_next_step(struct PatikaContext *ctx, AgentSlot *agent)
{
    if (agent->pos_q == agent->target_q &&
        agent->pos_r == agent->target_r &&
        agent->behavior == BEHAVIOR_IDLE)
    {
        agent->state = STATE_IDLE;
        PatikaEvent evt = {EVENT_REACHED_GOAL, agent->id, agent->pos_q, agent->pos_r};
        spsc_push(&ctx->event_queue, &evt);
        return;
    }

    if (!astar(ctx, agent, agent->target_q, agent->target_r)) {
        agent->state = STATE_IDLE;
        PatikaEvent evt = {EVENT_STUCK, agent->id, agent->pos_q, agent->pos_r};
        spsc_push(&ctx->event_queue, &evt);
        PATIKA_LOG_DEBUG("Agent %u stuck at (%d,%d)", agent->id, agent->pos_q, agent->pos_r);
    }
}

/* -------------------------------------------------------------------------
 * Public: compute_patrol
 *
 * Random walk within patrol_radius of parent barrack.
 * Now supports both rectangular and hex grids.
 * ------------------------------------------------------------------------- */

void compute_patrol(struct PatikaContext *ctx, AgentSlot *agent)
{
    BarrackSlot *barrack = barrack_pool_get(&ctx->barracks, agent->parent_barrack);
    if (!barrack) {
        agent->state = STATE_REMOVE_QUEUE;
        return;
    }

    const int (*dirs)[2];
    int dir_count;
    if (ctx->map.type == MAP_TYPE_RECTANGULAR) {
        dirs      = RECT_DIRS;
        dir_count = 4;
    } else {
        dirs      = HEX_DIRS;
        dir_count = 6;
    }

    int candidates[6];
    int count = 0;

    for (int i = 0; i < dir_count; i++) {
        int32_t nq = agent->pos_q + dirs[i][0];
        int32_t nr = agent->pos_r + dirs[i][1];

        if (!map_in_bounds(&ctx->map, nq, nr)) continue;
        MapTile *t = map_get(&ctx->map, nq, nr);
        if (!t || t->state != 0) continue;

        int32_t dist = (ctx->map.type == MAP_TYPE_RECTANGULAR)
                       ? manhattan(nq, nr, barrack->pos_q, barrack->pos_r)
                       : hex_dist (nq, nr, barrack->pos_q, barrack->pos_r);

        if (dist <= (int32_t)barrack->patrol_radius)
            candidates[count++] = i;
    }

    if (count > 0) {
        int choice = candidates[pcg32_next(&ctx->rng) % (uint32_t)count];
        agent->next_q = agent->pos_q + dirs[choice][0];
        agent->next_r = agent->pos_r + dirs[choice][1];
        agent->state  = STATE_MOVING;
    } else {
        agent->state = STATE_IDLE;
    }
}
