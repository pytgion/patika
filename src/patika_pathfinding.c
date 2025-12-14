#include "internal/patika_internal.h"
#include <stdlib.h> // for abs()

static const int HEX_DIRS[6][2] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};


static int get_dist(int q1, int r1, int q2, int r2)
{
    return (abs(q1 - q2) + abs(q1 + r1 - q2 - r2) + abs(r1 - r2)) / 2;
}

void compute_next_step(struct PatikaContext *ctx, AgentSlot *agent)
{
    int32_t best_dist_sq = INT32_MAX;
    int candidates[6];
    int candidate_count = 0;

    for (int i = 0; i < 6; i++)
    {
        int32_t nq = agent->pos_q + HEX_DIRS[i][0];
        int32_t nr = agent->pos_r + HEX_DIRS[i][1];

        MapTile *tile = map_get(&ctx->map, nq, nr);
        if (!tile || tile->state != 0)
            continue;

        int32_t dq = agent->target_q - nq;
        int32_t dr = agent->target_r - nr;
        int32_t dist_sq = dq * dq + dr * dr;

        if (dist_sq < best_dist_sq)
        {
            best_dist_sq = dist_sq;
            candidates[0] = i;
            candidate_count = 1;
        }
        else if (dist_sq == best_dist_sq)
        {
            candidates[candidate_count++] = i;
        }
    }

    if (candidate_count > 0)
    {
        int choice = candidates[pcg32_next(&ctx->rng) % candidate_count];
        agent->next_q = agent->pos_q + HEX_DIRS[choice][0];
        agent->next_r = agent->pos_r + HEX_DIRS[choice][1];
        agent->state = STATE_MOVING; // MOVING
    }
    else
    {
        agent->state = STATE_IDLE; // STUCK
        PatikaEvent evt = {EVENT_STUCK, agent->id, agent->pos_q, agent->pos_r};
        spsc_push(&ctx->event_queue, &evt);
    }
}

void compute_patrol(struct PatikaContext *ctx, AgentSlot *agent)
{
    BarrackSlot *barrack = barrack_pool_get(&ctx->barracks, agent->parent_barrack);

    if (!barrack)
    {
        agent->state = STATE_REMOVE_QUEUE;
        return;
    }
    int candidates[6];
    int count = 0;

    // Check all 6 neighbors
    for (int i = 0; i < 6; i++)
    {
        int32_t nq = agent->pos_q + HEX_DIRS[i][0];
        int32_t nr = agent->pos_r + HEX_DIRS[i][1];

        // 1. Check Map Bounds & Walls
        if (!map_in_bounds(&ctx->map, nq, nr))
        {
            continue;
        }
        MapTile *t = map_get(&ctx->map, nq, nr);
        if (t->state != 0)
        {
            continue; // Blocked
        }

        // 2. Check Radius (The "Leash" Logic)
        int dist_to_barrack = get_dist(nq, nr, barrack->pos_q, barrack->pos_r);

        // ONLY allow move if it stays inside the radius
        if (dist_to_barrack <= barrack->patrol_radius)
        {
            candidates[count++] = i;
        }
        if (count > 0) {
            // Pick random valid neighbor
            int choice = candidates[pcg32_next(&ctx->rng) % count];
            agent->next_q = agent->pos_q + HEX_DIRS[choice][0];
            agent->next_r = agent->pos_r + HEX_DIRS[choice][1];

            // Mark as moving so patika_tick applies the move
            // (But keep logical state as PATROLLING so next tick we do this again)
            agent->state = STATE_MOVING;
        }
    }
}
