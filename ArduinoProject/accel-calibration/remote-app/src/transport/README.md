# transport

Moves text lines over the serial port, and knows nothing of what they mean. It is the only
code that imports pyserial.

| Module | Does |
|---|---|
| `serial_link.py` | `SerialLink`: open and close the port, `send()`, `send_sequence()` with pauses, and a reader thread that puts every received line on the `lines` queue as `(time, line)` |

Tests stand in for a board by putting lines on `SerialLink.lines` themselves.
