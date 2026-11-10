"""
Processing: a reusable library for accelerometer calibration.

Layer: processing, the lowest layer. Every module here depends only on numpy and
on other modules of this package, never on files, ports, the protocol or the GUI,
so it can be copied into another project or tested on its own.

    sfem            CalibrationModel, the SFEM correction K (U - b) with K = M C S
    ellipsoid_fit   b, S and alpha from static readings, closed form
    alignment       the misalignment angles from known reference positions
    regression      LinearTrend: least-squares lines of many values against one x
    compensation    TemperatureCompensation: SFEM parameters as lines in temperature
    geometry        unit vectors, roll and pitch, the nearest axis
    statistics      rms of the magnitude error
    placement       the checks the board makes before accepting a position
"""
