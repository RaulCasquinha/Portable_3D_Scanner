/**
 * @file app_log.h
 * @brief Project logging macro LOG() (rule FUN-09).
 *
 * All debug output goes through LOG(), never printf directly. In Debug builds
 * (DEBUG defined) the text is formatted and sent to COM1 (ST-LINK virtual COM
 * port, 115200 8N1). In Release builds the output is removed.
 *
 * The arguments of LOG() must have no side effects, because they are not
 * evaluated in Release builds.
 *
 * @author Raul Casquinha
 *
 * @date Oct/2026
 */

#ifndef APP_LOG_H
#define APP_LOG_H

#include <stdbool.h>

#if defined(DEBUG)

/** Formats and sends the text (printf format) */
#define LOG(...)    APP_LOG_Write(__VA_ARGS__)

#else

/*
 * Removed from Release builds: the arguments are not evaluated, but they are
 * still referenced so that variables used only for logging do not cause
 * unused variable warnings. The optimiser removes the dead call.
 */
#define LOG(...)                            \
    do {                                    \
        if (false) {                        \
            APP_LOG_Write(__VA_ARGS__);     \
        }                                   \
    } while (0)

#endif

/**
 * @brief Formats and sends a debug message to COM1. Use it through LOG().
 *
 * The output is flushed at once, so text without a line ending (e.g. a
 * countdown) is shown immediately.
 *
 * @param[in] format printf format string. Must not be NULL.
 * @param[in] ... Format arguments
 *
 * @note Must not be called from interrupt context (rule EMB-03).
 */
void APP_LOG_Write(const char *format, ...);

#endif // APP_LOG_H
