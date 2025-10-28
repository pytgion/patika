#include "internal/patika_internal.h"

static void process_cmd_add_agent(struct PatikaContext* ctx, const PatikaCommand* cmd) {
    AgentID id = agent_pool_allocate(&ctx->agents);
    if (id == PATIKA_INVALID_AGENT_ID) return;
    
    AgentSlot* agent = agent_pool_get(&ctx->agents, id);
    if (!agent) return;
    
    agent->pos_q = cmd->add_agent.start_q;
    agent->pos_r = cmd->add_agent.start_r;
    agent->faction = cmd->add_agent.faction;
    agent->side = cmd->add_agent.side;
    agent->parent_barrack = cmd->add_agent.parent_barrack;
    agent->state = 0;
    
    if (cmd->add_agent.out_agent_id) {
        *cmd->add_agent.out_agent_id = id;
    }
}

static void process_cmd_set_goal(struct PatikaContext* ctx, const PatikaCommand* cmd) {
    AgentSlot* agent = agent_pool_get(&ctx->agents, cmd->set_goal.agent_id);
    if (!agent) return;
    
    agent->target_q = cmd->set_goal.goal_q;
    agent->target_r = cmd->set_goal.goal_r;
    agent->state = 1;  // WAITING_FOR_CALC
}

void process_command(struct PatikaContext* ctx, const PatikaCommand* cmd) {
    switch (cmd->type) {
        case CMD_ADD_AGENT: 
            process_cmd_add_agent(ctx, cmd); 
            break;
        case CMD_SET_GOAL: 
            process_cmd_set_goal(ctx, cmd); 
            break;
        default: 
            break;
    }
    ctx->stats.commands_processed++;
}
