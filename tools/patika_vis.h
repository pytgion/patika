#ifndef PATIKA_VIS_H
#define PATIKA_VIS_H

/**
 * @file patika_vis.h
 * @brief Terminal visualizer for Patika simulation state.
 *
 * TEST TOOL ONLY. Accesses internal context directly.
 * Do NOT include this in production builds.
 *
 * Dependencies: none beyond stdio.h — uses ANSI escape codes.
 * Requires a terminal that supports ANSI (macOS Terminal, iTerm2, Linux).
 */

#include "../src/internal/patika_internal.h"

/* Maximum grid dimensions the visualizer will render.
 * Cells beyond this are silently clipped.
 * Each cell is 2 chars wide, so VIS_MAX_W=60 needs ~130 terminal columns. */
#ifndef PATIKA_VIS_MAX_W
#define PATIKA_VIS_MAX_W 60
#endif

#ifndef PATIKA_VIS_MAX_H
#define PATIKA_VIS_MAX_H 35
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Render the full simulation state to stdout.
 *
 * Prints: stats header, grid (tiles + agents + barracks + reservations),
 * and a legend. Does NOT clear the screen — call patika_vis_clear() first
 * if you want live animation.
 *
 * @param ctx Direct context pointer. Must not be NULL.
 */
void patika_vis_render(struct PatikaContext *ctx);

/**
 * @brief Clear the terminal screen and move cursor to home.
 */
void patika_vis_clear(void);

/**
 * @brief Run one tick then render. Convenience for test loops.
 *
 * @param ctx   Simulation context.
 * @param delay_ms Milliseconds to sleep after rendering (0 = no sleep).
 */
void patika_vis_step(struct PatikaContext *ctx, int delay_ms);

/**
 * @brief Print a compact agent roster below the grid.
 *  Shows each active agent: ID, position, state, target.
 */
void patika_vis_print_agents(struct PatikaContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* PATIKA_VIS_H */
