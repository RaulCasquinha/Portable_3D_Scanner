"""
Hover help of every tagged item: tag -> text.

Layer: view. Every new widget gets a tag and an entry here.
"""

__all__ = ["TIPS"]

_MEASUREMENTS_TIP = (
    "Every measured position and reference: the mean of its samples for x, y, z, and its "
    "magnitude |g|, in mg; and the standard deviation of the samples of each axis and of "
    "the raw |g|, in micro-g. The std is the noise of single samples, not of the mean.")
_PLOT_TIP = "{}: raw, calibrated and the nearest axis reference, against time."
_ERROR_PLOT_TIP = "{} error: reading minus the nearest axis reference, in mg."

TIPS = {
    # Controls
    "controls.status": "What the board asks for now, or what just happened.",
    "procedure.progress": "Positions and references measured, out of the total. Before a capture, the total the settings ask for.",
    "connection.port": "The Nucleo's ST-LINK virtual COM port (115200 8N1).",
    "connection.refresh_ports": "List the serial ports again.",
    "connection.connect": "Open the port. The live readings start at once.",
    "controls.start": "Start a capture with the Calibration settings.",
    "controls.capture": "Measure this position (d key). After a short countdown the board "
                        "averages the samples and keeps them if it stayed still and the "
                        "position is new.",
    "controls.abort": "Stop the capture (q key). The positions captured so far are discarded.",
    "connection.stream": "Make the board stream its readings to the plots (r key). It starts "
                       "streaming on connect.",
    "connection.stop": "Stop the board's stream (s key). The plots freeze; a capture still "
                     "streams.",
    "display.pause": "Freeze the Live stream, Tilt and Errors windows to study them: zoom with "
                      "the mouse wheel, drag to pan, double-click to fit. Unlike Stop, the "
                      "board's data keeps coming in; Resume display shows it again (the last "
                      "10 s).",
    "records.sensor_id": "Serial number or name of this sensor. Its records go in "
                          "calibrations/<id>/.",
    "records.save": "Save this calibration as a record of the sensor: the result, the "
                     "settings, every position with its standard deviation, and the "
                     "temperatures.",
    "procedure.temperature": "Temperature the sensor reports, and at the start and end of the "
                            "calibration. --- if the sensor doesn't send it.",
    "procedure.measurements_header": _MEASUREMENTS_TIP,
    "procedure.measurements": _MEASUREMENTS_TIP,
    "board_log.text": "The last text messages from the board.",

    # Calibration settings
    "settings.positions": "Orientations to capture, up to 32. The normal calibration is 6, for "
                          "bias and scale; at least 3 per estimated term (9 with alpha).",
    "settings.samples": "Readings averaged per orientation.",
    "settings.references": "Known orientations measured last, for the alignment, an optional "
                           "extra. z up corrects the tilt, z up + x up also the heading. none: "
                           "only the magnitude is calibrated.",
    "settings.term.bias": "Estimate the bias b: offset of each axis. Part of the normal "
                          "calibration.",
    "settings.term.scale": "Estimate the scale S: gain of each axis. Part of the normal "
                           "calibration.",
    "settings.term.nonortho": "Estimate the non-orthogonality alpha: axes not at 90 deg. An "
                              "optional extra: it needs 9 positions or more, spread out (including "
                              "tilted ones).",
    "settings.refit": "Fit the captured positions again with the ticked terms.",
    "display.cal_source": "Where the calibrated values come from. PC fit: this app applies "
                           "its own fit to the raw readings, so it shows the result before "
                           "flashing. board (flash): the values the Nucleo computes with the "
                           "constants in its firmware, so it shows what is really on the "
                           "board. They equal the raw readings until real constants are "
                           "flashed.",
    "display.apply.bias": "Apply the bias b to the calibrated values. Off: b = 0. Only what "
                           "is shown changes, with cal source PC fit; the fit and the "
                           "defines stay.",
    "display.apply.scale": "Apply the scale S to the calibrated values. Off: S = 1. Only what "
                            "is shown changes, with cal source PC fit; the fit and the "
                            "defines stay.",
    "display.apply.nonortho": "Apply the non-orthogonality alpha to the calibrated values. "
                               "Off: alpha = 0. Only what is shown changes, with cal source "
                               "PC fit; the fit and the defines stay.",
    "display.apply.misalignment": "Apply the misalignment M (phi, theta, psi) to the "
                                   "calibrated values. Off: angles 0. Only what is shown "
                                   "changes, with cal source PC fit; the fit and the defines "
                                   "stay.",
    "display.smoothing": "Smooth the numbers and the board's arrow: show the mean of the "
                         "last readings instead of the newest one. Off: the newest reading, "
                         "noisier but immediate. The plots are never smoothed.",
    "display.smoothing_s": "How much time the smoothing averages, in seconds: longer is "
                           "steadier but slower to follow the board. Only with smoothing on.",
    "display.show_reference": "Draw the nearest of +-X +-Y +-Z, taken from the calibrated "
                               "reading, on the plots and the tables. Lay the board on a face "
                               "on a level surface. There is no reference beyond 15 deg from "
                               "an axis.",
    "settings.preset_list": "Saved settings. The preset named Default is loaded when the app "
                            "starts: save over it to change what the app starts with.",
    "settings.preset_load": "Set the settings of the selected preset.",
    "settings.preset_delete": "Delete the selected preset.",
    "settings.preset_name": "Name of a new preset.",
    "settings.preset_save": "Save the settings of this window and the time scale under this "
                            "name.",

    # Live stream
    "stream.readout": "Mean of the smoothing time (Display settings), or the newest reading with smoothing off. ref: the "
                      "nearest axis, raw: the sensor, cal: with this calibration.",
    "stream.time_scale": "How much time the graphs show. Ctrl+click to type a value.",
    "stream.plot.x": _PLOT_TIP.format("X axis") + " In g.",
    "stream.plot.y": _PLOT_TIP.format("Y axis") + " In g.",
    "stream.plot.z": _PLOT_TIP.format("Z axis") + " In g.",
    "stream.plot.g": "Magnitude |g|, raw and calibrated. The reference is always 1 g, in any "
                     "position.",

    # Tilt
    "tilt.mode": "roll / pitch: the roll and pitch plots. axis angles: the angle of each "
                 "axis to gravity. The readout shows the columns of the mode. Presets "
                 "save it.",
    "tilt.readout": "Mean of the smoothing time (Display settings), or the newest reading with smoothing off, deg. Roll and pitch: 0 with +Z up. x-g, "
                    "y-g, z-g: the angle between each axis and the gravity vector, "
                    "atan2(sqrt(g^2 - m_k^2), m_k): 0 when the axis points up, 90 when it is "
                    "level, 180 when it points down. ref: the nearest axis, raw: the sensor, "
                    "cal: with this calibration.",
    "tilt.plot.roll": _PLOT_TIP.format("Roll, rotation about x") + " In deg.",
    "tilt.plot.pitch": _PLOT_TIP.format("Pitch, rotation about y") + " In deg.",
    "tilt.plot.axes": "The angle between each axis (x, y, z) and the gravity vector, in deg: "
                      "0 up, 90 level, 180 down. Calibrated in red, green, blue; raw in their "
                      "complements, cyan, magenta, yellow.",

    # Errors
    "errors.readout": "Mean of the smoothing time (Display settings), or the newest reading with smoothing off: the reading minus the reference, in mg. "
                      "Positive: it reads too high. nan: the board is more than 15 deg from "
                      "any axis.",
    "errors.plot.x": _ERROR_PLOT_TIP.format("X axis"),
    "errors.plot.y": _ERROR_PLOT_TIP.format("Y axis"),
    "errors.plot.z": _ERROR_PLOT_TIP.format("Z axis"),
    "errors.plot.g": "|g| error: the magnitude minus 1 g, in mg. It needs no axis, so it "
                     "works in any position.",

    # Calibration
    "sphere.view": "Captured positions as gravity directions. The arrow is where the "
                          "board points now: green if d will accept it, red if not. Drag to "
                          "rotate. Depth: the sphere is translucent, so what is behind it "
                          "looks dimmed (never the arrow: its colour stays clear). Points in "
                          "front are filled dots, points behind are "
                          "rings. The arrow's head is filled when it points towards you and "
                          "an outline when it points away, and its part behind is dashed; "
                          "seen end-on it is a dot (towards you) or a ring (away).",
    "sphere.shading": "How depth is shown on the sphere. Presets save it.",
    "sphere.tint": "How much the sphere darkens what is behind it. 0: none, front and back "
                   "look the same.",
    "sphere.highlight": "The light spot that makes the sphere look like a ball. 0: flat.",
    "sphere.marks_behind": "Draw what is behind the sphere's centre as rings, outlined arrow "
                           "heads and dashed lines. Off: everything is solid, and only the "
                           "tint shows depth.",
    "sphere.placement": "Would d accept the board as it is now? Stillness is checked by "
                             "the board.",

    # Parameters
    "parameters.table": "The calibration in use. b bias, S scale, a non-orthogonality, phi "
                        "theta psi alignment.",
    "parameters.summary": "corrected = K (raw - b), K = M C S.",
    "parameters.defines": "Paste into app_calib_conf.h and flash to use this calibration.",

    # Records
    "records.sensor": "Show the records of one sensor, or all of them.",
    "records.refresh": "Read calibrations/ again, for records added or removed outside this "
                       "app.",
    "records.load": "Use the selected record as the calibration: it fills Parameters and the "
                    "sphere, and the PC fit source applies it to the live readings. It also "
                    "works before connecting. The board keeps what is in its flash.",
    "records.list": "Saved calibrations, newest first: n positions, k samples each, "
                    "temperature, and the rms |g| error before (raw) and after (cal) the fit. "
                    "Click one to see it below; tick it to show it in Temperature analysis. "
                    "Only Load changes the calibration in use, and nothing here changes the "
                    "files.",
    "records.text": "The selected record. Each position is the raw mean, its |g| error before "
                    "and after this record's calibration, and the std of the raw |g| "
                    "(select, Ctrl+C).",

    # Temperature analysis
    "temperature.parameter": "The parameter on the vertical axis.",
    "temperature.plot": "Points: the chosen parameter of each ticked record against the "
                        "temperature it was calibrated at (mean of the start and end). Line: "
                        "the calibration in use, flat unless it is temperature compensated "
                        "(then its regression line, and orange points: the calibrations it "
                        "was fitted to). Yellow: where it is now, at the sensor's "
                        "temperature.",
    "temperature.info": "How many of the ticked records are on the plot.",
    "temperature.create": "Make a temperature compensated calibration of the ticked plain "
                          "calibrations of one sensor, at 2 or more temperatures: the "
                          "parameters ticked under 'slope for' are the linear regression "
                          "(least-squares line) of their values against temperature, the "
                          "others their mean. It is saved as a new record; Load it to use it, "
                          "and then it follows the sensor's temperature.",
    "temperature.slope_label": "Which parameters of the next compensated calibration change "
                               "with temperature. Bias and scale do; alpha and the alignment "
                               "are mechanical, and their spread between captures is mostly "
                               "where the board was placed, so a slope there fits noise.",
    "temperature.slope.bias": "Give the bias b a slope in temperature, else its mean.",
    "temperature.slope.scale": "Give the scale S a slope in temperature, else its mean.",
    "temperature.slope.nonortho": "Give the non-orthogonality alpha a slope in temperature, "
                                  "else its mean.",
    "temperature.slope.alignment": "Give the alignment phi theta psi a slope in temperature, "
                                   "else its mean.",
    "temperature.hold": "Outside the temperatures of its calibrations, keep the values at the "
                        "nearest end instead of following the lines, which are unknown there.",
    # Menu bar
    "windows.menu": "Show or hide each window. At start, the first workspace decides.",
    "windows.show_all": "Show every window again.",
    "windows.hide_all": "Hide every window. Show them again from this menu.",
    "workspaces.menu": "Window arrangements saved by name. Each saved workspace is an item on "
                       "this bar: click it to move the windows there. Docked windows become "
                       "floating windows in the same places.",
    "workspaces.save_as": "Save where every window is, its size and which windows are hidden, "
                          "under a name. A workspace of the same name is replaced.",
    "workspaces.delete": "Delete a saved workspace.",
    "workspaces.default": "Show every window and put them in their default places.",
    "workspaces.name": "Name of the workspace.",
    "workspaces.save": "Save the workspace under this name.",
    "workspaces.cancel": "Close without saving.",
}
