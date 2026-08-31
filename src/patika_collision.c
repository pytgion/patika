#include "internal/patika_internal.h"

int can_agent_enter(const AgentSlot *agent_A, const AgentSlot *agent_B)
{
    if (!agent_A || !agent_B) return 0;
    return (agent_A->collision_data.collision_mask & agent_B->collision_data.layer) ? 1 : 0;
}

int try_reserve_tile(struct PatikaContext *ctx, AgentSlot *agent, int32_t q, int32_t r)
{
    if (!map_in_bounds(&ctx->map, q, r)) return 1;

    MapTile *tile = map_get(&ctx->map, q, r);
    if (!tile || tile->state != 0) return 1;

    uint32_t grid_val = map_get_agent_grid(&ctx->map, q, r);

    if (map_is_tile_empty(grid_val))
    {
        map_set_agent_grid(&ctx->map, q, r, agent->id | AGENT_GRID_RESERVED_BIT);
        return 0;
    }

    AgentID   occupant_id = map_extract_agent_id(grid_val);
    AgentSlot *occupant   = agent_pool_get(&ctx->agents, occupant_id);

    if (!occupant || !occupant->active)
    {
        PATIKA_INTERNAL_LOG_WARN("Stale agent_grid at (%d,%d), clearing", q, r);
        map_set_agent_grid(&ctx->map, q, r, agent->id | AGENT_GRID_RESERVED_BIT);
        return 0;
    }

    if (can_agent_enter(agent, occupant) != 0) return 1;

    return 1;
}

void clear_tile_reservation(MapGrid *map, int32_t q, int32_t r, AgentID agent_id)
{
    uint32_t grid_val = map_get_agent_grid(map, q, r);
    if (map_is_tile_reserved(grid_val) && map_extract_agent_id(grid_val) == agent_id)
        map_set_agent_grid(map, q, r, PATIKA_INVALID_AGENT_ID);
}
