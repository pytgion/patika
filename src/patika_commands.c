#include "internal/patika_internal.h"

void process_command(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    switch (cmd->type)
    {
    case CMD_ADD_AGENT:
    {
        const AddAgentPayload *p = &cmd->add_agent;

        if (!map_in_bounds(&ctx->map, p->start_q, p->start_r))
        {
            PATIKA_LOG_ERROR("ADD_AGENT: position (%d,%d) out of bounds", p->start_q, p->start_r);
            break;
        }

        MapTile *tile = map_get(&ctx->map, p->start_q, p->start_r);
        if (tile->state != 0)
        {
            PATIKA_LOG_ERROR("ADD_AGENT: position (%d,%d) not walkable", p->start_q, p->start_r);
            break;
        }

        AgentID    id    = agent_pool_allocate(&ctx->agents);
        AgentSlot *agent = agent_pool_get(&ctx->agents, id);
        if (!agent)
        {
            PATIKA_LOG_ERROR("ADD_AGENT: agent pool full");
            break;
        }

        if (try_reserve_tile(ctx, agent, p->start_q, p->start_r) != 0)
        {
            agent_pool_free(&ctx->agents, id);
            PATIKA_LOG_ERROR("ADD_AGENT: tile (%d,%d) occupied", p->start_q, p->start_r);
            break;
        }

        agent->pos_q    = agent->next_q   = agent->target_q = p->start_q;
        agent->pos_r    = agent->next_r   = agent->target_r = p->start_r;
        agent->group    = p->group;
        agent->team     = p->team;
        agent->parent_barrack = p->parent_barrack;
        agent->collision_data = p->collision_data;
        agent->behavior = BEHAVIOR_IDLE;
        agent->state    = STATE_IDLE;
        agent->path_len = agent->path_cursor = 0;

        if (p->out_agent_id) *p->out_agent_id = agent->id;

        PATIKA_LOG_DEBUG("ADD_AGENT: agent %u at (%d,%d)", agent->id, agent->pos_q, agent->pos_r);
        ctx->stats.commands_processed++;
        ctx->stats.active_agents++;
        break;
    }

    case CMD_ADD_AGENT_WITH_BEHAVIOR:
    {
        const AddAgentWithBehaviorPayload *p = &cmd->add_agent_with_behavior;

        if (!map_in_bounds(&ctx->map, p->start_q, p->start_r))
        {
            PATIKA_LOG_ERROR("ADD_AGENT_WITH_BEHAVIOR: position (%d,%d) out of bounds",
                             p->start_q, p->start_r);
            break;
        }

        MapTile *tile = map_get(&ctx->map, p->start_q, p->start_r);
        if (tile->state != 0)
        {
            PATIKA_LOG_ERROR("ADD_AGENT_WITH_BEHAVIOR: position (%d,%d) not walkable",
                             p->start_q, p->start_r);
            break;
        }

        AgentID    id    = agent_pool_allocate(&ctx->agents);
        AgentSlot *agent = agent_pool_get(&ctx->agents, id);
        if (!agent)
        {
            PATIKA_LOG_ERROR("ADD_AGENT_WITH_BEHAVIOR: agent pool full");
            break;
        }

        if (try_reserve_tile(ctx, agent, p->start_q, p->start_r) != 0)
        {
            agent_pool_free(&ctx->agents, id);
            PATIKA_LOG_ERROR("ADD_AGENT_WITH_BEHAVIOR: tile (%d,%d) occupied",
                             p->start_q, p->start_r);
            break;
        }

        agent->pos_q    = agent->next_q   = agent->target_q = p->start_q;
        agent->pos_r    = agent->next_r   = agent->target_r = p->start_r;
        agent->group    = p->group;
        agent->team     = p->team;
        agent->parent_barrack         = p->parent_barrack;
        agent->collision_data.layer         = p->collision_data.layer;
        agent->collision_data.collision_mask = p->collision_data.collision_mask;
        agent->behavior = p->initial_behavior;
        agent->path_len = agent->path_cursor = 0;

        switch (p->initial_behavior)
        {
        case BEHAVIOR_IDLE:
            agent->state = STATE_IDLE;
            break;

        case BEHAVIOR_PATROL:
            agent->behavior_data.patrol.center_q       = p->behavior_params.patrol.center_q;
            agent->behavior_data.patrol.center_r       = p->behavior_params.patrol.center_r;
            agent->behavior_data.patrol.radius         = p->behavior_params.patrol.radius;
            agent->behavior_data.patrol.waypoint_index = 0;
            agent->behavior_data.patrol.idle_timer     = 0.0f;
            agent->state = STATE_CALCULATING;
            break;

        case BEHAVIOR_EXPLORE:
            agent->behavior_data.explore.mode           = p->behavior_params.explore.mode;
            agent->behavior_data.explore.cells_visited  = 0;
            agent->behavior_data.explore.last_target_q  = p->start_q;
            agent->behavior_data.explore.last_target_r  = p->start_r;
            agent->state = STATE_CALCULATING;
            break;

        default:
            PATIKA_LOG_ERROR("ADD_AGENT_WITH_BEHAVIOR: unknown behavior %d, using IDLE",
                             (int)p->initial_behavior);
            agent->behavior = BEHAVIOR_IDLE;
            agent->state    = STATE_IDLE;
            break;
        }

        if (p->out_agent_id) *p->out_agent_id = agent->id;

        PATIKA_LOG_DEBUG("ADD_AGENT_WITH_BEHAVIOR: agent %u at (%d,%d) behavior=%d",
                         agent->id, agent->pos_q, agent->pos_r, (int)agent->behavior);
        ctx->stats.commands_processed++;
        ctx->stats.active_agents++;
        break;
    }

    case CMD_REMOVE_AGENT:
    {
        AgentSlot *agent = agent_pool_get(&ctx->agents, cmd->remove_agent.agent_id);
        if (!agent || !agent->active)
        {
            PATIKA_LOG_WARN("REMOVE_AGENT: agent %u not found", cmd->remove_agent.agent_id);
            break;
        }

        map_set_agent_grid(&ctx->map, agent->pos_q, agent->pos_r, PATIKA_INVALID_AGENT_ID);
        agent_pool_free(&ctx->agents, cmd->remove_agent.agent_id);

        PatikaEvent evt = {EVENT_AGENT_REMOVED, cmd->remove_agent.agent_id, 0, 0};
        spsc_push(&ctx->event_queue, &evt);

        ctx->stats.active_agents--;
        ctx->stats.commands_processed++;
        break;
    }

    case CMD_SET_GOAL:
    {
        AgentSlot *agent = agent_pool_get(&ctx->agents, cmd->set_goal.agent_id);
        if (!agent || !agent->active)
        {
            PATIKA_LOG_WARN("SET_GOAL: agent %u not found", cmd->set_goal.agent_id);
            break;
        }
        if (!map_in_bounds(&ctx->map, cmd->set_goal.goal_q, cmd->set_goal.goal_r))
        {
            PATIKA_LOG_ERROR("SET_GOAL: (%d,%d) out of bounds",
                             cmd->set_goal.goal_q, cmd->set_goal.goal_r);
            break;
        }

        agent->target_q = cmd->set_goal.goal_q;
        agent->target_r = cmd->set_goal.goal_r;
        agent->behavior = BEHAVIOR_IDLE;
        agent->state    = STATE_CALCULATING;
        agent->path_len = agent->path_cursor = 0; /* cached route was for the old target */

        PATIKA_LOG_DEBUG("SET_GOAL: agent %u -> (%d,%d)",
                         agent->id, agent->target_q, agent->target_r);
        ctx->stats.commands_processed++;
        break;
    }

    case CMD_SET_BEHAVIOR:
    {
        AgentSlot *agent = agent_pool_get(&ctx->agents, cmd->set_behavior.agent_id);
        if (!agent || !agent->active)
        {
            PATIKA_LOG_WARN("SET_BEHAVIOR: agent %u not found", cmd->set_behavior.agent_id);
            break;
        }
        agent->behavior = cmd->set_behavior.behavior;
        if (agent->state == STATE_IDLE)
            agent->state = STATE_CALCULATING;

        ctx->stats.commands_processed++;
        break;
    }

    case CMD_SET_TILE_STATE:
    {
        if (!map_in_bounds(&ctx->map, cmd->set_tile.q, cmd->set_tile.r))
        {
            PATIKA_LOG_ERROR("SET_TILE_STATE: (%d,%d) out of bounds",
                             cmd->set_tile.q, cmd->set_tile.r);
            break;
        }
        MapTile *tile = map_get(&ctx->map, cmd->set_tile.q, cmd->set_tile.r);
        if (tile)
        {
            tile->state = cmd->set_tile.state;
            ctx->stats.commands_processed++;
        }
        break;
    }

    case CMD_ADD_BARRACK:
    {
        const AddBarrackPayload *p = &cmd->add_barrack;

        if (!map_in_bounds(&ctx->map, p->pos_q, p->pos_r))
        {
            PATIKA_LOG_ERROR("ADD_BARRACK: position (%d,%d) out of bounds", p->pos_q, p->pos_r);
            break;
        }

        BuildingID   id      = barrack_pool_allocate(&ctx->barracks);
        BarrackSlot *barrack = barrack_pool_get(&ctx->barracks, id);
        if (!barrack)
        {
            PATIKA_LOG_ERROR("ADD_BARRACK: pool full or invalid id %u", (unsigned)id);
            break;
        }

        barrack->pos_q         = p->pos_q;
        barrack->pos_r         = p->pos_r;
        barrack->group         = p->group;
        barrack->team          = p->team;
        barrack->patrol_radius = p->patrol_radius;
        barrack->max_agents    = p->max_agents;
        barrack->behavior      = p->behavior;
        barrack->agent_count   = 0;

        if (p->out_barrack_id) *p->out_barrack_id = id;

        PATIKA_LOG_DEBUG("ADD_BARRACK: barrack %u at (%d,%d)", (unsigned)id, p->pos_q, p->pos_r);
        ctx->stats.commands_processed++;
        ctx->stats.active_barracks++;
        break;
    }

    case CMD_REMOVE_BARRACK:
    {
        BarrackSlot *barrack = barrack_pool_get(&ctx->barracks, cmd->remove_barrack.barrack_id);
        if (!barrack)
        {
            PATIKA_LOG_WARN("REMOVE_BARRACK: barrack %u not found",
                            (unsigned)cmd->remove_barrack.barrack_id);
            break;
        }
        barrack->active = 0;
        ctx->stats.active_barracks--;
        ctx->stats.commands_processed++;
        break;
    }

    default:
        PATIKA_LOG_WARN("process_command: unhandled type %d", (int)cmd->type);
        break;
    }
}
