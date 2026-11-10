# protocol

The text lines exchanged with the Nucleo, as typed messages. Pure functions: no port and no
state.

| Module | Does |
|---|---|
| `messages.py` | one frozen dataclass per line the board sends: `Reading`, `Temperature`, `Position`, ... |
| `codec.py` | `parse_line()` turns a line into a message; `format_start()` and the keys (d, q, r, s) build what the PC sends; `format_capture()` turns a saved capture back into lines, for tests |

The line format is at the top of `codec.py`. Every message is specified in
[`../../../docs/diagrams/out/messages.svg`](../../../docs/diagrams/out/messages.svg), and one calibration is drawn in
[`../../../docs/diagrams/out/protocol.svg`](../../../docs/diagrams/out/protocol.svg). The firmware side is
`x-nucleo/Application/App/Src/app_protocol.c`, which lists the same lines in its header.

Values on the line are integers in micro-g and millidegrees. The messages hold g and degC.
