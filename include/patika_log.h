/*
 * Patika Logging System - Public API
 * Thread-safe, file-based logging with callbacks
 * Author: Oguzhan Akyuz
 * Created: 2025-10-28
 */

#ifndef PATIKA_LOG_H
#define PATIKA_LOG_H

#include <stdint.h>

#ifdef _WIN32
#ifdef PATIKA_EXPORTS
#define PATIKA_LOG_API __declspec(dllexport)
#else
#define PATIKA_LOG_API __declspec(dllimport)
#endif
#else
#if defined(__GNUC__) || defined(__clang__)
#ifdef PATIKA_EXPORTS
#define PATIKA_LOG_API __attribute__((visibility("default")))
#else
#define PATIKA_LOG_API
#endif
#else
#define PATIKA_LOG_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log severity levels.
 */
typedef enum {
  PATIKA_LOG_DEBUG = 0, // Verbose debugging information
  PATIKA_LOG_INFO = 1,  // General informational messages
  PATIKA_LOG_WARN = 2,  // Warning conditions
  PATIKA_LOG_ERROR = 3  // Error conditions
} PatikaLogLevel;

/**
 * @brief User-provided log callback.
 * @param level Severity level of the message.
 * @param message Formatted log message (null-terminated).
 */
typedef void (*PatikaLogCallback)(PatikaLogLevel level, const char *message);

/**
 * @brief Initialize the logging system.
 * @note Must be called before any logging operations.
 * @return 0 on success, -1 on failure.
 */
PATIKA_LOG_API int patika_log_init(void);

/**
 * @brief Shutdown the logging system and flush buffers.
 */
PATIKA_LOG_API void patika_log_shutdown(void);

/**
 * @brief Set the log output file.
 * @param filepath Path to log file (NULL to disable file logging).
 * @return 0 on success, -1 on failure (e.g., cannot open file).
 */
PATIKA_LOG_API int patika_log_set_file(const char *filepath);

/**
 * @brief Set a custom log callback.
 * @param callback User-provided callback (NULL to disable).
 */
PATIKA_LOG_API void patika_log_set_callback(PatikaLogCallback callback);

/**
 * @brief Set the minimum log level.
 * @param min_level Messages below this level will be ignored.
 */
PATIKA_LOG_API void patika_log_set_level(PatikaLogLevel min_level);

/**
 * @brief Enable/disable timestamps in log messages.
 * @param enable 1 to enable, 0 to disable.
 */
PATIKA_LOG_API void patika_log_set_timestamps(int enable);

/**
 * @brief Internal logging function (do not call directly).
 */
PATIKA_LOG_API void patika_log_write(PatikaLogLevel level, const char *file,
                                     int line, const char *fmt, ...);

/**
 * @brief Get string representation of log level.
 * @param level Log level enum.
 * @return String like "DEBUG", "INFO", etc.
 */
PATIKA_LOG_API const char *patika_log_level_str(PatikaLogLevel level);

/* ============================================================================
 *  LOGGING MACROS (use these in your code)
 * ============================================================================
 */

#ifdef PATIKA_DEBUG
#define PATIKA_LOG_DEBUG(fmt, ...)                                             \
  patika_log_write(PATIKA_LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define PATIKA_LOG_INFO(fmt, ...)                                              \
  patika_log_write(PATIKA_LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define PATIKA_LOG_DEBUG(fmt, ...) ((void)0)
#define PATIKA_LOG_INFO(fmt, ...) ((void)0)
#endif

#define PATIKA_LOG_WARN(fmt, ...)                                              \
  patika_log_write(PATIKA_LOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define PATIKA_LOG_ERROR(fmt, ...)                                             \
  patika_log_write(PATIKA_LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* PATIKA_LOG_H */
