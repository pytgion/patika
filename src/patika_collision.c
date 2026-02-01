#include "internal/patika_internal.h"

/**
 * @brief Check if agent A can physically enter a tile where agent B exists
 * @details Uses collision_mask & layer bit test
 * @return 0 if can enter (no collision), non-zero if blocked
 */
int can_agent_enter(const AgentSlot *agent_A, const AgentSlot *agent_B)
{
    if (!agent_A || !agent_B)
        return 0;

    if (agent_A->collision_data.collision_mask & agent_B->collision_data.layer)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief Check if agent A should attack agent B
 * @details Uses aggression_mask & layer bit test + faction check
 * @return 0 if should attack, non-zero if ignore
 */
int should_agent_attack(const AgentSlot *agent_A, const AgentSlot *agent_B)
{
    if (!agent_A || !agent_B)
        return 1;

    if (!(agent_A->collision_data.aggression_mask & agent_B->collision_data.layer))
    {
        return 1;
    }

    if (agent_A->side == agent_B->side)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief Try to reserve a tile for agent movement
 * @details Checks collision, existing reservations/occupations
 * @return 0 if reservation successful, non-zero if failed
 */
int try_reserve_tile(struct PatikaContext *ctx, AgentSlot *agent, int32_t q, int32_t r)
{
    if (!map_in_bounds(&ctx->map, q, r))
    {
        return 1;
    }

    MapTile *tile = map_get(&ctx->map, q, r);
    if (!tile || tile->state != 0)
    {
        return 1;
    }

    uint32_t grid_val = map_get_agent_grid(&ctx->map, q, r);

    if (map_is_tile_empty(grid_val))
    {
        map_set_agent_grid(&ctx->map, q, r, agent->id | AGENT_GRID_RESERVED_BIT);
        return 0;
    }

    // Tile has someone (reserved or occupied)
    AgentID occupant_id = map_extract_agent_id(grid_val);
    AgentSlot *occupant = agent_pool_get(&ctx->agents, occupant_id);

    if (!occupant || !occupant->active)
    {
        PATIKA_INTERNAL_LOG_WARN("Stale agent_grid entry at (%d,%d), clearing", q, r);
        map_set_agent_grid(&ctx->map, q, r, agent->id | AGENT_GRID_RESERVED_BIT);
        return 0;
    }

    // Check physical collision
    if (can_agent_enter(agent, occupant) != 0)
    {
        return 1;
    }

    return 1;
}

/**
 * @brief Clear a tile reservation if it belongs to this agent
 */
void clear_tile_reservation(MapGrid *map, int32_t q, int32_t r, AgentID agent_id)
{
    uint32_t grid_val = map_get_agent_grid(map, q, r);

    if (map_is_tile_reserved(grid_val) && map_extract_agent_id(grid_val) == agent_id)
    {
        map_set_agent_grid(map, q, r, PATIKA_INVALID_AGENT_ID);
    }
}
