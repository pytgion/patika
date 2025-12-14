#ifndef PATIKA_CONFIG_H
#define PATIKA_CONFIG_H

#include "types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
    #endif

    /**
     * @brief Engine configuration at creation time
     */
    typedef struct
    {
        uint8_t grid_type;           /**< Map type identifier */
        uint32_t max_agents;         /**< Agent pool capacity */
        uint16_t max_barracks;       /**< Barrack pool capacity */
        uint32_t grid_width;         /**< Map width in cells (q axis) */
        uint32_t grid_height;        /**< Map height in cells (r axis) */
        uint32_t sector_size;        /**< Optional sector side length */
        uint32_t command_queue_size; /**< MPSC command queue capacity */
        uint32_t event_queue_size;   /**< SPSC event queue capacity */
        uint64_t rng_seed;           /**< RNG seed */
    } PatikaConfig;

    #ifdef __cplusplus
}
#endif

#endif /* PATIKA_CONFIG_H */
