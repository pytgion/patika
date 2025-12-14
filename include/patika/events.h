#ifndef PATIKA_EVENTS_H
#define PATIKA_EVENTS_H

#include "types.h"
#include "enums.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
    #endif

    /**
     * @brief Event payload emitted by the simulation
     */
    typedef struct
    {
        EventType type;
        AgentID agent_id;
        int32_t pos_q, pos_r;
    } PatikaEvent;

    #ifdef __cplusplus
}
#endif

#endif /* PATIKA_EVENTS_H */
