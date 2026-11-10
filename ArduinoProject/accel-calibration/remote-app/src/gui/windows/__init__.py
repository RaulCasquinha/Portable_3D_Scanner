"""
One module per window. Each window class builds its items in __init__ and
redraws them in update(); it reads the controller's state and calls its methods.

    connection            Connection: the serial port, stream / stop
    controls              Controls: status, capture
    calibration_settings  Calibration settings: capture settings, terms, presets
    procedure             Calibration procedure: temperatures, captured positions
    display_settings      Display settings: pause, cal source, applied terms, smoothing
    stream                Live stream: x y z |g| readout and plots
    tilt                  Tilt: roll, pitch and each axis to gravity
    errors                Errors: reading minus reference, readout and plots
    status_sphere         Status sphere: the captured positions on a sphere
    parameters            Parameters: the fitted parameters and the C defines
    records               Records: Save record, saved calibrations, ticks, the selected one
    temperature           Temperature analysis: parameters against temperature
    board_log             Board messages: the board's last messages
"""
