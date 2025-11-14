#include "patika_log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
typedef CRITICAL_SECTION patika_mutex_t;
#define PATIKA_MUTEX_INIT(m) InitializeCriticalSection(m)
#define PATIKA_MUTEX_LOCK(m) EnterCriticalSection(m)
#define PATIKA_MUTEX_UNLOCK(m) LeaveCriticalSection(m)
#define PATIKA_MUTEX_DESTROY(m) DeleteCriticalSection(m)
#else
#include <pthread.h>
typedef pthread_mutex_t patika_mutex_t;
#define PATIKA_MUTEX_INIT(m) pthread_mutex_init(m, NULL)
#define PATIKA_MUTEX_LOCK(m) pthread_mutex_lock(m)
#define PATIKA_MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
#define PATIKA_MUTEX_DESTROY(m) pthread_mutex_destroy(m)
#endif

#define PATIKA_LOG_BUFFER_SIZE 4096

/* ============================================================================
 *  INTERNAL STATE
 * ============================================================================
 */

typedef struct {
  FILE *file;                          // Log file handle (NULL if disabled)
  PatikaLogCallback callback;          // User callback (NULL if disabled)
  PatikaLogLevel min_level;            // Minimum log level
  int timestamps_enabled;              // 1 if timestamps enabled
  int initialized;                     // 1 if init() called
  patika_mutex_t mutex;                // Thread-safety mutex
  char buffer[PATIKA_LOG_BUFFER_SIZE]; // Thread-local buffer
} PatikaLogState;

static PatikaLogState g_log_state = {.file = NULL,
                                     .callback = NULL,
                                     .min_level = PATIKA_LOG_INFO,
                                     .timestamps_enabled = 1,
                                     .initialized = 0};

static void get_timestamp(char *buf, size_t size) {
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

const char *patika_log_level_str(PatikaLogLevel level) {
  switch (level) {
  case PATIKA_LOG_DEBUG:
    return "DEBUG";
  case PATIKA_LOG_INFO:
    return "INFO";
  case PATIKA_LOG_WARN:
    return "WARN";
  case PATIKA_LOG_ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

PATIKA_LOG_API int patika_log_init(void) {
  if (g_log_state.initialized) {
    return 0; // Already initialized
  }

  PATIKA_MUTEX_INIT(&g_log_state.mutex);
  g_log_state.initialized = 1;

  return 0;
}

PATIKA_LOG_API void patika_log_shutdown(void) {
  if (!g_log_state.initialized) {
    return;
  }

  PATIKA_MUTEX_LOCK(&g_log_state.mutex);

  if (g_log_state.file) {
    fflush(g_log_state.file);
    fclose(g_log_state.file);
    g_log_state.file = NULL;
  }

  g_log_state.callback = NULL;
  g_log_state.initialized = 0;

  PATIKA_MUTEX_UNLOCK(&g_log_state.mutex);
  PATIKA_MUTEX_DESTROY(&g_log_state.mutex);
}

PATIKA_LOG_API int patika_log_set_file(const char *filepath) {
  if (!g_log_state.initialized) {
    patika_log_init();
  }

  PATIKA_MUTEX_LOCK(&g_log_state.mutex);

  // Close existing file
  if (g_log_state.file) {
    fclose(g_log_state.file);
    g_log_state.file = NULL;
  }

  // Open new file
  if (filepath) {
    g_log_state.file = fopen(filepath, "a"); // Append mode
    if (!g_log_state.file) {
      PATIKA_MUTEX_UNLOCK(&g_log_state.mutex);
      return -1; // Failed to open
    }
  }

  PATIKA_MUTEX_UNLOCK(&g_log_state.mutex);
  return 0;
}

PATIKA_LOG_API void patika_log_set_callback(PatikaLogCallback callback) {
  if (!g_log_state.initialized) {
    patika_log_init();
  }

  PATIKA_MUTEX_LOCK(&g_log_state.mutex);
  g_log_state.callback = callback;
  PATIKA_MUTEX_UNLOCK(&g_log_state.mutex);
}

PATIKA_LOG_API void patika_log_set_level(PatikaLogLevel min_level) {
  if (!g_log_state.initialized) {
    patika_log_init();
  }

  PATIKA_MUTEX_LOCK(&g_log_state.mutex);
  g_log_state.min_level = min_level;
  PATIKA_MUTEX_UNLOCK(&g_log_state.mutex);
}

PATIKA_LOG_API void patika_log_set_timestamps(int enable) {
  if (!g_log_state.initialized) {
    patika_log_init();
  }

  PATIKA_MUTEX_LOCK(&g_log_state.mutex);
  g_log_state.timestamps_enabled = enable;
  PATIKA_MUTEX_UNLOCK(&g_log_state.mutex);
}

PATIKA_LOG_API void patika_log_write(PatikaLogLevel level, const char *file,
                                     int line, const char *fmt, ...) {
  if (!g_log_state.initialized) {
    patika_log_init();
  }

  // Early return if level too low
  if (level < g_log_state.min_level) {
    return;
  }

  PATIKA_MUTEX_LOCK(&g_log_state.mutex);

  // Format message
  char *buf = g_log_state.buffer;
  int offset = 0;

  // Add timestamp
  if (g_log_state.timestamps_enabled) {
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    offset += snprintf(buf + offset, PATIKA_LOG_BUFFER_SIZE - offset, "[%s] ",
                       timestamp);
  }

  // Add level
  offset += snprintf(buf + offset, PATIKA_LOG_BUFFER_SIZE - offset, "[%s] ",
                     patika_log_level_str(level));

  // Add file and line (only for DEBUG/ERROR)
  if (level == PATIKA_LOG_DEBUG || level == PATIKA_LOG_ERROR) {
    const char *filename = strrchr(file, '/');
    if (!filename)
      filename = strrchr(file, '\\');
    filename = filename ? filename + 1 : file;

    offset += snprintf(buf + offset, PATIKA_LOG_BUFFER_SIZE - offset,
                       "%s:%d: ", filename, line);
  }

  // Add user message
  va_list args;
  va_start(args, fmt);
  offset += vsnprintf(buf + offset, PATIKA_LOG_BUFFER_SIZE - offset, fmt, args);
  va_end(args);

  // Ensure newline
  if (offset > 0 && buf[offset - 1] != '\n') {
    if (offset < PATIKA_LOG_BUFFER_SIZE - 1) {
      buf[offset++] = '\n';
      buf[offset] = '\0';
    }
  }

  // Write to file
  if (g_log_state.file) {
    fputs(buf, g_log_state.file);
    fflush(g_log_state.file); // Immediate flush for debugging
  }

  // Write to stderr for WARN/ERROR
  if (level >= PATIKA_LOG_WARN) {
    fputs(buf, stderr);
  }

  // Call user callback
  if (g_log_state.callback) {
    g_log_state.callback(level, buf);
  }

  PATIKA_MUTEX_UNLOCK(&g_log_state.mutex);
}
