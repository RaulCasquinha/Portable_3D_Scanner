/**
 * @file app.h
 * @brief Accelerometer calibration application (ADXL345 or ADXL355): the
 *        board captures, the PC app (Accel_Calib_App) fits.
 *
 * Layer: application (library APP). The main loop takes the PC's commands,
 * streams the readings at 50 Hz and the temperature at 1 Hz, and runs the
 * guided capture of the AccelCalib library. The messages are listed in
 * app_protocol.h; the settings and the flashed calibration are in
 * app_calib_conf.h.
 *
 * Wiring (NUCLEO-WL55JC1, same pins for both sensors):
 *      CS -> PA5, MISO/SDO -> PA11, MOSI/SDA -> PA12, SCLK/SCL -> PB3,
 *      VDD -> 3V3, GND -> GND
 *
 * @author Raul Casquinha
 *
 * @date Nov/2026
 */

#ifndef APP_H
#define APP_H

/**
 * @brief Application initialisation, called from main.c USER CODE 2, before
 *        the BSP and COM1 initialisation. Starts the sensor.
 */
void APP_Init(void);

/**
 * @brief Application main loop, called from main.c USER CODE WHILE. Never
 *        returns; calls Error_Handler() if the sensor did not start.
 */
_Noreturn void APP_Run(void);

#endif // APP_H
