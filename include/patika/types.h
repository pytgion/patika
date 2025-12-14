#ifndef PATIKA_TYPES_H
#define PATIKA_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
    #endif

    /* ============================================================================
     * API Export Macros
     * ========================================================================= */
    #ifdef _WIN32
    #ifdef PATIKA_EXPORTS
    #define PATIKA_API __declspec(dllexport)
    #else
    #define PATIKA_API __declspec(dllimport)
    #endif
    #else
    #if defined(__GNUC__) || defined(__clang__)
    #ifdef PATIKA_EXPORTS
    #define PATIKA_API __attribute__((visibility("default")))
    #else
    #define PATIKA_API
    #endif
    #else
    #define PATIKA_API
    #endif
    #endif

    /* ============================================================================
     * Base Type Definitions
     * ========================================================================= */

    /** @brief Opaque agent identifier */
    typedef uint32_t AgentID;

    /** @brief Opaque barrack identifier */
    typedef uint16_t BarrackID;

    /** @brief Opaque simulation context handle */
    typedef struct PatikaContext *PatikaHandle;

    /* ============================================================================
     * Constants
     * ========================================================================= */

    PATIKA_API extern const uint32_t PATIKA_INVALID_AGENT_ID;
    PATIKA_API extern const uint16_t PATIKA_INVALID_BARRACK_ID;

    #ifdef __cplusplus
}
#endif

#endif /* PATIKA_TYPES_H */
