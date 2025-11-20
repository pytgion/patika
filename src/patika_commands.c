#include "internal/patika_internal.h"

static void process_cmd_add_agent(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    AgentID id = agent_pool_allocate(&ctx->agents);
    if (id == PATIKA_INVALID_AGENT_ID)
        return;

    AgentSlot *agent = agent_pool_get(&ctx->agents, id);
    if (!agent)
        return;

    agent->pos_q = cmd->add_agent.start_q;
    agent->pos_r = cmd->add_agent.start_r;
    agent->faction = cmd->add_agent.faction;
    agent->side = cmd->add_agent.side;
    agent->parent_barrack = cmd->add_agent.parent_barrack;
    agent->state = 0;

    if (cmd->add_agent.out_agent_id)
    {
        *cmd->add_agent.out_agent_id = id;
    }
}

static void process_cmd_remove_agent(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    AgentSlot *agent = agent_pool_get(&ctx->agents, cmd->remove_agent.agent_id);
    if (!agent)
    {
        //    PATIKA_INTERNAL_LOG_ERROR("Invalid agent ID for removal: %u", cmd->remove_agent.agent_id);
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
    agent->state = 1; // WAITING_FOR_CALC
}

static void process_cmd_add_barrack(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    BarrackID id = barrack_pool_allocate(&ctx->barracks);
    if (id == PATIKA_INVALID_BARRACK_ID)
    {
        PATIKA_INTERNAL_LOG_ERROR("Failed to allocate barrack");
        return;
    }
    BarrackSlot *barrack = barrack_pool_get(&ctx->barracks, id);
    if (!barrack)
    {
        PATIKA_INTERNAL_LOG_ERROR("Allocated barrack ID is invalid");
        return;
    }
    barrack->pos_q = cmd->add_barrack.pos_q;
    barrack->pos_r = cmd->add_barrack.pos_r;
    barrack->faction = cmd->add_barrack.faction;
    barrack->side = cmd->add_barrack.side;
    barrack->patrol_radius = cmd->add_barrack.patrol_radius;
    barrack->max_agents = cmd->add_barrack.max_agents;
    barrack->agent_count = 0;
    if (cmd->add_barrack.out_barrack_id)
    {
        *cmd->add_barrack.out_barrack_id = id;
    }
}
void process_command(struct PatikaContext *ctx, const PatikaCommand *cmd)
{
    switch (cmd->type)
    {
    case CMD_ADD_AGENT:
        process_cmd_add_agent(ctx, cmd);
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
