"""
Services: the app's use cases as functions without state. No GUI, no port.

    calibrate  a capture -> a Calibration (ellipsoid fit and alignment)
    merge      simple records -> a temperature compensated record
    export     a calibration -> the C defines for the firmware
    report     the text of tables and records shown in the GUI
"""
