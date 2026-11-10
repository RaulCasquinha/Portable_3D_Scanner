/**
 * @file accel_calib.c
 * @brief Source file for the accel_calib.h header file.
 *
 * @author Raul Casquinha
 *
 * @date Nov/2026
 */

#include "accel_calib.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Axes of a vector
#define X                           0U
#define Y                           1U
#define Z                           2U
#define AXES                        ACCEL_CALIB_AXES
#define STD_VALUES                  ACCEL_CALIB_STD_VALUES
#define STD_MAGNITUDE               ACCEL_CALIB_STD_MAGNITUDE

// A reference is accepted this close to its axis, as checked by the PC app
#define REFERENCE_MAX_ANGLE_DEG     25.0F
#define DEG_TO_RAD                  (3.14159265F / 180.0F)
#define MS_PER_SECOND               1000U

/** A reference position: the axis that points up */
typedef struct {
    char name;                      /**< 'Z' or 'X', as sent to the PC */
    float axis[AXES];               /**< The unit reading expected (g) */
} Reference_t;

static const Reference_t REFERENCE_Z_UP = {'Z', {0.0F, 0.0F, 1.0F}};
static const Reference_t REFERENCE_X_UP = {'X', {1.0F, 0.0F, 0.0F}};

// The capture running, set by ACCEL_CALIB_Run()
static const ACCEL_CALIB_Io_t *ACCEL_CALIB_Io = NULL;
static const ACCEL_CALIB_Config_t *ACCEL_CALIB_Config = NULL;
static float ACCEL_CALIB_Positions_g[ACCEL_CALIB_MAX_POSITIONS][AXES];
static uint8_t ACCEL_CALIB_PositionCount = 0U;

static bool IsValidSetup(const ACCEL_CALIB_Io_t *io,
                         const ACCEL_CALIB_Config_t *config);
static ACCEL_CALIB_Status_t CapturePositions(void);
static ACCEL_CALIB_Status_t CaptureReferences(void);
static ACCEL_CALIB_Status_t MeasureReference(const Reference_t *reference);
static bool IsNewOrientation(const float mean_g[]);
static ACCEL_CALIB_Status_t ConfirmAndMeasure(float mean_g[], float std_g[],
                                              bool *is_still);
static ACCEL_CALIB_Status_t WaitForConfirmation(void);
static ACCEL_CALIB_Status_t CountDown(void);
static ACCEL_CALIB_Status_t MeasureMean(float mean_g[], float std_g[],
                                        bool *is_still);
static float StdDeviation(double sum, double sum_sq, double count);
static bool IsQuitRequested(void);
static void Idle(void);
static void Report(ACCEL_CALIB_EventType_t type);
static void ReportMeasurement(ACCEL_CALIB_EventType_t type, uint8_t index,
                              char reference, const float mean_g[],
                              const float std_g[]);
static void ReportEvent(ACCEL_CALIB_Event_t *event);
static float Norm(const float v[]);
static float CosAngle(const float a[], const float b[]);

ACCEL_CALIB_Status_t ACCEL_CALIB_Run(const ACCEL_CALIB_Io_t *io,
                                     const ACCEL_CALIB_Config_t *config) {
    ACCEL_CALIB_Event_t end = {0};

    if (!IsValidSetup(io, config)) {
        return ACCEL_CALIB_INVALID_PARAMETER;
    }
    ACCEL_CALIB_Io = io;
    ACCEL_CALIB_Config = config;

    Report(ACCEL_CALIB_EVENT_BEGIN);
    ACCEL_CALIB_Status_t status = CapturePositions();
    if (status == ACCEL_CALIB_OK) {
        status = CaptureReferences();
    }

    end.type = ACCEL_CALIB_EVENT_END;
    end.status = status;
    ReportEvent(&end);
    return status;
}

ACCEL_CALIB_Status_t ACCEL_CALIB_Apply(
    const ACCEL_CALIB_Calibration_t *calibration, const float raw_g[],
    float corrected_g[]) {
    float centered_g[AXES] = {0.0F};

    if ((calibration == NULL) || (raw_g == NULL) || (corrected_g == NULL)) {
        return ACCEL_CALIB_INVALID_PARAMETER;
    }
    // All of raw_g is read before corrected_g is written: they may be the same
    for (uint8_t i = 0U; i < AXES; i++) {
        centered_g[i] = raw_g[i] - calibration->bias_g[i];
    }
    for (uint8_t i = 0U; i < AXES; i++) {
        corrected_g[i] = (calibration->k[i][X] * centered_g[X])
                         + (calibration->k[i][Y] * centered_g[Y])
                         + (calibration->k[i][Z] * centered_g[Z]);
    }
    return ACCEL_CALIB_OK;
}

/* ------------------------------------------------------------------------ */
/* Procedure                                                                 */
/* ------------------------------------------------------------------------ */

/** Every service but idle is set, and every setting is in range */
static bool IsValidSetup(const ACCEL_CALIB_Io_t *io,
                         const ACCEL_CALIB_Config_t *config) {
    return (io != NULL) && (config != NULL)
           && (io->read_g != NULL) && (io->get_input != NULL)
           && (io->get_ms != NULL) && (io->report != NULL)
           && (config->positions > 0U)
           && (config->positions <= ACCEL_CALIB_MAX_POSITIONS)
           && (config->samples > 0U)
           && (config->input_timeout_s > 0U)
           && (config->references <= ACCEL_CALIB_REFERENCES_Z_X);
}

/** Measures positions until enough are kept: still, and each one new */
static ACCEL_CALIB_Status_t CapturePositions(void) {
    ACCEL_CALIB_Status_t status = ACCEL_CALIB_OK;
    float mean_g[AXES] = {0.0F};
    float std_g[STD_VALUES] = {0.0F};
    bool is_still = false;

    ACCEL_CALIB_PositionCount = 0U;
    while ((status == ACCEL_CALIB_OK)
           && (ACCEL_CALIB_PositionCount < ACCEL_CALIB_Config->positions)) {
        ACCEL_CALIB_Event_t prompt = {0};
        prompt.type = ACCEL_CALIB_EVENT_POSITION_PROMPT;
        prompt.index = (uint8_t)(ACCEL_CALIB_PositionCount + 1U);
        ReportEvent(&prompt);

        status = ConfirmAndMeasure(mean_g, std_g, &is_still);
        if (status != ACCEL_CALIB_OK) {
            break;
        }
        if (!is_still) {
            Report(ACCEL_CALIB_EVENT_MOVED);
        } else if (!IsNewOrientation(mean_g)) {
            Report(ACCEL_CALIB_EVENT_TOO_CLOSE);
        } else {
            for (uint8_t i = 0U; i < AXES; i++) {
                ACCEL_CALIB_Positions_g[ACCEL_CALIB_PositionCount][i] =
                    mean_g[i];
            }
            ACCEL_CALIB_PositionCount++;
            ReportMeasurement(ACCEL_CALIB_EVENT_POSITION,
                              ACCEL_CALIB_PositionCount, '\0', mean_g, std_g);
        }
    }
    return status;
}

/** +Z up, then +X up, as many as the settings ask for */
static ACCEL_CALIB_Status_t CaptureReferences(void) {
    ACCEL_CALIB_Status_t status = ACCEL_CALIB_OK;

    if (ACCEL_CALIB_Config->references != ACCEL_CALIB_REFERENCES_NONE) {
        status = MeasureReference(&REFERENCE_Z_UP);
    }
    if ((status == ACCEL_CALIB_OK)
        && (ACCEL_CALIB_Config->references == ACCEL_CALIB_REFERENCES_Z_X)) {
        status = MeasureReference(&REFERENCE_X_UP);
    }
    return status;
}

/** Measures one reference until it is still and close enough to its axis */
static ACCEL_CALIB_Status_t MeasureReference(const Reference_t *reference) {
    ACCEL_CALIB_Status_t status = ACCEL_CALIB_OK;
    float mean_g[AXES] = {0.0F};
    float std_g[STD_VALUES] = {0.0F};
    float min_cos = cosf(REFERENCE_MAX_ANGLE_DEG * DEG_TO_RAD);
    bool is_still = false;

    while (status == ACCEL_CALIB_OK) {
        ACCEL_CALIB_Event_t prompt = {0};
        prompt.type = ACCEL_CALIB_EVENT_REFERENCE_PROMPT;
        prompt.reference = reference->name;
        ReportEvent(&prompt);

        status = ConfirmAndMeasure(mean_g, std_g, &is_still);
        if (status != ACCEL_CALIB_OK) {
            break;
        }
        if (!is_still) {
            Report(ACCEL_CALIB_EVENT_MOVED);
        } else if (CosAngle(mean_g, reference->axis) < min_cos) {
            Report(ACCEL_CALIB_EVENT_WRONG_ORIENTATION);
        } else {
            ReportMeasurement(ACCEL_CALIB_EVENT_REFERENCE, 0U,
                              reference->name, mean_g, std_g);
            break;
        }
    }
    return status;
}

/** True if the reading is at least min_separation_deg from every position */
static bool IsNewOrientation(const float mean_g[]) {
    float max_cos = cosf(ACCEL_CALIB_Config->min_separation_deg * DEG_TO_RAD);

    for (uint8_t i = 0U; i < ACCEL_CALIB_PositionCount; i++) {
        if (CosAngle(mean_g, ACCEL_CALIB_Positions_g[i]) > max_cos) {
            return false;
        }
    }
    return true;
}

/* ------------------------------------------------------------------------ */
/* Measuring                                                                 */
/* ------------------------------------------------------------------------ */

/** Waits for the confirmation, counts down, then averages the samples */
static ACCEL_CALIB_Status_t ConfirmAndMeasure(float mean_g[], float std_g[],
                                              bool *is_still) {
    ACCEL_CALIB_Status_t status = WaitForConfirmation();

    if (status == ACCEL_CALIB_OK) {
        status = CountDown();
    }
    if (status == ACCEL_CALIB_OK) {
        Report(ACCEL_CALIB_EVENT_MEASURING);
        status = MeasureMean(mean_g, std_g, is_still);
    }
    return status;
}

/** Waits for CONFIRM or QUIT, at most input_timeout_s (rule FUN-12) */
static ACCEL_CALIB_Status_t WaitForConfirmation(void) {
    uint32_t start_ms = ACCEL_CALIB_Io->get_ms();
    uint32_t timeout_ms =
        (uint32_t)ACCEL_CALIB_Config->input_timeout_s * MS_PER_SECOND;

    while ((ACCEL_CALIB_Io->get_ms() - start_ms) < timeout_ms) {
        Idle();
        ACCEL_CALIB_Input_t input = ACCEL_CALIB_Io->get_input();
        if (input == ACCEL_CALIB_INPUT_QUIT) {
            return ACCEL_CALIB_ABORTED;
        }
        if (input == ACCEL_CALIB_INPUT_CONFIRM) {
            return ACCEL_CALIB_OK;
        }
    }
    return ACCEL_CALIB_TIMEOUT;
}

/** Lets the surface settle after the confirmation; QUIT still works */
static ACCEL_CALIB_Status_t CountDown(void) {
    for (uint8_t left = ACCEL_CALIB_Config->settle_s; left > 0U; left--) {
        uint32_t start_ms = ACCEL_CALIB_Io->get_ms();
        ACCEL_CALIB_Event_t countdown = {0};

        countdown.type = ACCEL_CALIB_EVENT_COUNTDOWN;
        countdown.seconds_left = left;
        ReportEvent(&countdown);
        while ((ACCEL_CALIB_Io->get_ms() - start_ms) < MS_PER_SECOND) {
            Idle();
            if (IsQuitRequested()) {
                return ACCEL_CALIB_ABORTED;
            }
        }
    }
    return ACCEL_CALIB_OK;
}

/**
 * Averages the samples, with the population std of each axis and of |g|. The
 * board is still when no axis std is above still_g.
 */
static ACCEL_CALIB_Status_t MeasureMean(float mean_g[], float std_g[],
                                        bool *is_still) {
    float sample_g[AXES] = {0.0F};
    // DEVIATION TYP-18: double sums keep the variance exact over 65535
    // samples; float would lose it in the subtraction of StdDeviation()
    double sum[AXES] = {0.0};
    double sum_sq[AXES] = {0.0};
    double sum_magnitude = 0.0;
    double sum_magnitude_sq = 0.0;
    double count = (double)ACCEL_CALIB_Config->samples;

    for (uint16_t n = 0U; n < ACCEL_CALIB_Config->samples; n++) {
        if (IsQuitRequested()) {
            return ACCEL_CALIB_ABORTED;
        }
        if (!ACCEL_CALIB_Io->read_g(sample_g)) {
            return ACCEL_CALIB_SENSOR_ERROR;
        }
        for (uint8_t i = 0U; i < AXES; i++) {
            sum[i] += (double)sample_g[i];
            sum_sq[i] += (double)sample_g[i] * (double)sample_g[i];
        }
        double magnitude = (double)Norm(sample_g);
        sum_magnitude += magnitude;
        sum_magnitude_sq += magnitude * magnitude;
    }

    *is_still = true;
    for (uint8_t i = 0U; i < AXES; i++) {
        mean_g[i] = (float)(sum[i] / count);
        std_g[i] = StdDeviation(sum[i], sum_sq[i], count);
        *is_still = *is_still && (std_g[i] <= ACCEL_CALIB_Config->still_g);
    }
    std_g[STD_MAGNITUDE] = StdDeviation(sum_magnitude, sum_magnitude_sq,
                                        count);
    return ACCEL_CALIB_OK;
}

/** Population standard deviation from the sum and the sum of squares */
static float StdDeviation(double sum, double sum_sq, double count) {
    double mean = sum / count;

    // Rounding can leave equal samples slightly below 0
    return (float)sqrt(fmax((sum_sq / count) - (mean * mean), 0.0));
}

/** True if the user asked to quit since the last call */
static bool IsQuitRequested(void) {
    return ACCEL_CALIB_Io->get_input() == ACCEL_CALIB_INPUT_QUIT;
}

/** The application's idle work, if it has any */
static void Idle(void) {
    if (ACCEL_CALIB_Io->idle != NULL) {
        ACCEL_CALIB_Io->idle();
    }
}

/* ------------------------------------------------------------------------ */
/* Events                                                                    */
/* ------------------------------------------------------------------------ */

/** Reports an event that carries no data */
static void Report(ACCEL_CALIB_EventType_t type) {
    ACCEL_CALIB_Event_t event = {0};

    event.type = type;
    ReportEvent(&event);
}

/** Reports a kept position or reference with its mean and std */
static void ReportMeasurement(ACCEL_CALIB_EventType_t type, uint8_t index,
                              char reference, const float mean_g[],
                              const float std_g[]) {
    ACCEL_CALIB_Event_t event = {0};

    event.type = type;
    event.index = index;
    event.reference = reference;
    for (uint8_t i = 0U; i < AXES; i++) {
        event.mean_g[i] = mean_g[i];
    }
    for (uint8_t i = 0U; i < STD_VALUES; i++) {
        event.std_g[i] = std_g[i];
    }
    ReportEvent(&event);
}

/** Adds the settings to an event and gives it to the application */
static void ReportEvent(ACCEL_CALIB_Event_t *event) {
    event->config = ACCEL_CALIB_Config;
    ACCEL_CALIB_Io->report(event);
}

/* ------------------------------------------------------------------------ */
/* Vectors                                                                   */
/* ------------------------------------------------------------------------ */

/** Length of a 3D vector */
static float Norm(const float v[]) {
    return sqrtf((v[X] * v[X]) + (v[Y] * v[Y]) + (v[Z] * v[Z]));
}

/** Cosine of the angle between two vectors; 1 if either is zero */
static float CosAngle(const float a[], const float b[]) {
    float dot = (a[X] * b[X]) + (a[Y] * b[Y]) + (a[Z] * b[Z]);
    float norms = Norm(a) * Norm(b);

    return (norms > 0.0F) ? (dot / norms) : 1.0F;
}
