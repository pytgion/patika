#include "internal/patika_internal.h"

static const int HEX_DIRS[6][2] = {
    {1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}
};

void compute_next_step(struct PatikaContext* ctx, AgentSlot* agent) {
    int32_t best_dist_sq = INT32_MAX;
    int candidates[6];
    int candidate_count = 0;
    
    for (int i = 0; i < 6; i++) {
        int32_t nq = agent->pos_q + HEX_DIRS[i][0];
        int32_t nr = agent->pos_r + HEX_DIRS[i][1];
        
        MapTile* tile = map_get(&ctx->map, nq, nr);
        if (!tile || tile->state != 0) continue;
        
        int32_t dq = agent->target_q - nq;
        int32_t dr = agent->target_r - nr;
        int32_t dist_sq = dq * dq + dr * dr;
        
        if (dist_sq < best_dist_sq) {
            best_dist_sq = dist_sq;
            candidates[0] = i;
            candidate_count = 1;
        } else if (dist_sq == best_dist_sq) {
            candidates[candidate_count++] = i;
        }
    }
    
    if (candidate_count > 0) {
        int choice = candidates[pcg32_next(&ctx->rng) % candidate_count];
        agent->next_q = agent->pos_q + HEX_DIRS[choice][0];
        agent->next_r = agent->pos_r + HEX_DIRS[choice][1];
        agent->state = 2;  // MOVING
    } else {
        agent->state = 3;  // STUCK
        PatikaEvent evt = {EVENT_STUCK, agent->id, agent->pos_q, agent->pos_r};
        spsc_push(&ctx->event_queue, &evt);
    }
}

