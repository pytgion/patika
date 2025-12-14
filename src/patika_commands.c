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

    // 1. Havuzdan yer al
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
        if (payload->parent_barrack != PATIKA_INVALID_BARRACK_ID) {
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
        process_cmd_add_agent(ctx, cmd);
        break;
    case CMD_ADD_AGENT_WITH_BEHAVIOR:
        process_cmd_add_agent_behavior(ctx, cmd);
        break;
    case CMD_SET_GOAL:
        process_cmd_set_goal(ctx, cmd);
        break;
    case CMD_ADD_BARRACK:
        process_cmd_add_barrack(ctx, cmd);
        break;
    case CMD_SET_TILE_STATE:
        map_set_tile_state(&ctx->map, cmd->set_tile.q, cmd->set_tile.r, cmd->set_tile.state);
        break;
    case CMD_REMOVE_AGENT:
        process_cmd_remove_agent(ctx, cmd);
        break;
    case CMD_BIND_BARRACK:
        PATIKA_LOG_WARN("CMD_BIND_BARRACK not implemented yet");
        break;
    case CMD_COMPUTE_NEXT:
        PATIKA_LOG_WARN("CMD_COMPUTE_NEXT not implemented yet");
        break;
    case CMD_ADD_BUILDING:
        PATIKA_LOG_WARN("CMD_ADD_BUILDING not implemented yet");
        break;
    default:
        PATIKA_INTERNAL_LOG_ERROR("Unhandled command type: %d", cmd->type);
        break;
    }
    ctx->stats.commands_processed++;
}
