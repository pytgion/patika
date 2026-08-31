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

#ifdef __cplusplus
}
#endif

#endif /* PATIKA_API_H */
