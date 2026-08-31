#include "internal/patika_internal.h"

void process_movement(struct PatikaContext *ctx, AgentSlot *agent)
{
    MapTile *tile = map_get(&ctx->map, agent->next_q, agent->next_r);
    if (!tile || tile->state != 0)
    {
        /* Cached tail is for a route through a tile that's no longer
         * walkable — don't keep feeding stale waypoints after replanning. */
        agent->path_len = 0;
        agent->state = STATE_CALCULATING;
        return;
    }

    /* Update reservation table: vacate current tile, occupy next tile. */
    map_set_agent_grid(&ctx->map, agent->pos_q, agent->pos_r, PATIKA_INVALID_AGENT_ID);
    map_set_agent_grid(&ctx->map, agent->next_q, agent->next_r, agent->id);

    agent->pos_q = agent->next_q;
    agent->pos_r = agent->next_r;

    if (agent->pos_q == agent->target_q && agent->pos_r == agent->target_r)
    {
        agent->state = STATE_IDLE;
        PatikaEvent evt = {EVENT_REACHED_GOAL, agent->id, agent->pos_q, agent->pos_r};
        spsc_push(&ctx->event_queue, &evt);
    }
    else if (agent->path_cursor < agent->path_len)
    {
        /* Still have cached waypoints from the last A* search — take the
         * next one instead of doing a full grid re-search this tick. The
         * tile check above re-validates it before it's actually committed. */
        agent->next_q = agent->path_q[agent->path_cursor];
        agent->next_r = agent->path_r[agent->path_cursor];
        agent->path_cursor++;
        agent->state = STATE_MOVING;
    }
    else
    {
        agent->state = STATE_CALCULATING;
    }
}
