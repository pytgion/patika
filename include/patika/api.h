#ifndef PATIKA_API_H
#define PATIKA_API_H

#include "types.h"
#include "enums.h"
#include "config.h"
#include "commands/agent.h"
#include "commands/barrack.h"
#include "commands/base.h"
#include "commands/guard.h"
#include "events.h"
#include "snapshot.h"

#ifdef __cplusplus
extern "C" {
    #endif

    PATIKA_API PatikaHandle patika_create(const PatikaConfig *config);
    PATIKA_API void patika_destroy(PatikaHandle handle);


    PATIKA_API PatikaError patika_submit_command(
        PatikaHandle handle,
        const PatikaCommand *cmd
    );

    PATIKA_API PatikaError patika_submit_commands(
        PatikaHandle handle,
        const PatikaCommand *cmds,
        uint32_t count
    );


    PATIKA_API void patika_tick(PatikaHandle handle);

    PATIKA_API uint32_t patika_poll_events(
        PatikaHandle handle,
        PatikaEvent *out_events,
        uint32_t max_events
    );


    PATIKA_API const PatikaSnapshot *patika_get_snapshot(PatikaHandle handle);
    PATIKA_API PatikaStats patika_get_stats(PatikaHandle handle);

    // it still pushes command to queue (use payloads instead)
//    PATIKA_API PatikaError patika_add_agent_sync(
//        PatikaHandle handle,
//        int32_t start_q,
//        int32_t start_r,
//        uint8_t faction,
//        uint8_t side,
//        BuildingID parent_barrack,
//        AgentID *id_output
//    );


    #ifdef __cplusplus
}
#endif

#endif /* PATIKA_API_H */
