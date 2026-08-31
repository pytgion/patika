#ifndef PATIKA_API_H
#define PATIKA_API_H

#include "types.h"
#include "enums.h"
#include "config.h"
#include "commands/agent.h"
#include "commands/barrack.h"
#include "commands/base.h"
#include "events.h"
#include "snapshot.h"

/*
 * Thread-safety contract
 * ──────────────────────
 * Each PatikaHandle is an independent simulation context.  Multiple handles
 * may coexist in the same process with no shared mutable state between them
 * (the only process-wide state is the log mutex, which is safe from any thread).
 *
 * Per-handle rules:
 *
 *   patika_submit_command / patika_submit_commands
 *       Safe to call from multiple threads simultaneously (MPSC queue).
 *
 *   patika_tick
 *       Must be called from exactly ONE thread per handle.  All simulation
 *       state (agent positions, pathfinding scratch, stats) is mutated here.
 *
 *   patika_get_snapshot
 *       Safe from any thread; reads an atomic index and returns a pointer to
 *       the double-buffered snapshot that tick is NOT currently writing.
 *
 *   patika_poll_events
 *       Safe from exactly ONE consumer thread per handle (SPSC queue).
 *       Do not call from multiple threads simultaneously.
 *
 *   patika_get_stats
 *       Returns a struct by value; safe to call from any thread, but the
 *       values are only coherent when read from the tick thread or after a
 *       happens-before edge (e.g. after joining the tick thread).
 *
 *   patika_create / patika_destroy / patika_load_map
 *       Not thread-safe; call only when no other thread holds the handle.
 */

#ifdef __cplusplus
extern "C" {
#endif

    PATIKA_API PatikaHandle patika_create(const PatikaConfig *config);
    PATIKA_API void         patika_destroy(PatikaHandle handle);

    PATIKA_API PatikaError patika_submit_command (PatikaHandle handle, const PatikaCommand *cmd);
    PATIKA_API PatikaError patika_submit_commands(PatikaHandle handle, const PatikaCommand *cmds, uint32_t count);

    PATIKA_API void patika_tick(PatikaHandle handle);

    PATIKA_API uint32_t patika_poll_events(PatikaHandle handle, PatikaEvent *out_events, uint32_t max_events);

    PATIKA_API const PatikaSnapshot *patika_get_snapshot(PatikaHandle handle);
    PATIKA_API PatikaStats           patika_get_stats   (PatikaHandle handle);

    PATIKA_API PatikaError patika_load_map(PatikaHandle handle, const uint8_t *map_states, uint32_t width, uint32_t height);

    /*
     * Flow fields
     *
     * Compute a BFS flow field toward (goal_q, goal_r) given the current map
     * obstacle state.  Up to PATIKA_MAX_FLOW_FIELDS (16) can be live at once.
     * Multiple agents may share the same flow field ID — assign it via
     * CMD_SET_FLOW_FIELD.
     *
     * patika_rebuild_flow_field: recomputes an existing slot in-place (e.g.
     * after obstacle changes).  Use when agents are already following a field
     * whose map has changed.
     */
    PATIKA_API PatikaError patika_compute_flow_field  (PatikaHandle handle, int32_t goal_q, int32_t goal_r, FlowFieldID *out_id);
    PATIKA_API void        patika_free_flow_field      (PatikaHandle handle, FlowFieldID id);
    PATIKA_API PatikaError patika_rebuild_flow_field   (PatikaHandle handle, FlowFieldID id);

    /*
     * Sector system
     *
     * Sectors partition the map into square chunks of config.sector_size tiles.
     * Rebuilding recomputes the adjacency graph — call after bulk tile-state
     * changes.  patika_create calls this automatically when sector_size > 0.
     *
     * Only supported for MAP_TYPE_RECTANGULAR.
     */
    PATIKA_API PatikaError patika_rebuild_sectors(PatikaHandle handle);

#ifdef __cplusplus
}
#endif

#endif /* PATIKA_API_H */
