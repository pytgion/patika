#include "internal/patika_internal.h"


void agent_arrive_at_tile(struct PatikaContext *ctx, AgentSlot *agent) {
    map_set_agent_grid(&ctx->map, agent->pos_q, agent->pos_r, PATIKA_INVALID_AGENT_ID);

    uint32_t grid_val = map_get_agent_grid(&ctx->map, agent->next_q, agent->next_r);
    AgentID occupant_id = grid_val & AGENT_GRID_AGENT_MASK;

    if (occupant_id != PATIKA_INVALID_AGENT_ID && occupant_id != agent->id) {
        AgentSlot *occupant = agent_pool_get(&ctx->agents, occupant_id);

        if (occupant && occupant->active) {
            // check collision
            if (agent->collision_data.collision_mask & occupant->collision_data.layer) {
                PATIKA_INTERNAL_LOG_ERROR("collision detected not suppose to happen, agent %d turns back previous location", agent->id);
                map_set_agent_grid(&ctx->map, agent->pos_q, agent->pos_r, agent->id);
                agent->state = STATE_CALCULATING;
                agent->progress = 0;
                return;
            }
            // check aggression
            if (agent->collision_data.aggression_mask & occupant->collision_data.layer) {
                agent->interaction_data.type = INTERACT_ATTACK;
                agent->interaction_data.data.agent.target_id = occupant_id;
            }
        }
    }

    map_set_agent_grid(&ctx->map, agent->next_q, agent->next_r, agent->id);
    agent->pos_q = agent->next_q;
    agent->pos_r = agent->next_r;
    agent->progress = 0;

    if (agent->pos_q == agent->target_q && agent->pos_r == agent->target_r) {
        agent->state = STATE_IDLE;
        PatikaEvent evt = {EVENT_REACHED_GOAL, agent->id, agent->pos_q, agent->pos_r};
        spsc_push(&ctx->event_queue, &evt);
    } else {
        agent->state = STATE_CALCULATING;
    }
}

void process_movement(struct PatikaContext *ctx, AgentSlot *agent)
{
    if (agent->progress >= AGENT_PROGRESS_MAX_DISTANCE)
    {
        agent_arrive_at_tile(ctx, agent);
    }
    else {
        agent->progress += agent->speed; // TODO: a multipler based on tile's occupation level
    }

}
