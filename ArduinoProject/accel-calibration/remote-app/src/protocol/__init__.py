"""
Protocol: the text lines exchanged with the Nucleo, as typed messages.

Layer: protocol. Pure: no port, no state. The firmware side is
Accel_Calib_stm32/Application/App/Src/app_protocol.c.

    messages    one dataclass per line the board sends
    codec       parse_line() turns a line into a message; format_*() and the
                keys build what the PC sends
"""
