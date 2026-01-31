#include "internal/patika_internal.h"
#include <stdlib.h>

void process_command(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    switch (cmd->type)
    {
    case CMD_ADD_AGENT:
    {
        AddAgentPayload *payload = (AddAgentPayload *)cmd->large_command.payload;

        if (!payload)
        {
            PATIKA_LOG_ERROR("ADD_AGENT: NULL payload");
            return;
        }

        if (!map_in_bounds(&ctx->map, payload->start_q, payload->start_r))
        {
            PATIKA_LOG_ERROR("ADD_AGENT: position (%d, %d) out of bounds",
                             payload->start_q, payload->start_r);
            free(payload);
            return;
        }

        MapTile *tile = map_get(&ctx->map, payload->start_q, payload->start_r);
        if (tile->state != 0)
        {
            PATIKA_LOG_ERROR("ADD_AGENT: position (%d, %d) is not walkable",
                             payload->start_q, payload->start_r);
            free(payload);
            return;
        }

        AgentID id = agent_pool_allocate(&ctx->agents);
        AgentSlot *agent = agent_pool_get(&ctx->agents, id);
        if (!agent)
        {
            PATIKA_LOG_ERROR("ADD_AGENT: agent pool full");
            free(payload);
            return;
        }

        if (try_reserve_tile(ctx, agent, payload->start_q, payload->start_r) != 1)
        {
            agent_pool_free(&ctx->agents, id);
            PATIKA_LOG_ERROR("ADD_AGENT: tile (%d, %d) is occupied",
                             payload->start_q, payload->start_r);
            free(payload);
            return;
        }

        agent->pos_q    = payload->start_q;
        agent->pos_r    = payload->start_r;
        agent->next_q   = payload->start_q;
        agent->next_r   = payload->start_r;
        // no goals here.
        agent->target_q = payload->start_q;
        agent->target_r = payload->start_r;

        agent->faction         = payload->faction;
        agent->side            = payload->side;
        agent->parent_barrack  = payload->parent_barrack;
        agent->collision_data  = payload->collision_data;

        agent->behavior = BEHAVIOR_IDLE;
        agent->state    = STATE_IDLE;

        /* write-back ID if caller requested */
        if (payload->out_agent_id != NULL)
        {
            *payload->out_agent_id = agent->id;
        }

        PATIKA_LOG_DEBUG("ADD_AGENT: agent %u spawned at (%d, %d)",
                         agent->id, agent->pos_q, agent->pos_r);

        ctx->stats.commands_processed++;
        ctx->stats.active_agents++;
        free(payload);
        break;
    }

    case CMD_ADD_AGENT_WITH_BEHAVIOR:
    {
        AddAgentWithBehaviorPayload *payload =
            (AddAgentWithBehaviorPayload *)cmd->large_command.payload;

        if (!payload)
        {
            PATIKA_LOG_ERROR("ADD_AGENT_WITH_BEHAVIOR: NULL payload");
            return;
        }

        if (!map_in_bounds(&ctx->map, payload->start_q, payload->start_r))
        {
            PATIKA_LOG_ERROR("ADD_AGENT_WITH_BEHAVIOR: position (%d, %d) out of bounds",
                             payload->start_q, payload->start_r);
            free(payload);
            return;
        }

        MapTile *tile = map_get(&ctx->map, payload->start_q, payload->start_r);
        if (tile->state != 0)
        {
            PATIKA_LOG_ERROR("ADD_AGENT_WITH_BEHAVIOR: position (%d, %d) is not walkable",
                             payload->start_q, payload->start_r);
            free(payload);
            return;
        }

        AgentID id = agent_pool_allocate(&ctx->agents);
        AgentSlot *agent = agent_pool_get(&ctx->agents, id);
        if (!agent)
        {
            PATIKA_LOG_ERROR("ADD_AGENT_WITH_BEHAVIOR: agent pool full");
            free(payload);
            return;
        }

        if (try_reserve_tile(ctx, agent, payload->start_q, payload->start_r) != 1)
        {
            agent_pool_free(&ctx->agents, id);
            PATIKA_LOG_ERROR("ADD_AGENT_WITH_BEHAVIOR: tile (%d, %d) is occupied",
                             payload->start_q, payload->start_r);
            free(payload);
            return;
        }

        agent->pos_q    = payload->start_q;
        agent->pos_r    = payload->start_r;
        agent->next_q   = payload->start_q;
        agent->next_r   = payload->start_r;
        agent->target_q = payload->start_q;
        agent->target_r = payload->start_r;

        agent->faction        = payload->faction;
        agent->side           = payload->side;
        agent->parent_barrack = payload->parent_barrack;
        agent->collision_data.layer          = payload->collision_data.layer;
        agent->collision_data.collision_mask = payload->collision_data.collision_mask;
        agent->collision_data.aggression_mask= payload->collision_data.aggression_mask;

        agent->behavior = payload->initial_behavior;

        switch (payload->initial_behavior)
        {
            case BEHAVIOR_IDLE:
                agent->state = STATE_IDLE;
                break;

            case BEHAVIOR_PATROL:
                agent->behavior_data.patrol.center_q      = payload->behavior_params.patrol.center_q;
                agent->behavior_data.patrol.center_r      = payload->behavior_params.patrol.center_r;
                agent->behavior_data.patrol.radius        = payload->behavior_params.patrol.radius;
                agent->behavior_data.patrol.waypoint_index = 0;
                agent->behavior_data.patrol.idle_timer    = 0.0f;
                agent->state = STATE_CALCULATING;
                break;

            case BEHAVIOR_EXPLORE:
                agent->behavior_data.explore.mode            = payload->behavior_params.explore.mode;
                agent->behavior_data.explore.cells_visited   = 0;
                agent->behavior_data.explore.last_target_q   = agent->pos_q;
                agent->behavior_data.explore.last_target_r   = agent->pos_r;
                agent->state = STATE_CALCULATING;
                break;

            case BEHAVIOR_GUARD:
                PATIKA_LOG_WARN("ADD_AGENT_WITH_BEHAVIOR: GUARD not implemented, falling back to IDLE");
                agent->behavior = BEHAVIOR_IDLE;
                agent->state    = STATE_IDLE;
                break;

            case BEHAVIOR_FLEE:
                PATIKA_LOG_WARN("ADD_AGENT_WITH_BEHAVIOR: FLEE not implemented, falling back to IDLE");
                agent->behavior = BEHAVIOR_IDLE;
                agent->state    = STATE_IDLE;
                break;

            default:
                PATIKA_LOG_ERROR("ADD_AGENT_WITH_BEHAVIOR: unknown behavior %d, falling back to IDLE",
                                 (int)payload->initial_behavior);
                agent->behavior = BEHAVIOR_IDLE;
                agent->state    = STATE_IDLE;
                break;
        }

        /* write-back ID if caller requested */
        if (payload->out_agent_id != NULL)
        {
            *payload->out_agent_id = agent->id;
        }

        PATIKA_LOG_DEBUG("ADD_AGENT_WITH_BEHAVIOR: agent %u spawned at (%d, %d) behavior=%d",
                         agent->id, agent->pos_q, agent->pos_r, (int)agent->behavior);

        ctx->stats.commands_processed++;
        ctx->stats.active_agents++;
        free(payload);
        break;
    }

    case CMD_REMOVE_AGENT:
    {
        AgentSlot *agent = agent_pool_get(&ctx->agents, cmd->remove_agent.agent_id);
        if (!agent || !agent->active)
        {
            PATIKA_LOG_WARN("REMOVE_AGENT: agent %u not found or already inactive",
                            cmd->remove_agent.agent_id);
            break;
        }

        /* clear tile so nothing ghosts here */
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
            PATIKA_LOG_ERROR("SET_GOAL: position (%d, %d) out of bounds",
                             cmd->set_goal.goal_q, cmd->set_goal.goal_r);
            break;
        }

        agent->target_q = cmd->set_goal.goal_q;
        agent->target_r = cmd->set_goal.goal_r;
        agent->behavior = BEHAVIOR_IDLE;
        agent->state    = STATE_CALCULATING;

        PATIKA_LOG_DEBUG("SET_GOAL: agent %u -> (%d, %d)",
                         agent->id, agent->target_q, agent->target_r);
        ctx->stats.commands_processed++;
        break;
    }

    case CMD_SET_TILE_STATE:
    {
        if (!map_in_bounds(&ctx->map, cmd->set_tile.q, cmd->set_tile.r))
        {
            PATIKA_LOG_ERROR("SET_TILE_STATE: (%d, %d) out of bounds",
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
        AddBarrackPayload *payload = (AddBarrackPayload *)cmd->large_command.payload;

        if (!payload)
        {
            PATIKA_LOG_ERROR("ADD_BARRACK: NULL payload");
            return;
        }

        if (!map_in_bounds(&ctx->map, payload->pos_q, payload->pos_r))
        {
            PATIKA_LOG_ERROR("ADD_BARRACK: position (%d, %d) out of bounds",
                             payload->pos_q, payload->pos_r);
            free(payload);
            return;
        }

        BuildingID id = barrack_pool_allocate(&ctx->barracks);
        if (id == PATIKA_INVALID_BARRACK_ID)
        {
            PATIKA_LOG_ERROR("ADD_BARRACK: barrack pool full");
            free(payload);
            return;
        }

        BarrackSlot *barrack = barrack_pool_get(&ctx->barracks, id);
        if (!barrack)
        {
            PATIKA_LOG_ERROR("ADD_BARRACK: allocated ID %u is invalid", (unsigned)id);
            free(payload);
            return;
        }

        barrack->pos_q         = payload->pos_q;
        barrack->pos_r         = payload->pos_r;
        barrack->faction       = payload->faction;
        barrack->side          = payload->side;
        barrack->patrol_radius = payload->patrol_radius;
        barrack->max_agents    = payload->max_agents;
        barrack->behavior      = payload->behavior;
        barrack->agent_count   = 0;

        /* write-back ID if caller requested */
        if (payload->out_barrack_id != NULL)
        {
            *payload->out_barrack_id = id;
        }

        PATIKA_LOG_DEBUG("ADD_BARRACK: barrack %u at (%d, %d)",
                         (unsigned)id, barrack->pos_q, barrack->pos_r);

        ctx->stats.commands_processed++;
        ctx->stats.active_barracks++;
        free(payload);
        break;
    }

    case CMD_REMOVE_BARRACK:
    {
        /* No inline field for barrack_id in the command union yet.
         * Stubbed — implement once base.h gets a remove_barrack member. */
        PATIKA_LOG_WARN("REMOVE_BARRACK: not implemented");
        break;
    }

    default:
        PATIKA_LOG_WARN("process_command: unhandled command type %d", (int)cmd->type);
        break;
    }
}
