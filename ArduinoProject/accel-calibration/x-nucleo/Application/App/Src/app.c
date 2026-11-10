/**
 * @file app.c
 * @brief Source file for the app.h header file.
 *
 * The main loop takes the PC's commands and streams the readings. A capture
 * is run by the AccelCalib library, which this file gives the platform
 * services: the sensor (app_accel.h), the PC's keys and the events, which go
 * to the PC through app_protocol.h.
 *
 * @author Raul Casquinha
 *
 * @date Nov/2026
 */

#include "app.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "accel_calib.h"
#include "app_accel.h"
#include "app_calib_conf.h"
#include "app_link.h"
#include "app_log.h"
#include "app_protocol.h"

#define STREAM_PERIOD_MS        20U     // $READ at 50 Hz
#define TEMPERATURE_PERIOD_MS   1000U   // $TEMP at 1 Hz

// The capture of the c key; $START from the PC replaces positions, samples
// and references
static const ACCEL_CALIB_Config_t APP_DefaultConfig = {
    .positions = APP_CALIB_POSITIONS,
    .samples = APP_CALIB_SAMPLES,
    .settle_s = APP_CALIB_SETTLE_S,
    .input_timeout_s = APP_CALIB_INPUT_TIMEOUT_S,
    .still_g = APP_CALIB_STILL_G,
    .min_separation_deg = APP_CALIB_MIN_SEPARATION_DEG,
    .references = APP_CALIB_REFERENCES
};

// The calibration flashed with this firmware, for the cal values of $READ
static const ACCEL_CALIB_Calibration_t APP_Calibration = {
    .bias_g = ACCEL_CAL_BIAS_G,
    .k = ACCEL_CAL_K
};

static bool APP_IsSensorReady = false;
static bool APP_IsStreaming = false;
static uint32_t APP_LastReadingMs = 0U;
static uint32_t APP_LastTemperatureMs = 0U;

static void HandleCommand(void);
static void Calibrate(const ACCEL_CALIB_Config_t *config);
static void SetStreaming(bool is_streaming);
static void StreamWhileIdle(void);
static void SendReadingIfDue(const float raw_g[]);
static void SendTemperatureIfDue(void);
static void SendCaptureTemperature(APP_PROTOCOL_Moment_t moment);
static void CheckSent(APP_PROTOCOL_Status_t status);
static bool ReadAndStream(float sample_g[]);
static ACCEL_CALIB_Input_t GetCaptureInput(void);
static void OnCaptureEvent(const ACCEL_CALIB_Event_t *event);

// The platform services of the capture (rule SRC-10)
static const ACCEL_CALIB_Io_t APP_CalibIo = {
    .read_g = ReadAndStream,
    .get_input = GetCaptureInput,
    .get_ms = HAL_GetTick,
    .report = OnCaptureEvent,
    .idle = StreamWhileIdle
};

void APP_Init(void) {
    APP_IsSensorReady = APP_ACCEL_Init();
}

_Noreturn void APP_Run(void) {
    // Here and not in APP_Init(): COM1 is only initialised between the two
    if (APP_LINK_Init() != APP_LINK_OK) {
        LOG("COM1 FIFO not enabled: a $START may lose bytes\r\n");
    }
    if (!APP_IsSensorReady) {
        CheckSent(APP_PROTOCOL_SendText(
            "ERROR: accelerometer not found, check the wiring"));
        Error_Handler();
    }
    CheckSent(APP_PROTOCOL_SendHelp());

    while (true) {
        HandleCommand();
        StreamWhileIdle();
    }
}

/* ------------------------------------------------------------------------ */
/* Main loop                                                                 */
/* ------------------------------------------------------------------------ */

/** Does what the PC asked for. Keys of the capture are ignored outside one */
static void HandleCommand(void) {
    APP_PROTOCOL_Command_t command = {0};

    if (APP_PROTOCOL_ReceiveCommand(&command) != APP_PROTOCOL_OK) {
        return;     // A bad $ line: the protocol already told the PC
    }

    switch (command.type) {
    case APP_PROTOCOL_COMMAND_CALIBRATE:
        Calibrate(&APP_DefaultConfig);
        break;
    case APP_PROTOCOL_COMMAND_START: {
        ACCEL_CALIB_Config_t config = APP_DefaultConfig;
        config.positions = command.positions;
        config.samples = command.samples;
        config.references = command.references;
        // The PC app plots the capture as it happens; s still stops it
        SetStreaming(true);
        Calibrate(&config);
        break;
    }
    case APP_PROTOCOL_COMMAND_STREAM_ON:
        SetStreaming(true);
        break;
    case APP_PROTOCOL_COMMAND_STREAM_OFF:
        SetStreaming(false);
        break;
    default:
        break;      // NONE, or d / q with no capture running
    }
}

/** Runs one capture; the library reports its progress through the events */
static void Calibrate(const ACCEL_CALIB_Config_t *config) {
    if (ACCEL_CALIB_Run(&APP_CalibIo, config)
        == ACCEL_CALIB_INVALID_PARAMETER) {
        CheckSent(APP_PROTOCOL_SendText(
            "Capture not started: positions 1 to 32, samples above 0"));
    }
}

/** Starts or stops the stream, and confirms it to the PC */
static void SetStreaming(bool is_streaming) {
    APP_IsStreaming = is_streaming;
    CheckSent(APP_PROTOCOL_SendStreamState(is_streaming));
}

/** Streams a reading and the temperature when they are due */
static void StreamWhileIdle(void) {
    float raw_g[ACCEL_CALIB_AXES] = {0.0F};

    SendTemperatureIfDue();
    if (!APP_IsStreaming
        || ((HAL_GetTick() - APP_LastReadingMs) < STREAM_PERIOD_MS)) {
        return;
    }
    if (APP_ACCEL_ReadG(raw_g)) {
        SendReadingIfDue(raw_g);
    } else {
        CheckSent(APP_PROTOCOL_SendText("ERROR: reading the accelerometer"));
    }
}

/** $READ with the raw reading and the flashed calibration applied to it */
static void SendReadingIfDue(const float raw_g[]) {
    float cal_g[ACCEL_CALIB_AXES] = {0.0F};

    if (!APP_IsStreaming
        || ((HAL_GetTick() - APP_LastReadingMs) < STREAM_PERIOD_MS)) {
        return;
    }
    APP_LastReadingMs = HAL_GetTick();
    // IGNORED: both pointers are valid, the only failure
    (void)ACCEL_CALIB_Apply(&APP_Calibration, raw_g, cal_g);
    CheckSent(APP_PROTOCOL_SendReading(raw_g, cal_g));
}

/** $TEMP once a second, stream or not. Nothing if the sensor has none */
static void SendTemperatureIfDue(void) {
    int32_t temperature_mdegc = 0;

    if ((HAL_GetTick() - APP_LastTemperatureMs) < TEMPERATURE_PERIOD_MS) {
        return;
    }
    APP_LastTemperatureMs = HAL_GetTick();
    if (APP_ACCEL_ReadTemperature(&temperature_mdegc)) {
        CheckSent(APP_PROTOCOL_SendTemperature(temperature_mdegc));
    }
}

/** $CALTEMP at the start or end of a capture, if the sensor has one */
static void SendCaptureTemperature(APP_PROTOCOL_Moment_t moment) {
    int32_t temperature_mdegc = 0;

    if (APP_ACCEL_ReadTemperature(&temperature_mdegc)) {
        CheckSent(APP_PROTOCOL_SendCaptureTemperature(moment,
                                                      temperature_mdegc));
    }
}

/**
 * Handles the status of a line sent to the PC (rule FUN-11). A lost line
 * can't be sent again usefully: the PC sees a gap in the stream, or retries
 * $START. It is logged in Debug builds.
 */
static void CheckSent(APP_PROTOCOL_Status_t status) {
    if (status != APP_PROTOCOL_OK) {
        LOG("line not sent (%" PRIu32 ")\r\n", (uint32_t)status);
    }
}

/* ------------------------------------------------------------------------ */
/* Platform services of the capture                                          */
/* ------------------------------------------------------------------------ */

/** read_g: reads a sample and keeps streaming while the capture runs */
static bool ReadAndStream(float sample_g[]) {
    if (!APP_ACCEL_ReadG(sample_g)) {
        return false;
    }
    SendTemperatureIfDue();
    SendReadingIfDue(sample_g);
    return true;
}

/** get_input: d confirms, q or Esc quits; r and s still control the stream */
static ACCEL_CALIB_Input_t GetCaptureInput(void) {
    APP_PROTOCOL_Command_t command = {0};

    if (APP_PROTOCOL_ReceiveCommand(&command) != APP_PROTOCOL_OK) {
        return ACCEL_CALIB_INPUT_NONE;
    }
    switch (command.type) {
    case APP_PROTOCOL_COMMAND_CAPTURE:
        return ACCEL_CALIB_INPUT_CONFIRM;
    case APP_PROTOCOL_COMMAND_ABORT:
        return ACCEL_CALIB_INPUT_QUIT;
    case APP_PROTOCOL_COMMAND_STREAM_ON:
        SetStreaming(true);
        return ACCEL_CALIB_INPUT_NONE;
    case APP_PROTOCOL_COMMAND_STREAM_OFF:
        SetStreaming(false);
        return ACCEL_CALIB_INPUT_NONE;
    default:
        return ACCEL_CALIB_INPUT_NONE;  // c or $START during a capture
    }
}

/**
 * report: sends the event to the PC. The capture temperatures go right after
 * $BEGIN and right before $END or $ABORT: the PC clears a capture's
 * temperatures on $BEGIN and fits it on $END.
 */
static void OnCaptureEvent(const ACCEL_CALIB_Event_t *event) {
    if (event->type == ACCEL_CALIB_EVENT_END) {
        SendCaptureTemperature(APP_PROTOCOL_MOMENT_END);
    }
    CheckSent(APP_PROTOCOL_SendCaptureEvent(event));
    if (event->type == ACCEL_CALIB_EVENT_BEGIN) {
        SendCaptureTemperature(APP_PROTOCOL_MOMENT_BEGIN);
    }
}
