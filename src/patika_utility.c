#include "internal/patika_internal.h"
#include "patika.h"

static inline AgentID map_get_agent_id(uint32_t grid_value)
{
    return grid_value & AGENT_GRID_AGENT_MASK;
}

static inline int is_tile_reserved(uint32_t grid_value)
{
    return (grid_value & AGENT_GRID_RESERVED_BIT) != 0;
}

static inline int is_tile_occupied(uint32_t grid_value)
{
    AgentID id = map_get_agent_id(grid_value);
    return id != PATIKA_INVALID_AGENT_ID && !(grid_value & AGENT_GRID_RESERVED_BIT);
}

