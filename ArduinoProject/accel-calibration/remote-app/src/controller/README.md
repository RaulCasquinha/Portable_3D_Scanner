# controller

The state of the app that the windows show and change. No GUI code, so it runs without
DearPyGui.

| Module | Main item | Does |
|---|---|---|
| `board.py` | `Board` | the Nucleo: connect, stream, start and abort a capture, fit it, the calibration in use, the status line. `advance()` handles the received lines once per frame |
| `record_browser.py` | `RecordBrowser` | the listed, selected and ticked records; load, save, and merge into a compensated one |
| `live_view.py` | `live_view()` | the arrays the live windows plot this frame: readings, means, nearest axis |

```python
board = Board()                  # or Board(fake_link) in a test
board.connect("COM5")
board.advance()                  # every frame
board.start_capture(12, 200, 1, ("bias", "scale", "nonortho"))
```
