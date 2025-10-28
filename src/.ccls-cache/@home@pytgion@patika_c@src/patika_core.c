/* Patika Core - Main Entry Point
 * Lifecycle management and public API implementation
 */

#include "../include/patika_core.h"

#include <stdlib.h>
#include <string.h>

#include "internal/patika_internal.h"

/* ============================================================================
 *  PUBLIC API: LIFECYCLE
 * ============================================================================
 */

PATIKA_API PatikaHandle patika_create(const PatikaConfig* config) {
    if (!config) return NULL;

    struct PatikaContext* ctx = calloc(1, sizeof(struct PatikaContext));
    if (!ctx) return NULL;

    ctx->config = *config;

    // Initialize subsystems
    mpsc_init(&ctx->cmd_queue, config->command_queue_size);
    spsc_init(&ctx->event_queue, config->event_queue_size);
    agent_pool_init(&ctx->agents, config->max_agents);
    barrack_pool_init(&ctx->barracks, config->max_barracks);
    map_init(&ctx->map, config->grid_width, config->grid_height);
    pcg32_init(&ctx->rng, config->rng_seed);

    // Allocate snapshot buffers
    ctx->snapshots[0].agents =
        calloc(config->max_agents, sizeof(AgentSnapshot));
    ctx->snapshots[1].agents =
        calloc(config->max_agents, sizeof(AgentSnapshot));
    ctx->snapshots[0].barracks =
        calloc(config->max_barracks, sizeof(BarrackSnapshot));
    ctx->snapshots[1].barracks =
        calloc(config->max_barracks, sizeof(BarrackSnapshot));

    atomic_init(&ctx->snapshot_index, 0);
    atomic_init(&ctx->version, 0);

    memset(&ctx->stats, 0, sizeof(PatikaStats));

    return ctx;
}

PATIKA_API void patika_destroy(PatikaHandle handle) {
    if (!handle) return;

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

PATIKA_API PatikaError patika_load_map(PatikaHandle handle,
                                       const uint8_t* map_states,
                                       uint32_t width, uint32_t height) {
    if (!handle) return PATIKA_ERR_NULL_HANDLE;
    if (!map_states) return PATIKA_ERR_NULL_HANDLE;

    for (uint32_t i = 0; i < width * height; i++) {
        handle->map.tiles[i].state = map_states[i];
    }

    return PATIKA_OK;
}

/* ============================================================================
 *  PUBLIC API: COMMAND SUBMISSION
 * ============================================================================
 */

PATIKA_API PatikaError patika_submit_command(PatikaHandle handle,
                                             const PatikaCommand* cmds,
                                             uint32_t count) {
    if (!handle) return PATIKA_ERR_NULL_HANDLE;
    for (uint32_t i = 0; i < count; i++) {
        if (mpsc_push(&handle->cmd_queue, &cmds[i] != 0)) {
            return PATIKA_ERR_QUEUE_FULL;
        }
    }
    return PATIKA_OK;
}
