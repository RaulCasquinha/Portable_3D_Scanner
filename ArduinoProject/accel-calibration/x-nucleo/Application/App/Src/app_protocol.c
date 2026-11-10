/**
 * @file app_protocol.c
 * @brief Source file for the app_protocol.h header file.
 *
 * @author Raul Casquinha
 *
 * @date Nov/2026
 */

#include "app_protocol.h"

#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "accel_calib.h"
#include "app_link.h"

// Keys from the PC
#define KEY_CALIBRATE           'c'
#define KEY_CAPTURE             'd'
#define KEY_ABORT               'q'
#define KEY_ESCAPE              '\x1B'
#define KEY_STREAM_ON           'r'
#define KEY_STREAM_OFF          's'
#define KEY_COMMAND             '$'

// The $START command line
#define COMMAND_SIZE            32U     // Longest line, terminator included
#define COMMAND_BYTE_TIMEOUT_MS 1000U   // Between two bytes: time to type
#define COMMAND_START           "START,"
#define START_FIELDS            3U
#define FIELD_POSITIONS         0U
#define FIELD_SAMPLES           1U
#define FIELD_REFERENCES        2U
#define DECIMAL_BASE            10U

// The lines sent
#define LINE_SIZE               160U    // Longest line, terminator included
#define AXES                    ACCEL_CALIB_AXES
#define X                       0U
#define Y                       1U
#define Z                       2U
#define G_TO_UG                 1000000.0F

static APP_PROTOCOL_Status_t ReceiveCommandLine(
    APP_PROTOCOL_Command_t *command);
static bool ReadLine(char line[], size_t size);
static bool DecodeStart(const char line[], APP_PROTOCOL_Command_t *command);
static bool ParseNumber(const char **cursor, char separator, uint32_t max,
                        uint32_t *value);
static APP_PROTOCOL_CommandType_t DecodeKey(char key);
static APP_PROTOCOL_Status_t SendMeasurement(const char label[],
                                             const ACCEL_CALIB_Event_t *event);
static APP_PROTOCOL_Status_t SendBegin(const ACCEL_CALIB_Config_t *config);
static APP_PROTOCOL_Status_t SendEnd(ACCEL_CALIB_Status_t status);
static const char *ReferenceInstruction(char reference);
static APP_PROTOCOL_Status_t SendFormatted(const char *format, ...);
static int32_t ToMicroG(float value_g);

/* ------------------------------------------------------------------------ */
/* Commands from the PC                                                      */
/* ------------------------------------------------------------------------ */

APP_PROTOCOL_Status_t APP_PROTOCOL_ReceiveCommand(
    APP_PROTOCOL_Command_t *command) {
    static const APP_PROTOCOL_Command_t NO_COMMAND = {0};
    uint8_t byte = 0U;

    if (command == NULL) {
        return APP_PROTOCOL_INVALID_PARAMETER;
    }
    *command = NO_COMMAND;      // Only a START sets the other fields
    if (APP_LINK_ReceiveByte(&byte) != APP_LINK_OK) {
        return APP_PROTOCOL_OK;         // Nothing received
    }
    if ((char)byte == KEY_COMMAND) {
        return ReceiveCommandLine(command);
    }
    command->type = DecodeKey((char)byte);
    return APP_PROTOCOL_OK;
}

/** The rest of a $ line; a line not understood is reported to the PC */
static APP_PROTOCOL_Status_t ReceiveCommandLine(
    APP_PROTOCOL_Command_t *command) {
    char line[COMMAND_SIZE] = {'\0'};

    if (ReadLine(line, sizeof(line)) && DecodeStart(line, command)) {
        return APP_PROTOCOL_OK;
    }
    command->type = APP_PROTOCOL_COMMAND_NONE;    // DecodeStart() may fail late
    // IGNORED: the error text is a courtesy; the PC retries $START anyway
    (void)SendFormatted("ERROR: received \"$%s\", expected "
                        "$START,<positions>,<samples>,<references 0-2>\r\n",
                        line);
    return APP_PROTOCOL_BAD_COMMAND;
}

/** Reads bytes until \r or \n. false on a timeout or a line too long */
static bool ReadLine(char line[], size_t size) {
    uint8_t byte = 0U;

    for (size_t length = 0U; length < (size - 1U); length++) {
        if (APP_LINK_WaitForByte(&byte, COMMAND_BYTE_TIMEOUT_MS)
            != APP_LINK_OK) {
            line[length] = '\0';
            return false;
        }
        if ((byte == (uint8_t)'\n') || (byte == (uint8_t)'\r')) {
            line[length] = '\0';
            return true;
        }
        line[length] = (char)byte;
    }
    line[size - 1U] = '\0';
    return false;
}

/** START,<positions>,<samples>,<references>, each within its limit */
static bool DecodeStart(const char line[], APP_PROTOCOL_Command_t *command) {
    static const uint32_t MAX_VALUE[START_FIELDS] = {
        ACCEL_CALIB_MAX_POSITIONS, UINT16_MAX,
        (uint32_t)ACCEL_CALIB_REFERENCES_Z_X
    };
    uint32_t field[START_FIELDS] = {0U};
    const char *cursor = &line[strlen(COMMAND_START)];

    if (strncmp(line, COMMAND_START, strlen(COMMAND_START)) != 0) {
        return false;
    }
    for (uint8_t i = 0U; i < START_FIELDS; i++) {
        char separator = (i < (START_FIELDS - 1U)) ? ',' : '\0';
        if (!ParseNumber(&cursor, separator, MAX_VALUE[i], &field[i])) {
            return false;
        }
    }
    command->type = APP_PROTOCOL_COMMAND_START;
    command->positions = (uint8_t)field[FIELD_POSITIONS];
    command->samples = (uint16_t)field[FIELD_SAMPLES];
    command->references = (ACCEL_CALIB_References_t)field[FIELD_REFERENCES];
    return true;
}

/**
 * Reads a decimal number at *cursor, ended by separator, at most max. Moves
 * *cursor past the separator. false if there are no digits, another
 * character, or the value is above max.
 */
static bool ParseNumber(const char **cursor, char separator, uint32_t max,
                        uint32_t *value) {
    const char *next = *cursor;
    uint32_t number = 0U;

    if ((*next < '0') || (*next > '9')) {
        return false;
    }
    while ((*next >= '0') && (*next <= '9')) {
        uint32_t digit = (uint32_t)(*next - '0');
        // number * 10 + digit <= max, checked before computing it so it can't
        // overflow (rule TYP-15); digit > max first, or max - digit wraps
        if ((digit > max) || (number > ((max - digit) / DECIMAL_BASE))) {
            return false;
        }
        number = (number * DECIMAL_BASE) + digit;
        next++;
    }
    if (*next != separator) {
        return false;
    }
    *cursor = next + 1;
    *value = number;
    return true;
}

/** The command of a single key */
static APP_PROTOCOL_CommandType_t DecodeKey(char key) {
    switch (key) {
    case KEY_CALIBRATE:
        return APP_PROTOCOL_COMMAND_CALIBRATE;
    case KEY_CAPTURE:
        return APP_PROTOCOL_COMMAND_CAPTURE;
    case KEY_ABORT:
    case KEY_ESCAPE:
        return APP_PROTOCOL_COMMAND_ABORT;
    case KEY_STREAM_ON:
        return APP_PROTOCOL_COMMAND_STREAM_ON;
    case KEY_STREAM_OFF:
        return APP_PROTOCOL_COMMAND_STREAM_OFF;
    default:
        return APP_PROTOCOL_COMMAND_NONE;   // Other keys mean nothing
    }
}

/* ------------------------------------------------------------------------ */
/* Lines to the PC                                                           */
/* ------------------------------------------------------------------------ */

APP_PROTOCOL_Status_t APP_PROTOCOL_SendReading(const float raw_g[],
                                               const float cal_g[]) {
    if ((raw_g == NULL) || (cal_g == NULL)) {
        return APP_PROTOCOL_INVALID_PARAMETER;
    }
    return SendFormatted("$READ,%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32
                         ",%" PRId32 ",%" PRId32 "\r\n",
                         ToMicroG(raw_g[X]), ToMicroG(raw_g[Y]),
                         ToMicroG(raw_g[Z]), ToMicroG(cal_g[X]),
                         ToMicroG(cal_g[Y]), ToMicroG(cal_g[Z]));
}

APP_PROTOCOL_Status_t APP_PROTOCOL_SendTemperature(int32_t temperature_mdegc) {
    return SendFormatted("$TEMP,%" PRId32 "\r\n", temperature_mdegc);
}

APP_PROTOCOL_Status_t APP_PROTOCOL_SendCaptureTemperature(
    APP_PROTOCOL_Moment_t moment, int32_t temperature_mdegc) {
    if ((moment != APP_PROTOCOL_MOMENT_BEGIN)
        && (moment != APP_PROTOCOL_MOMENT_END)) {
        return APP_PROTOCOL_INVALID_PARAMETER;
    }
    const char *name = (moment == APP_PROTOCOL_MOMENT_BEGIN) ? "BEGIN" : "END";

    return SendFormatted("$CALTEMP,%s,%" PRId32 "\r\n", name,
                         temperature_mdegc);
}

APP_PROTOCOL_Status_t APP_PROTOCOL_SendStreamState(bool is_streaming) {
    return SendFormatted("$STREAM,%c\r\n", is_streaming ? '1' : '0');
}

APP_PROTOCOL_Status_t APP_PROTOCOL_SendCaptureEvent(
    const ACCEL_CALIB_Event_t *event) {
    if ((event == NULL) || (event->config == NULL)) {
        return APP_PROTOCOL_INVALID_PARAMETER;
    }
    const ACCEL_CALIB_Config_t *config = event->config;

    switch (event->type) {
    case ACCEL_CALIB_EVENT_BEGIN:
        return SendBegin(config);
    case ACCEL_CALIB_EVENT_POSITION_PROMPT:
        return SendFormatted("\r\n[%" PRIu32 "/%" PRIu32 "] New orientation, "
                             "press d when in place\r\n",
                             (uint32_t)event->index,
                             (uint32_t)config->positions);
    case ACCEL_CALIB_EVENT_REFERENCE_PROMPT:
        return SendFormatted("\r\nReference %s\r\nPress d when in place\r\n",
                             ReferenceInstruction(event->reference));
    case ACCEL_CALIB_EVENT_COUNTDOWN:
        return SendFormatted("  Measuring in %" PRIu32 "\r\n",
                             (uint32_t)event->seconds_left);
    case ACCEL_CALIB_EVENT_MEASURING:
        return SendFormatted("  Measuring, keep still\r\n");
    case ACCEL_CALIB_EVENT_MOVED:
        return SendFormatted("  The board moved, try again\r\n");
    case ACCEL_CALIB_EVENT_TOO_CLOSE:
        return SendFormatted("  Too close to a captured orientation, rotate "
                             "further\r\n");
    case ACCEL_CALIB_EVENT_WRONG_ORIENTATION:
        return SendFormatted("  Wrong orientation, try again\r\n");
    case ACCEL_CALIB_EVENT_POSITION:
        return SendMeasurement("  Position captured\r\n$POS,", event);
    case ACCEL_CALIB_EVENT_REFERENCE:
        return SendMeasurement("  Reference captured\r\n$REF,", event);
    case ACCEL_CALIB_EVENT_END:
        return SendEnd(event->status);
    default:
        return APP_PROTOCOL_INVALID_PARAMETER;
    }
}

APP_PROTOCOL_Status_t APP_PROTOCOL_SendHelp(void) {
    return SendFormatted(
        "\r\nAccelerometer calibration\r\n"
        "Keys: c = calibrate, r = stream readings, s = stop the stream\r\n"
        "Command: $START,<positions>,<samples>,<references 0-2>\r\n\r\n");
}

APP_PROTOCOL_Status_t APP_PROTOCOL_SendText(const char text[]) {
    if (text == NULL) {
        return APP_PROTOCOL_INVALID_PARAMETER;
    }
    return SendFormatted("%s\r\n", text);
}

/**
 * A kept position or reference: the text, then the line
 * <label><index or axis>,<mean x y z ug>,<std x y z |g| ug>.
 */
static APP_PROTOCOL_Status_t SendMeasurement(
    const char label[], const ACCEL_CALIB_Event_t *event) {
    char id[4] = {'\0'};    // A position number up to 255, or the axis

    if (event->type == ACCEL_CALIB_EVENT_POSITION) {
        // IGNORED: 3 digits always fit
        (void)snprintf(id, sizeof(id), "%" PRIu32, (uint32_t)event->index);
    } else {
        id[0] = event->reference;
    }
    return SendFormatted(
        "%s%s,%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32
        ",%" PRId32 ",%" PRId32 "\r\n",
        label, id, ToMicroG(event->mean_g[X]), ToMicroG(event->mean_g[Y]),
        ToMicroG(event->mean_g[Z]), ToMicroG(event->std_g[X]),
        ToMicroG(event->std_g[Y]), ToMicroG(event->std_g[Z]),
        ToMicroG(event->std_g[ACCEL_CALIB_STD_MAGNITUDE]));
}

/**
 * The start text (the three lines telling the user what to do), then $BEGIN,
 * each line sent on its own: together they are longer than LINE_SIZE, and
 * SendFormatted() sends nothing when a line is cut, so the PC never saw
 * $BEGIN. $BEGIN is sent even if a text line fails, and last, so the capture
 * temperatures still follow it directly.
 */
static APP_PROTOCOL_Status_t SendBegin(const ACCEL_CALIB_Config_t *config) {
    APP_PROTOCOL_Status_t text = SendFormatted(
        "\r\n=== Accelerometer calibration ===\r\n");

    if (text == APP_PROTOCOL_OK) {
        text = SendFormatted("Place the board in %" PRIu32 " different "
                             "orientations, spread over all directions.\r\n",
                             (uint32_t)config->positions);
    }
    if (text == APP_PROTOCOL_OK) {
        text = SendFormatted("Press d to measure each one and keep the board "
                             "still. q aborts.\r\n");
    }
    APP_PROTOCOL_Status_t begin = SendFormatted(
        "$BEGIN,%" PRIu32 ",%" PRIu32 ",%" PRId32 "\r\n",
        (uint32_t)config->positions, (uint32_t)config->references,
        (int32_t)lroundf(config->min_separation_deg));

    return (begin != APP_PROTOCOL_OK) ? begin : text;
}

/** $END or $ABORT, then the result in words */
static APP_PROTOCOL_Status_t SendEnd(ACCEL_CALIB_Status_t status) {
    const char *result = NULL;

    switch (status) {
    case ACCEL_CALIB_OK:
        result = "Capture done, the PC computes the calibration";
        break;
    case ACCEL_CALIB_ABORTED:
        result = "Capture aborted";
        break;
    case ACCEL_CALIB_SENSOR_ERROR:
        result = "Capture stopped: sensor error";
        break;
    case ACCEL_CALIB_TIMEOUT:
        result = "Capture stopped: no confirmation in time";
        break;
    default:
        result = "Capture stopped";
        break;
    }
    return SendFormatted("%s\r\n%s\r\n\r\n",
                         (status == ACCEL_CALIB_OK) ? "$END" : "$ABORT",
                         result);
}

/** How to place the board for a reference */
static const char *ReferenceInstruction(char reference) {
    return (reference == 'X')
           ? "+X up: board on its edge, square to the level surface"
           : "+Z up: board flat on a level surface";
}

/** Formats a line (printf format) and sends it whole, or not at all */
static APP_PROTOCOL_Status_t SendFormatted(const char *format, ...) {
    char line[LINE_SIZE] = {'\0'};
    va_list args;

    va_start(args, format);
    // DEVIATION TYP-01: vsnprintf() returns a plain int
    int length = vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    if ((length <= 0) || ((size_t)length >= sizeof(line))) {
        return APP_PROTOCOL_SEND_ERROR;     // Error, or the line was cut
    }
    if (APP_LINK_Send(line, (size_t)length) != APP_LINK_OK) {
        return APP_PROTOCOL_SEND_ERROR;
    }
    return APP_PROTOCOL_OK;
}

/** A value in g as integer micro-g, as the lines carry it */
static int32_t ToMicroG(float value_g) {
    return (int32_t)lroundf(value_g * G_TO_UG);
}
