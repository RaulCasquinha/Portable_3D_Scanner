/**
 * @file accel_calib.h
 * @brief Guided capture of an accelerometer calibration, and the correction
 *        that applies its result. The fit runs on the PC (Accel_Calib_App).
 *
 * Library: no hardware, no text and no protocol. The application gives the
 * platform services (::ACCEL_CALIB_Io_t) and receives what happens as events
 * (::ACCEL_CALIB_Event_t), which it turns into messages for the PC.
 *
 * The capture: the user places the board in a new orientation and confirms;
 * after a countdown the samples are averaged, and the position is kept if the
 * board stayed still and the orientation is new. Then one or two reference
 * positions (+Z up, +X up) are measured, for the alignment. The usual
 * calibration is the 6-point calibration: 6 independent positions, without
 * references.
 *
 * The result, computed on the PC, is applied with ACCEL_CALIB_Apply():
 *
 *     corrected = K * (raw - bias)
 *
 * @author Raul Casquinha
 *
 * @date Nov/2026
 */

#ifndef ACCEL_CALIB_H
#define ACCEL_CALIB_H

#include <stdbool.h>
#include <stdint.h>

/** Axes of a reading: X, Y, Z */
#define ACCEL_CALIB_AXES            3U
/** Standard deviations of a measurement: X, Y, Z, then the magnitude |g| */
#define ACCEL_CALIB_STD_VALUES      4U
/** Index of the magnitude in a standard deviation array */
#define ACCEL_CALIB_STD_MAGNITUDE   3U
/** Most positions a capture can hold */
#define ACCEL_CALIB_MAX_POSITIONS   32U

/** Result of a capture */
typedef enum {
    ACCEL_CALIB_OK = 0,             /**< Everything measured */
    ACCEL_CALIB_ABORTED = 1,        /**< The user quit */
    ACCEL_CALIB_SENSOR_ERROR = 2,   /**< read_g() failed */
    ACCEL_CALIB_TIMEOUT = 3,        /**< No confirmation in input_timeout_s */
    ACCEL_CALIB_INVALID_PARAMETER = 4   /**< NULL or setting out of range */
} ACCEL_CALIB_Status_t;

/** Reference positions measured after the positions. The value is how many */
typedef enum {
    ACCEL_CALIB_REFERENCES_NONE = 0,    /**< No alignment */
    ACCEL_CALIB_REFERENCES_Z = 1,       /**< +Z up: corrects the tilt */
    ACCEL_CALIB_REFERENCES_Z_X = 2      /**< +Z up, +X up: full alignment */
} ACCEL_CALIB_References_t;

/** What the user asked for, as given by get_input() */
typedef enum {
    ACCEL_CALIB_INPUT_NONE = 0,     /**< Nothing */
    ACCEL_CALIB_INPUT_CONFIRM = 1,  /**< The board is in place: measure it */
    ACCEL_CALIB_INPUT_QUIT = 2      /**< Abort the capture */
} ACCEL_CALIB_Input_t;

/** What happened during the capture. The fields each type fills are listed */
typedef enum {
    ACCEL_CALIB_EVENT_BEGIN = 0,            /**< Started */
    ACCEL_CALIB_EVENT_POSITION_PROMPT = 1,  /**< Place position index */
    ACCEL_CALIB_EVENT_REFERENCE_PROMPT = 2, /**< Place reference */
    ACCEL_CALIB_EVENT_COUNTDOWN = 3,        /**< seconds_left to measuring */
    ACCEL_CALIB_EVENT_MEASURING = 4,        /**< Averaging the samples */
    ACCEL_CALIB_EVENT_MOVED = 5,            /**< Rejected: not still */
    ACCEL_CALIB_EVENT_TOO_CLOSE = 6,        /**< Rejected: not new */
    ACCEL_CALIB_EVENT_WRONG_ORIENTATION = 7, /**< Rejected: not the axis */
    ACCEL_CALIB_EVENT_POSITION = 8,         /**< Kept: index, mean, std */
    ACCEL_CALIB_EVENT_REFERENCE = 9,        /**< Kept: reference, mean, std */
    ACCEL_CALIB_EVENT_END = 10              /**< Ended with status */
} ACCEL_CALIB_EventType_t;

/** Settings of a capture */
typedef struct {
    uint8_t positions;          /**< 1 to ACCEL_CALIB_MAX_POSITIONS */
    uint16_t samples;           /**< Samples averaged per position, >= 1 */
    uint8_t settle_s;           /**< Countdown before measuring (s) */
    uint16_t input_timeout_s;   /**< Longest wait for a confirmation (s) */
    float still_g;              /**< Largest axis std when still (g) */
    float min_separation_deg;   /**< Least angle between positions (deg) */
    ACCEL_CALIB_References_t references;    /**< Reference positions */
} ACCEL_CALIB_Config_t;

/** One event, given to report(). Only the fields of its type are set */
typedef struct {
    ACCEL_CALIB_EventType_t type;           /**< What happened */
    const ACCEL_CALIB_Config_t *config;     /**< The capture's settings */
    uint8_t index;              /**< Position number, from 1 */
    char reference;             /**< 'Z' or 'X' */
    uint8_t seconds_left;       /**< Of the countdown */
    ACCEL_CALIB_Status_t status;            /**< Result, for END */
    float mean_g[ACCEL_CALIB_AXES];         /**< Mean X Y Z (g) */
    float std_g[ACCEL_CALIB_STD_VALUES];    /**< Std X Y Z and |g| (g) */
} ACCEL_CALIB_Event_t;

/** Platform services, filled by the application (rule SRC-10) */
typedef struct {
    /** Waits for and reads one new sample, X Y Z in g. false on error */
    bool (*read_g)(float sample_g[]);
    /** Non-blocking: what the user asked for since the last call */
    ACCEL_CALIB_Input_t (*get_input)(void);
    /** Milliseconds since start-up, for the countdown and the timeout */
    uint32_t (*get_ms)(void);
    /** Receives every event, e.g. to send it to the PC */
    void (*report)(const ACCEL_CALIB_Event_t *event);
    /** Called repeatedly while waiting, e.g. to stream. May be NULL */
    void (*idle)(void);
} ACCEL_CALIB_Io_t;

/** A calibration result: corrected = K * (raw - bias) */
typedef struct {
    float bias_g[ACCEL_CALIB_AXES];                 /**< Bias of each axis, g */
    float k[ACCEL_CALIB_AXES][ACCEL_CALIB_AXES];    /**< K = M C S */
} ACCEL_CALIB_Calibration_t;

/**
 * @brief Runs the guided capture. Blocking until it ends.
 *
 * Reports ACCEL_CALIB_EVENT_BEGIN first and ACCEL_CALIB_EVENT_END last, with
 * the status that is returned.
 *
 * @param[in] io Platform services; every function but idle must be set.
 * @param[in] config Settings, in the ranges of ::ACCEL_CALIB_Config_t.
 *
 * @retval ACCEL_CALIB_OK                Every position and reference kept.
 * @retval ACCEL_CALIB_ABORTED           The user quit.
 * @retval ACCEL_CALIB_SENSOR_ERROR      A sample could not be read.
 * @retval ACCEL_CALIB_TIMEOUT           No confirmation in input_timeout_s.
 * @retval ACCEL_CALIB_INVALID_PARAMETER NULL pointer or setting out of range;
 *                                       nothing is reported.
 */
ACCEL_CALIB_Status_t ACCEL_CALIB_Run(const ACCEL_CALIB_Io_t *io,
                                     const ACCEL_CALIB_Config_t *config);

/**
 * @brief Corrects a reading: corrected = K * (raw - bias).
 *
 * @param[in] calibration The calibration. Must not be NULL.
 * @param[in] raw_g Raw X Y Z (g). Must not be NULL.
 * @param[out] corrected_g Corrected X Y Z (g). Must not be NULL; may be
 *             raw_g.
 *
 * @retval ACCEL_CALIB_OK                Corrected.
 * @retval ACCEL_CALIB_INVALID_PARAMETER A pointer is NULL.
 */
ACCEL_CALIB_Status_t ACCEL_CALIB_Apply(
    const ACCEL_CALIB_Calibration_t *calibration, const float raw_g[],
    float corrected_g[]);

#endif // ACCEL_CALIB_H
