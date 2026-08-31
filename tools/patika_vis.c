#include "patika_vis.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#  include <windows.h>
#  define VIS_SLEEP_MS(ms) Sleep((DWORD)(ms))
#else
#  include <unistd.h>
#  define VIS_SLEEP_MS(ms) usleep((unsigned int)(ms) * 1000u)
#endif

/* ============================================================
 *  ANSI escape codes
 * ============================================================ */
#define A_RESET    "\033[0m"
#define A_BOLD     "\033[1m"
#define A_DIM      "\033[2m"
#define A_RED      "\033[31m"
#define A_GREEN    "\033[32m"
#define A_YELLOW   "\033[33m"
#define A_BLUE     "\033[34m"
#define A_MAGENTA  "\033[35m"
#define A_CYAN     "\033[36m"
#define A_WHITE    "\033[37m"
#define A_BG_DARK  "\033[40m"

/* ============================================================
 *  Internal cell model
 * ============================================================ */
typedef enum {
    CELL_EMPTY    = 0,
    CELL_BLOCKED  = 1,
    CELL_RESERVED = 2,
    CELL_DEPOT    = 3,  /* barrack / depot */
    CELL_AGENT    = 4,
    CELL_TARGET   = 5,  /* goal tile of any active agent */
} CellType;

typedef struct {
    CellType type;
    uint8_t  agent_digit;  /* 0–9: agent pool index % 10             */
    uint8_t  agent_state;  /* AgentState of the occupying agent       */
    uint8_t  depot_digit;  /* 0–9: barrack pool index % 10           */
} Cell;

/* ============================================================
 *  State → display helpers
 * ============================================================ */
static const char *state_color(uint8_t s)
{
    switch ((AgentState)s) {
        case STATE_IDLE:         return A_WHITE;
        case STATE_CALCULATING:  return A_YELLOW;
        case STATE_MOVING:       return A_GREEN;
        case STATE_INTERACTING:  return A_MAGENTA;
        case STATE_REMOVE_QUEUE: return A_RED;
        default:                 return A_DIM;
    }
}

static char state_char(uint8_t s)
{
    switch ((AgentState)s) {
        case STATE_IDLE:         return 'i';
        case STATE_CALCULATING:  return 'c';
        case STATE_MOVING:       return 'm';
        case STATE_INTERACTING:  return 'x';
        case STATE_REMOVE_QUEUE: return '!';
        default:                 return '?';
    }
}

static const char *state_name(uint8_t s)
{
    switch ((AgentState)s) {
        case STATE_IDLE:         return "IDLE";
        case STATE_CALCULATING:  return "CALC";
        case STATE_MOVING:       return "MOVE";
        case STATE_INTERACTING:  return "INTR";
        case STATE_REMOVE_QUEUE: return "REMV";
        default:                 return "????";
    }
}

/* ============================================================
 *  Public API
 * ============================================================ */

void patika_vis_clear(void)
{
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
}

void patika_vis_render(struct PatikaContext *ctx)
{
    if (!ctx) return;

    MapGrid *map = &ctx->map;

    if (map->type != MAP_TYPE_RECTANGULAR) {
        printf("[patika_vis] hex grid not supported yet — use MAP_TYPE_RECTANGULAR\n");
        return;
    }

    uint32_t W = map->width  < PATIKA_VIS_MAX_W ? map->width  : PATIKA_VIS_MAX_W;
    uint32_t H = map->height < PATIKA_VIS_MAX_H ? map->height : PATIKA_VIS_MAX_H;

    /* ---- build display buffer (stack allocated, fixed size) ---- */
    static Cell cells[PATIKA_VIS_MAX_H][PATIKA_VIS_MAX_W];
    memset(cells, 0, sizeof(cells));

    /* Pass 1: tile walkability */
    for (uint32_t r = 0; r < H; r++) {
        for (uint32_t q = 0; q < W; q++) {
            MapTile *tile = map_get(map, (int32_t)q, (int32_t)r);
            cells[r][q].type = (tile && tile->state == 0) ? CELL_EMPTY : CELL_BLOCKED;
        }
    }

    /* Pass 2: tile reservations (only overwrite EMPTY) */
    for (uint32_t r = 0; r < H; r++) {
        for (uint32_t q = 0; q < W; q++) {
            if (cells[r][q].type != CELL_EMPTY) continue;
            uint32_t gv = map_get_agent_grid(map, (int32_t)q, (int32_t)r);
            if (map_is_tile_reserved(gv)) {
                cells[r][q].type = CELL_RESERVED;
            }
        }
    }

    /* Pass 3: depots / barracks */
    for (uint16_t i = 0; i < ctx->barracks.next_id; i++) {
        BarrackSlot *b = &ctx->barracks.slots[i];
        if (!b->active) continue;
        int32_t q = b->pos_q, r = b->pos_r;
        if (q < 0 || q >= (int32_t)W || r < 0 || r >= (int32_t)H) continue;
        cells[r][q].type        = CELL_DEPOT;
        cells[r][q].depot_digit = (uint8_t)(i % 10);
    }

    /* Pass 4: agent target markers (only on EMPTY tiles) */
    for (uint32_t i = 0; i < ctx->agents.capacity; i++) {
        AgentSlot *a = &ctx->agents.slots[i];
        if (!a->active) continue;
        int32_t tq = a->target_q, tr = a->target_r;
        if (tq == a->pos_q && tr == a->pos_r) continue; /* at goal */
        if (tq < 0 || tq >= (int32_t)W || tr < 0 || tr >= (int32_t)H) continue;
        if (cells[tr][tq].type == CELL_EMPTY) {
            cells[tr][tq].type = CELL_TARGET;
        }
    }

    /* Pass 5: agents (highest priority — always overwrite) */
    for (uint32_t i = 0; i < ctx->agents.capacity; i++) {
        AgentSlot *a = &ctx->agents.slots[i];
        if (!a->active) continue;
        int32_t q = a->pos_q, r = a->pos_r;
        if (q < 0 || q >= (int32_t)W || r < 0 || r >= (int32_t)H) continue;
        cells[r][q].type        = CELL_AGENT;
        cells[r][q].agent_digit = (uint8_t)(i % 10);
        cells[r][q].agent_state = a->state;
    }

    /* ---- render ---- */

    /* Stats header */
    printf(A_BOLD "PATIKA VIS" A_RESET
           "  tick:" A_CYAN "%-6llu" A_RESET
           "  robots:" A_GREEN "%u" A_RESET "/%u"
           "  depots:%u"
           "  cmds:" A_DIM "%llu" A_RESET
           "  evts:" A_DIM "%llu" A_RESET "\n",
           (unsigned long long)ctx->stats.total_ticks,
           ctx->agents.active_count,
           ctx->agents.capacity,
           ctx->barracks.next_id,
           (unsigned long long)ctx->stats.commands_processed,
           (unsigned long long)ctx->stats.events_emitted);

    if (map->width > PATIKA_VIS_MAX_W || map->height > PATIKA_VIS_MAX_H) {
        printf(A_YELLOW "[clipped to %ux%u of %ux%u]" A_RESET "\n",
               W, H, map->width, map->height);
    }

    /* Column header — tick mark every 5 */
    printf("     ");
    for (uint32_t q = 0; q < W; q++) {
        if (q % 5 == 0)
            printf(A_DIM "%-10u" A_RESET, q);  /* 10 = 5 cols × 2 chars/cell */
    }
    printf("\n");

    /* Top border */
    printf("    +");
    for (uint32_t q = 0; q < W; q++) printf("--");
    printf("+\n");

    /* Grid rows */
    for (uint32_t r = 0; r < H; r++) {
        printf(A_DIM "%3u " A_RESET "|", r);

        for (uint32_t q = 0; q < W; q++) {
            Cell *c = &cells[r][q];
            switch (c->type) {
                case CELL_EMPTY:
                    printf(A_DIM ". " A_RESET);
                    break;

                case CELL_BLOCKED:
                    printf(A_WHITE A_BOLD "##" A_RESET);
                    break;

                case CELL_RESERVED:
                    printf(A_CYAN "~~" A_RESET);
                    break;

                case CELL_TARGET:
                    printf(A_BLUE "TT" A_RESET);
                    break;

                case CELL_DEPOT:
                    printf(A_MAGENTA A_BOLD "D%u" A_RESET, c->depot_digit);
                    break;

                case CELL_AGENT:
                    printf("%s" A_BOLD "%u%c" A_RESET,
                           state_color(c->agent_state),
                           c->agent_digit,
                           state_char(c->agent_state));
                    break;
            }
        }
        printf("|\n");
    }

    /* Bottom border */
    printf("    +");
    for (uint32_t q = 0; q < W; q++) printf("--");
    printf("+\n");

    /* Legend */
    printf("\n"
           "  " A_DIM ". " A_RESET " empty   "
           A_WHITE A_BOLD "##" A_RESET " wall    "
           A_CYAN "~~" A_RESET " reserved  "
           A_BLUE "TT" A_RESET " target    "
           A_MAGENTA A_BOLD "Dn" A_RESET " depot(n)\n"
           "  robot(n,state): "
           A_WHITE A_BOLD "ni" A_RESET " idle  "
           A_YELLOW A_BOLD "nc" A_RESET " calc  "
           A_GREEN  A_BOLD "nm" A_RESET " move  "
           A_MAGENTA A_BOLD "nx" A_RESET " interact  "
           A_RED A_BOLD "n!" A_RESET " remove\n\n");

    fflush(stdout);
}

void patika_vis_print_agents(struct PatikaContext *ctx)
{
    if (!ctx) return;

    printf(A_BOLD "  ID     idx  pos       next      target    state    behavior\n" A_RESET);
    printf("  ----   ---  --------  --------  --------  -------  --------\n");

    uint32_t printed = 0;
    for (uint32_t i = 0; i < ctx->agents.capacity; i++) {
        AgentSlot *a = &ctx->agents.slots[i];
        if (!a->active) continue;

        const char *beh = "?";
        switch ((AgentBehavior)a->behavior) {
            case BEHAVIOR_IDLE:    beh = "idle";    break;
            case BEHAVIOR_PATROL:  beh = "patrol";  break;
            case BEHAVIOR_EXPLORE: beh = "explore"; break;
            case BEHAVIOR_GUARD:   beh = "guard";   break;
            case BEHAVIOR_FLEE:    beh = "flee";    break;
        }

        printf("  %s%04x%s   %-3u  (%3d,%3d)  (%3d,%3d)  (%3d,%3d)  %s%-7s%s  %s\n",
               A_CYAN, a->id, A_RESET,
               i,
               a->pos_q,    a->pos_r,
               a->next_q,   a->next_r,
               a->target_q, a->target_r,
               state_color(a->state), state_name(a->state), A_RESET,
               beh);
        printed++;
    }

    if (printed == 0) {
        printf("  " A_DIM "(no active agents)" A_RESET "\n");
    }
    printf("\n");
    fflush(stdout);
}

void patika_vis_step(struct PatikaContext *ctx, int delay_ms)
{
    patika_vis_clear();
    patika_tick(ctx);
    patika_vis_render(ctx);
    patika_vis_print_agents(ctx);
    if (delay_ms > 0) {
        VIS_SLEEP_MS(delay_ms);
    }
}
