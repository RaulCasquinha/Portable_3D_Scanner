/**
 * @file app_log.c
 * @brief Source file for the app_log.h header file.
 *
 * This is the only module that calls the <stdio.h> output functions
 * (rule FUN-09). printf output reaches COM1 through the BSP __io_putchar().
 *
 * @author Raul Casquinha
 *
 * @date Oct/2026
 */

#include "app_log.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

void APP_LOG_Write(const char *format, ...) {
    va_list args;

    if (format == NULL) {
        return;
    }

    va_start(args, format);
    // IGNORED: there is nowhere to report a failure of the debug output
    (void)vprintf(format, args);
    va_end(args);

    // IGNORED: same as above
    (void)fflush(stdout);
}
