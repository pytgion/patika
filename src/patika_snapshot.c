#include "internal/patika_internal.h"

void update_snapshot(struct PatikaContext *ctx)
{
    uint32_t idx = (atomic_load(&ctx->snapshot_index) + 1) % 2;
    PatikaSnapshot *snap = &ctx->snapshots[idx];

    snap->agent_count = ctx->agents.active_count;
    for (uint32_t i = 0; i < ctx->agents.capacity; i++)
    {
        AgentSlot *slot = &ctx->agents.slots[i];
        if (!slot->active)
            continue;

        AgentSnapshot *a = &snap->agents[snap->agent_count++];
        a->id = slot->id;
        a->state = slot->state;
        a->faction = slot->faction;
        a->side = slot->side;
        a->parent_barrack = slot->parent_barrack;
        a->pos_q = slot->pos_q;
        a->pos_r = slot->pos_r;
        a->next_q = slot->next_q;
        a->next_r = slot->next_r;
        a->target_q = slot->target_q;
        a->target_r = slot->target_r;
    }
    for (uint16_t i = 0; i < ctx->barracks.next_id; i++)
    {
        BarrackSlot *slot = &ctx->barracks.slots[i];
        if (!slot->active)
            continue;

        BarrackSnapshot *b = &snap->barracks[snap->barrack_count++];
        b->id = slot->id;
        b->state = slot->state;
        b->faction = slot->faction;
        b->side = slot->side;
        b->pos_q = slot->pos_q;
        b->pos_r = slot->pos_r;
        b->patrol_radius = slot->patrol_radius;
        b->agent_count = slot->agent_count;
    }
    snap->barrack_count = ctx->barracks.next_id;

    snap->version = atomic_fetch_add(&ctx->version, 1) + 1;
    atomic_store(&ctx->snapshot_index, idx);
}
