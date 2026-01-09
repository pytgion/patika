#include "internal/patika_internal.h"
#include <stdlib.h>

static void process_cmd_add_agent_behavior(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    AddAgentWithBehaviorPayload* payload = (AddAgentWithBehaviorPayload*)cmd->large_command.payload;

    if (!payload)
    {
        PATIKA_INTERNAL_LOG_ERROR("Can not get AddAgentPayload");
        return;
    }

    AgentID id = agent_pool_allocate(&ctx->agents);
    if (id == PATIKA_INVALID_AGENT_ID)
    {
        PATIKA_INTERNAL_LOG_ERROR("AGENT POOL ALLOCATE FAILED");
        free(payload);
        return;
    }

    AgentSlot *agent = agent_pool_get(&ctx->agents, id);
    if (!agent)
    {
        PATIKA_INTERNAL_LOG_ERROR("AGENT SLOT IS FAILED TO INITIALIZE");
        free(payload);
        return;
    }

    agent->pos_q = payload->start_q;
    agent->pos_r = payload->start_r;
    agent->side = payload->side;
    agent->faction = payload->faction;
    agent->parent_barrack = payload->parent_barrack;
    agent->collision_data.layer = payload->collision_data.layer;
    agent->collision_data.collision_mask = payload->collision_data.collision_mask;
    agent->collision_data.aggression_mask = payload->collision_data.aggression_mask;
    agent->behavior = payload->initial_behavior;

    switch (payload->initial_behavior)
    {
        case BEHAVIOR_IDLE:
            break;

        case BEHAVIOR_EXPLORE:
            agent->behavior_data.explore.mode = payload->behavior_params.explore.mode;
            agent->behavior_data.explore.cells_visited = 0;
            agent->behavior_data.explore.last_target_q = agent->pos_q;
            agent->behavior_data.explore.last_target_r = agent->pos_r;
            break;

        case BEHAVIOR_GUARD:
            PATIKA_INTERNAL_LOG_ERROR("Guard behaviour not supported...");
            break;

        case BEHAVIOR_PATROL:
            agent->behavior_data.patrol.center_q = payload->behavior_params.patrol.center_q;
            agent->behavior_data.patrol.center_r = payload->behavior_params.patrol.center_r;
            agent->behavior_data.patrol.radius = payload->behavior_params.patrol.radius;
            agent->behavior_data.patrol.waypoint_index = 0;
            agent->behavior_data.patrol.idle_timer = 0.0f;
            break;

        case BEHAVIOR_FLEE:
            PATIKA_INTERNAL_LOG_WARN("Are you serious?");
            break;

        default:
            PATIKA_INTERNAL_LOG_ERROR("Unable to resolve behaviour");
            agent->behavior = BEHAVIOR_IDLE;
            break;
    }

    agent->state = STATE_CALCULATING;

    free(payload);
}

static void process_cmd_add_agent(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    AddAgentPayload* payload = (AddAgentPayload*)cmd->large_command.payload;

    if (!payload)
    {
        PATIKA_INTERNAL_LOG_ERROR("Can not get AddAgentPayload");
        return;
    }

    AgentID id = agent_pool_allocate(&ctx->agents);
    if (id == PATIKA_INVALID_AGENT_ID)
    {
        PATIKA_INTERNAL_LOG_ERROR("AGENT POOL ALLOCATE FAILED");
        free(payload);
        return;
    }

    AgentSlot *agent = agent_pool_get(&ctx->agents, id);
    if (!agent)
    {
        PATIKA_INTERNAL_LOG_ERROR("AGENT SLOT IS FAILED TO INITIALIZE");
        free(payload);
        return;
    }

    agent->pos_q = payload->start_q;
    agent->pos_r = payload->start_r;
    agent->side = payload->side;
    agent->faction = payload->faction;
    agent->parent_barrack = payload->parent_barrack;

    BarrackSlot *barrack = NULL;

    if (payload->parent_barrack != PATIKA_INVALID_BARRACK_ID)
    {
        barrack = barrack_pool_get(&ctx->barracks, payload->parent_barrack);
    }

    if (barrack)
    {
        agent->behavior = BEHAVIOR_IDLE;
    }
    else
    {
        BuildingID b_id = barrack_pool_allocate(&ctx->barracks);
        if (id != PATIKA_INVALID_BARRACK_ID)
        {
            barrack = barrack_pool_get(&ctx->barracks, b_id);
            agent->parent_barrack = b_id;
        }
        else if (payload->parent_barrack != PATIKA_INVALID_BARRACK_ID && payload->parent_barrack != 0) {
            PATIKA_INTERNAL_LOG_WARN("Agent spawned with invalid parent barrack ID: %u", payload->parent_barrack);
        }
        agent->behavior = BEHAVIOR_IDLE;
    }

    agent->state = STATE_CALCULATING;

    // only if called asyncned (not likely till stable version)
    // if (payload->out_agent_id)
    // {
    //     *payload->out_agent_id = id;
    // }

    free(payload);
}

static void process_cmd_remove_agent(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    AgentSlot *agent = agent_pool_get(&ctx->agents, cmd->remove_agent.agent_id);
    if (!agent)
    {
        PATIKA_INTERNAL_LOG_ERROR("Invalid agent ID for removal: %u", cmd->remove_agent.agent_id);
        return;
    }
    agent_pool_free(&ctx->agents, cmd->remove_agent.agent_id);
}

static void process_cmd_set_goal(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    AgentSlot *agent = agent_pool_get(&ctx->agents, cmd->set_goal.agent_id);
    if (!agent)
    {
        PATIKA_INTERNAL_LOG_ERROR("Invalid agent ID for set goal: %u", cmd->set_goal.agent_id);
        return;
    }

    agent->target_q = cmd->set_goal.goal_q;
    agent->target_r = cmd->set_goal.goal_r;
    agent->behavior = BEHAVIOR_IDLE;
    agent->state = STATE_CALCULATING; // WAITING_FOR_CALC
}

static void process_cmd_add_barrack(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    AddBarrackPayload* payload = (AddBarrackPayload*)cmd->large_command.payload;

    if (!payload)
    {
        PATIKA_INTERNAL_LOG_ERROR("Unable retrieve payload. aborting...");
        free(payload);
        return;
    }
    BuildingID id = barrack_pool_allocate(&ctx->barracks);
    if (id == PATIKA_INVALID_BARRACK_ID)
    {
        PATIKA_INTERNAL_LOG_ERROR("Failed to allocate barrack");
        return;
    }
    BarrackSlot *barrack = barrack_pool_get(&ctx->barracks, id);
    if (!barrack)
    {
        PATIKA_INTERNAL_LOG_ERROR("Allocated barrack ID is invalid");
        free(payload);
        return;
    }
    barrack->pos_q = payload->pos_q;
    barrack->pos_r = payload->pos_r;
    barrack->faction = payload->faction;
    barrack->side = payload->side;
    barrack->patrol_radius = payload->patrol_radius;
    barrack->max_agents = payload->max_agents;
    barrack->behavior = payload->behavior;
    barrack->agent_count = 0;

    // NOTE: no need since event based id.
    // if (cmd->add_barrack.out_barrack_id)
    // {
    //     *cmd->add_barrack.out_barrack_id = id;
    // }
    free(payload);
}
void process_command(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    switch (cmd->type)
    {
    case CMD_ADD_AGENT:
    {
        AddAgentPayload *payload = (AddAgentPayload *)cmd->large_command.payload;

        if (!payload)
        {
            PATIKA_LOG_ERROR("ADD_AGENT command has NULL payload");
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

        // Initialize agent
        agent->pos_q = payload->start_q;
        agent->pos_r = payload->start_r;
        agent->next_q = payload->start_q;
        agent->next_r = payload->start_r;

        agent->target_q = payload->start_q;
        agent->target_r = payload->start_r;

        agent->faction = payload->faction;
        agent->side = payload->side;
        agent->parent_barrack = payload->parent_barrack;

        agent->state = STATE_IDLE;

        agent->collision_data = payload->collision_data;

        // If caller wants the ID, write it back
        if (payload->out_agent_id != NULL)
        {
            *payload->out_agent_id = agent->id;
        }

        PATIKA_LOG_DEBUG("Agent %u spawned at (%d, %d)",
                       agent->id, agent->pos_q, agent->pos_r);
        free(payload);

//        PatikaEvent evt = {EVENT_AGENT_SPAWNED, agent->id, agent->pos_q, agent->pos_r};
//        spsc_push(&ctx->event_queue, &evt);

        ctx->stats.active_agents++;
        break;
    }

    case CMD_REMOVE_AGENT:
    {
        AgentSlot *agent = agent_pool_get(&ctx->agents, cmd->remove_agent.agent_id);
        if (agent && agent->active)
        {
            agent_pool_free(&ctx->agents, cmd->remove_agent.agent_id);

            PatikaEvent evt = {EVENT_AGENT_REMOVED, cmd->remove_agent.agent_id, 0, 0};
            spsc_push(&ctx->event_queue, &evt);

            ctx->stats.active_agents--;
        }
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

        // goal validadity check
        if (!map_in_bounds(&ctx->map, cmd->set_goal.goal_q, cmd->set_goal.goal_r))
        {
            PATIKA_LOG_ERROR("SET_GOAL: position (%d, %d) out of bounds",
                           cmd->set_goal.goal_q, cmd->set_goal.goal_r);
            break;
        }

        agent->target_q = cmd->set_goal.goal_q;
        agent->target_r = cmd->set_goal.goal_r;

        agent->state = STATE_CALCULATING;

        PATIKA_LOG_INFO("Agent %u goal set to (%d, %d)",
                       agent->id, agent->target_q, agent->target_r);
        break;
    }

    case CMD_SET_TILE_STATE:
    {
        MapTile *tile = map_get(&ctx->map, cmd->set_tile.q, cmd->set_tile.r);
        if (tile)
        {
            tile->state = cmd->set_tile.state;
        }
        break;
    }

    case CMD_ADD_BARRACK:
    {
        BuildingID barr_id = barrack_pool_allocate(&ctx->barracks);
        BarrackSlot *barrack = barrack_pool_get(&ctx->barracks, barr_id);
        if (!barrack)
        {
            PATIKA_LOG_ERROR("ADD_BARRACK: barrack pool full");
            break;
        }

        // TODO: barrack data fill here
       PATIKA_LOG_WARN(
               "not implemented yet, fill data");
        break;
    }

    case CMD_REMOVE_BARRACK:
    {
//        BarrackSlot *barrack = barrack_pool_get(&ctx->barracks, cmd->remove_barrack.barrack_id);
//        if (barrack && barrack->active)
//        {
//            // Remove all agents belonging to this barrack
//            for (uint32_t i = 0; i < ctx->agents.capacity; i++)
//            {
//                AgentSlot *agent = &ctx->agents.slots[i];
//                if (agent->active && agent->parent_barrack == cmd->remove_barrack.barrack_id)
//                {
//                    agent_pool_remove(&ctx->agents, agent->id);
//                }
//            }
//
//            barrack_pool_remove(&ctx->barracks, cmd->remove_barrack.barrack_id);
//        }
//
        PATIKA_LOG_WARN("NOT IMPLEMENTED YET");
        break;
    }

    default:
        PATIKA_LOG_WARN("Unknown command type: %d", cmd->type);
        break;
    }
}


