/**
 * @file app_protocol.h
 * @brief Protocol: the text lines between the board and the PC app.
 *
 * Layer: protocol. Builds the lines the board sends and decodes the commands
 * it receives; the bytes go through app_link.h. The PC side is
 * Accel_Calib_App/src/protocol/codec.py, with the same lines and units.
 *
 * Lines from the board, ended by \r\n, values in micro-g (ug) and
 * millidegrees Celsius (mdegC):
 *
 *     $READ,<raw x y z ug>,<cal x y z ug>     50 Hz while streaming
 *     $TEMP,<mdegC>                           1 Hz, if the sensor has one
 *     $STREAM,<0|1>                           the stream started or stopped
 *     $BEGIN,<positions>,<references>,<min separation deg>
 *     $CALTEMP,BEGIN|END,<mdegC>              around a capture, if any
 *     $POS,<index>,<x y z ug>,<std x y z |g| ug>
 *     $REF,<Z|X>,<x y z ug>,<std x y z |g| ug>
 *     $END   or   $ABORT
 *     <text>                                  for the user (prompts, errors)
 *
 * From the PC: single keys (c calibrate with the settings of
 * app_calib_conf.h, d capture a position, q or Esc abort, r stream, s stop)
 * and one command line, ended by \r or \n:
 *
 *     $START,<positions>,<samples>,<references 0-2>
 *
 * Every message, with its fields, types and units, is drawn in
 * docs/diagrams/out/messages.svg of the project root.
 *
 * @author Raul Casquinha
 *
 * @date Nov/2026
 */

#ifndef APP_PROTOCOL_H
#define APP_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include "accel_calib.h"

/** Result of a protocol function */
typedef enum {
    APP_PROTOCOL_OK = 0,                /**< Done */
    APP_PROTOCOL_INVALID_PARAMETER = 1, /**< NULL or value out of range */
    APP_PROTOCOL_SEND_ERROR = 2,        /**< The line could not be sent whole */
    APP_PROTOCOL_BAD_COMMAND = 3        /**< A $ line was not understood */
} APP_PROTOCOL_Status_t;

/** What the PC asked for */
typedef enum {
    APP_PROTOCOL_COMMAND_NONE = 0,      /**< Nothing, or an unknown key */
    APP_PROTOCOL_COMMAND_CALIBRATE = 1, /**< c: capture, default settings */
    APP_PROTOCOL_COMMAND_START = 2,     /**< $START: capture, its settings */
    APP_PROTOCOL_COMMAND_CAPTURE = 3,   /**< d: measure this position */
    APP_PROTOCOL_COMMAND_ABORT = 4,     /**< q or Esc: abort the capture */
    APP_PROTOCOL_COMMAND_STREAM_ON = 5, /**< r: stream the readings */
    APP_PROTOCOL_COMMAND_STREAM_OFF = 6 /**< s: stop the stream */
} APP_PROTOCOL_CommandType_t;

/** A command from the PC */
typedef struct {
    APP_PROTOCOL_CommandType_t type;    /**< What is asked */
    uint8_t positions;                  /**< START: positions to capture */
    uint16_t samples;                   /**< START: samples per position */
    ACCEL_CALIB_References_t references;    /**< START: reference positions */
} APP_PROTOCOL_Command_t;

/** When a capture temperature ($CALTEMP) was read */
typedef enum {
    APP_PROTOCOL_MOMENT_BEGIN = 0,      /**< At the start of the capture */
    APP_PROTOCOL_MOMENT_END = 1         /**< At its end */
} APP_PROTOCOL_Moment_t;

/**
 * @brief Takes the next command from the PC, if there is one. Returns at once
 *        for a key; reads the rest of a $ line, waiting at most 1 s between
 *        two bytes.
 *
 * @param[out] command The command; type APP_PROTOCOL_COMMAND_NONE if none.
 *             Must not be NULL.
 *
 * @retval APP_PROTOCOL_OK                A command, or none.
 * @retval APP_PROTOCOL_BAD_COMMAND       A $ line was not understood; an error
 *                                        text was sent to the PC.
 * @retval APP_PROTOCOL_INVALID_PARAMETER command is NULL.
 */
APP_PROTOCOL_Status_t APP_PROTOCOL_ReceiveCommand(
    APP_PROTOCOL_Command_t *command);

/**
 * @brief Sends one reading: $READ, raw and calibrated.
 *
 * @param[in] raw_g Raw X Y Z (g). Must not be NULL.
 * @param[in] cal_g Calibrated X Y Z (g). Must not be NULL.
 *
 * @retval APP_PROTOCOL_OK                Sent.
 * @retval APP_PROTOCOL_SEND_ERROR        Not sent.
 * @retval APP_PROTOCOL_INVALID_PARAMETER A pointer is NULL.
 */
APP_PROTOCOL_Status_t APP_PROTOCOL_SendReading(const float raw_g[],
                                               const float cal_g[]);

/**
 * @brief Sends the sensor's temperature: $TEMP.
 *
 * @param[in] temperature_mdegc The temperature (mdegC).
 *
 * @retval APP_PROTOCOL_OK         Sent.
 * @retval APP_PROTOCOL_SEND_ERROR Not sent.
 */
APP_PROTOCOL_Status_t APP_PROTOCOL_SendTemperature(int32_t temperature_mdegc);

/**
 * @brief Sends a temperature at the start or end of a capture: $CALTEMP.
 *
 * @param[in] moment APP_PROTOCOL_MOMENT_BEGIN or APP_PROTOCOL_MOMENT_END.
 * @param[in] temperature_mdegc The temperature (mdegC).
 *
 * @retval APP_PROTOCOL_OK                Sent.
 * @retval APP_PROTOCOL_SEND_ERROR        Not sent.
 * @retval APP_PROTOCOL_INVALID_PARAMETER moment is not a moment.
 */
APP_PROTOCOL_Status_t APP_PROTOCOL_SendCaptureTemperature(
    APP_PROTOCOL_Moment_t moment, int32_t temperature_mdegc);

/**
 * @brief Says that the stream started or stopped: $STREAM.
 *
 * @param[in] is_streaming true if it started.
 *
 * @retval APP_PROTOCOL_OK         Sent.
 * @retval APP_PROTOCOL_SEND_ERROR Not sent.
 */
APP_PROTOCOL_Status_t APP_PROTOCOL_SendStreamState(bool is_streaming);

/**
 * @brief Sends what a capture event means: the text for the user and, for
 *        BEGIN, POSITION, REFERENCE and END, the $ line for the PC.
 *
 * @param[in] event The event from accel_calib.h. Must not be NULL.
 *
 * @retval APP_PROTOCOL_OK                Sent.
 * @retval APP_PROTOCOL_SEND_ERROR        A line was not sent.
 * @retval APP_PROTOCOL_INVALID_PARAMETER event or its config is NULL.
 */
APP_PROTOCOL_Status_t APP_PROTOCOL_SendCaptureEvent(
    const ACCEL_CALIB_Event_t *event);

/**
 * @brief Sends the list of keys and commands, for a terminal.
 *
 * @retval APP_PROTOCOL_OK         Sent.
 * @retval APP_PROTOCOL_SEND_ERROR Not sent.
 */
APP_PROTOCOL_Status_t APP_PROTOCOL_SendHelp(void);

/**
 * @brief Sends a line of text for the user, e.g. an error. \r\n is added.
 *
 * @param[in] text The text, without line end. Must not be NULL.
 *
 * @retval APP_PROTOCOL_OK                Sent.
 * @retval APP_PROTOCOL_SEND_ERROR        Not sent.
 * @retval APP_PROTOCOL_INVALID_PARAMETER text is NULL.
 */
APP_PROTOCOL_Status_t APP_PROTOCOL_SendText(const char text[]);

#endif // APP_PROTOCOL_H
