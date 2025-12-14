#include "../include/patika.h"
#include "../include/patika/patika_log.h"
#include "internal/patika_internal.h"
#include <stdlib.h>
#include <string.h>

const uint32_t PATIKA_INVALID_AGENT_ID = 0xFFFF;
const uint16_t PATIKA_INVALID_BARRACK_ID = 0xFFFF;


PATIKA_API PatikaHandle patika_create(const PatikaConfig *config)
{
    if (!config)
    {
        return NULL;
    }
    PATIKA_LOG_INFO("Creating patika context %u agents %ux%u map",
                    config->max_agents,
                    config->grid_width,
                    config->grid_height);

    struct PatikaContext *ctx = calloc(1, sizeof(struct PatikaContext));
    if (!ctx)
    {
        PATIKA_LOG_ERROR("Failed to allocate context");
        return NULL;
    }

    ctx->config = *config;

    // Initialize subsystems
    mpsc_init(&ctx->cmd_queue, config->command_queue_size);
    spsc_init(&ctx->event_queue, config->event_queue_size);
    agent_pool_init(&ctx->agents, config->max_agents);
    barrack_pool_init(&ctx->barracks, config->max_barracks);
    map_init(&ctx->map, config->grid_type, config->grid_width, config->grid_height);
    pcg32_init(&ctx->rng, config->rng_seed);

    // Allocate snapshot buffers
    ctx->snapshots[0].agents = calloc(config->max_agents, sizeof(AgentSnapshot));
    ctx->snapshots[1].agents = calloc(config->max_agents, sizeof(AgentSnapshot));
    ctx->snapshots[0].barracks = calloc(config->max_barracks, sizeof(BarrackSnapshot));
    ctx->snapshots[1].barracks = calloc(config->max_barracks, sizeof(BarrackSnapshot));

    atomic_init(&ctx->snapshot_index, 0);
    atomic_init(&ctx->version, 0);

    memset(&ctx->stats, 0, sizeof(PatikaStats));

    return ctx;
}

PATIKA_API void patika_destroy(PatikaHandle handle)
{
    if (!handle)
        return;

    mpsc_destroy(&handle->cmd_queue);
    spsc_destroy(&handle->event_queue);
    agent_pool_destroy(&handle->agents);
    barrack_pool_destroy(&handle->barracks);
    map_destroy(&handle->map);

    free(handle->snapshots[0].agents);
    free(handle->snapshots[1].agents);
    free(handle->snapshots[0].barracks);
    free(handle->snapshots[1].barracks);

    free(handle);
}

PATIKA_API PatikaError patika_load_map(PatikaHandle handle, const uint8_t *map_states, uint32_t width, uint32_t height)
{
    if (!handle)
        return PATIKA_ERR_NULL_HANDLE;
    if (!map_states)
        return PATIKA_ERR_NULL_HANDLE;

    for (uint32_t i = 0; i < width * height; i++)
    {
        handle->map.tiles[i].state = map_states[i];
    }

    return PATIKA_OK;
}

PATIKA_API PatikaError patika_submit_command(PatikaHandle handle, const PatikaCommand *cmd)
{
    if (!handle)
    {
        return PATIKA_ERR_NULL_HANDLE;
    }
    return mpsc_push(&handle->cmd_queue, cmd) == 0 ? PATIKA_OK : PATIKA_ERR_QUEUE_FULL;
}

PATIKA_API PatikaError patika_submit_commands(PatikaHandle handle, const PatikaCommand *cmds, uint32_t count)
{
    if (!handle)
        return PATIKA_ERR_NULL_HANDLE;

    for (uint32_t i = 0; i < count; i++)
    {
        if (mpsc_push(&handle->cmd_queue, &cmds[i]) != 0)
        {
            return PATIKA_ERR_QUEUE_FULL;
        }
    }
    return PATIKA_OK;
}

PATIKA_API void patika_tick(PatikaHandle handle)
{
    if (!handle)
        return;

    // process all pending commands
    PatikaCommand cmd;
    while (mpsc_pop(&handle->cmd_queue, &cmd) == 0)
    {
        process_command(handle, &cmd);
    }

    for (uint32_t i = 0; i < handle->agents.capacity; i++)
    {
        AgentSlot *agent = &handle->agents.slots[i];
        if (!agent->active)
        {
            continue;
        }

        if (agent->state == STATE_CALCULATING)
        { // WAITING_FOR_CALC
            compute_next_step(handle, agent);
        }
        else if (agent->state == STATE_MOVING)
        {
            process_movement(handle, agent);
        }
    }

    update_snapshot(handle);

    handle->stats.total_ticks++;
    handle->stats.active_agents = handle->agents.active_count;
}

PATIKA_API uint32_t patika_poll_events(PatikaHandle handle, PatikaEvent *out_events, uint32_t max_events)
{
    if (!handle)
    {
        return 0;
    }

    uint32_t count = 0;
    while (count < max_events && spsc_pop(&handle->event_queue, &out_events[count]) == 0)
    {
        count++;
    }

    handle->stats.events_emitted += count;
    return count;
}

PATIKA_API const PatikaSnapshot *patika_get_snapshot(PatikaHandle handle)
{
    if (!handle)
        return NULL;

    uint32_t idx = atomic_load(&handle->snapshot_index);
    return &handle->snapshots[idx];
}

PATIKA_API PatikaStats patika_get_stats(PatikaHandle handle)
{
    if (!handle)
    {
        PatikaStats empty = {0};
        return empty;
    }
    return handle->stats;
}


PATIKA_API PatikaError patika_add_agent_sync(PatikaHandle handle,
                                             int32_t start_q,
                                             int32_t start_r,
                                             uint8_t faction,
                                             uint8_t side,
                                             BuildingID parent_barrack,
                                             AgentID *id_output)
{
    if (!handle)
        return PATIKA_ERR_NULL_HANDLE;

    AddAgentPayload *payload = (AddAgentPayload*)malloc(sizeof(AddAgentPayload));
    if (!payload)
    {
        return PATIKA_ERR_INVALID_COMMAND_TYPE;
    }

    payload->start_q = start_q;
    payload->start_r = start_r;
    payload->faction = faction;
    payload->side = side;
    payload->parent_barrack = parent_barrack;
    payload->out_agent_id = id_output;

    PatikaCommand cmd = {0};
    cmd.type = CMD_ADD_AGENT;
    cmd.large_command.payload = payload;

    PatikaError err = patika_submit_command(handle, &cmd);

    // fucked up situation, evacuate the data
    if (err != PATIKA_OK)
    {
        free(payload);
    }

    return err;
}
