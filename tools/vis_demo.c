/**
 * @file vis_demo.c
 * @brief Quick visual smoke test — 3 robots on a 20x15 rectangular grid.
 *
 * Build (from repo root):
 *   cc -std=c11 -DPATIKA_DEBUG \
 *      src/patika_core.c src/patika_pool.c src/patika_map.c \
 *      src/patika_mpsc.c src/patika_spsc.c src/patika_snapshot.c \
 *      src/patika_pathfinding.c src/patika_movement.c \
 *      src/patika_collision.c src/patika_rng.c src/patika_log.c \
 *      src/patika_utility.c \
 *      tools/patika_vis.c tools/vis_demo.c \
 *      -Iinclude -o vis_demo && ./vis_demo
 */

#include "patika_vis.h"
#include <stdlib.h>
#include <stdio.h>

#define MAP_W 20
#define MAP_H 15
#define MAX_AGENTS 8
#define MAX_BARRACKS 4
#define CMD_QUEUE 64
#define EVT_QUEUE 64
#define TICKS 120
#define DELAY_MS 120

int main(void)
{
    PatikaConfig cfg = {
        .grid_type          = MAP_TYPE_RECTANGULAR,
        .max_agents         = MAX_AGENTS,
        .max_barracks       = MAX_BARRACKS,
        .grid_width         = MAP_W,
        .grid_height        = MAP_H,
        .command_queue_size = CMD_QUEUE,
        .event_queue_size   = EVT_QUEUE,
        .rng_seed           = 0xDEADBEEF,
    };

    PatikaHandle h = patika_create(&cfg);
    if (!h) {
        fprintf(stderr, "patika_create failed\n");
        return 1;
    }

    /* --- build a map: some walls forming a corridor --- */
    /* horizontal wall at row 5, cols 3..12 with a gap at col 8 */
    for (int q = 3; q <= 12; q++) {
        if (q == 8) continue; /* gap */
        PatikaCommand wall = { .type = CMD_SET_TILE_STATE };
        wall.set_tile.q     = q;
        wall.set_tile.r     = 5;
        wall.set_tile.state = 1; /* blocked */
        patika_submit_command(h, &wall);
    }
    /* vertical wall at col 14, rows 2..10 with a gap at row 6 */
    for (int r = 2; r <= 10; r++) {
        if (r == 6) continue;
        PatikaCommand wall = { .type = CMD_SET_TILE_STATE };
        wall.set_tile.q     = 14;
        wall.set_tile.r     = r;
        wall.set_tile.state = 1;
        patika_submit_command(h, &wall);
    }
    /* flush wall commands */
    patika_tick(h);

    /* --- place a depot --- */
    AddBarrackPayload *dep = (AddBarrackPayload *)malloc(sizeof(AddBarrackPayload));
    if (dep) {
        dep->pos_q         = 1;
        dep->pos_r         = 1;
        dep->faction       = 0;
        dep->side          = 0;
        dep->behavior      = BEHAVIOR_IDLE;
        dep->patrol_radius = 0;
        dep->max_agents    = MAX_AGENTS;
        dep->out_barrack_id = NULL;
        PatikaCommand dc = { .type = CMD_ADD_BARRACK };
        dc.large_command.payload = dep;
        patika_submit_command(h, &dc);
        patika_tick(h);
    }

    /* --- spawn robots and set goals --- */
    typedef struct { int sq; int sr; int gq; int gr; } RobotSpec;
    RobotSpec robots[] = {
        { 0,  0, 19, 14 },
        { 0,  7, 19,  0 },
        { 0, 14,  9,  2 },
    };
    int nrobots = (int)(sizeof(robots) / sizeof(robots[0]));

    AgentID ids[8] = {0};

    for (int i = 0; i < nrobots; i++) {
        AddAgentPayload *p = (AddAgentPayload *)malloc(sizeof(AddAgentPayload));
        if (!p) continue;
        p->start_q        = robots[i].sq;
        p->start_r        = robots[i].sr;
        p->faction        = 0;
        p->side           = 0;
        p->parent_barrack = 0;
        p->out_agent_id   = &ids[i];
        p->collision_data.layer          = 1;
        p->collision_data.collision_mask = 1;
        p->collision_data.aggression_mask = 0;

        PatikaCommand cmd = { .type = CMD_ADD_AGENT };
        cmd.large_command.payload = p;
        patika_submit_command(h, &cmd);
    }
    patika_tick(h); /* process spawns */

    /* set goals now that IDs are written back */
    for (int i = 0; i < nrobots; i++) {
        if (ids[i] == PATIKA_INVALID_AGENT_ID) continue;
        PatikaCommand goal = { .type = CMD_SET_GOAL };
        goal.set_goal.agent_id = ids[i];
        goal.set_goal.goal_q   = robots[i].gq;
        goal.set_goal.goal_r   = robots[i].gr;
        patika_submit_command(h, &goal);
    }

    /* --- main animation loop --- */
    for (int t = 0; t < TICKS; t++) {
        patika_vis_step(h, DELAY_MS);

        /* drain events and print them */
        PatikaEvent evts[16];
        uint32_t n = patika_poll_events(h, evts, 16);
        for (uint32_t e = 0; e < n; e++) {
            const char *ename = "?";
            switch (evts[e].type) {
                case EVENT_REACHED_GOAL:  ename = "REACHED_GOAL";  break;
                case EVENT_STUCK:         ename = "STUCK";         break;
                case EVENT_BLOCKED:       ename = "BLOCKED";       break;
                case EVENT_REPLAN_NEEDED: ename = "REPLAN_NEEDED"; break;
                case EVENT_AGENT_REMOVED: ename = "AGENT_REMOVED"; break;
            }
            printf("  event: agent %04x  %s  @ (%d,%d)\n",
                   evts[e].agent_id, ename, evts[e].pos_q, evts[e].pos_r);
        }
        if (n > 0) fflush(stdout);
    }

    printf("\nDone. %llu ticks simulated.\n",
           (unsigned long long)patika_get_stats(h).total_ticks);

    patika_destroy(h);
    return 0;
}
