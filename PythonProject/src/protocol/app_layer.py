"""Application-layer packet decoder - mirrors the firmware Data/AppLayer protocol.

Packet wire format::

    [ 0xAA | 0xBB ] [ TYPE : 1 B ] [ LEN : 1 B ] [ PAYLOAD : LEN B ] [ CRC8 : 1 B ]

Packet types:
    POINT (0x01) - x, y, z  as little-endian float32  (12 B payload)
    POSE  (0x02) - px py pz pitch roll yaw             (24 B payload)
    LOG   (0x03) - UTF-8 text                          (variable)
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum
from typing import Callable


class PacketType(IntEnum):
    POINT = 0x01
    POSE  = 0x02
    LOG   = 0x03


@dataclass
class PointPacket:
    x: float; y: float; z: float       # surface point (world coords, mm)
    ox: float; oy: float; oz: float    # sensor origin when measured (mm)


@dataclass
class PosePacket:
    px: float; py: float; pz: float
    pitch: float; roll: float; yaw: float


@dataclass
class LogPacket:
    message: str


class _State(IntEnum):
    SYNC1 = 0; SYNC2 = 1; TYPE = 2; LENGTH = 3; PAYLOAD = 4; CRC = 5


def _crc8(data: bytes) -> int:
    crc = 0
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) if (crc & 0x80) else (crc << 1)
    return crc & 0xFF


class AppLayer:
    """Feeds raw bytes and calls typed callbacks for each decoded packet.

    Assign callables to :attr:`on_point`, :attr:`on_pose`, :attr:`on_log`
    before feeding data::

        p = AppLayer()
        p.on_point = lambda pkt: print(pkt.x, pkt.y, pkt.z)
        p.feed(raw_bytes)
    """

    def __init__(self) -> None:
        self.on_point: Callable[[PointPacket], None] | None = None
        self.on_pose:  Callable[[PosePacket],  None] | None = None
        self.on_log:   Callable[[LogPacket],   None] | None = None
        self.on_debug: Callable[[str], None]         | None = None

        self._state    = _State.SYNC1
        self._pkt_type = 0
        self._pkt_len  = 0
        self._payload  = bytearray()
        self._sync_found = False   # fires on_debug once per sync sequence

    def feed(self, data: bytes) -> None:
        """Push raw bytes from the transport layer into the decoder."""
        for b in data:
            self._process(b)

    # ------------------------------------------------------------------

    def _process(self, byte: int) -> None:
        s = self._state
        if s == _State.SYNC1:
            if byte == 0xAA:
                self._state = _State.SYNC2
        elif s == _State.SYNC2:
            if byte == 0xBB:
                self._state = _State.TYPE
                if not self._sync_found:
                    self._sync_found = True
                    self._dbg("Sync sequence found - decoder locked")
            else:
                self._state = _State.SYNC1
        elif s == _State.TYPE:
            self._pkt_type = byte
            self._state = _State.LENGTH
        elif s == _State.LENGTH:
            self._pkt_len = byte
            self._payload.clear()
            self._state = _State.PAYLOAD if byte > 0 else _State.CRC
        elif s == _State.PAYLOAD:
            self._payload.append(byte)
            if len(self._payload) == self._pkt_len:
                self._state = _State.CRC
        elif s == _State.CRC:
            expected = _crc8(bytes([self._pkt_type, self._pkt_len]) + self._payload)
            if byte == expected:
                self._dispatch()
            else:
                self._dbg(
                    f"CRC mismatch - type=0x{self._pkt_type:02X} "
                    f"len={self._pkt_len} "
                    f"expected=0x{expected:02X} got=0x{byte:02X}"
                )
            self._state = _State.SYNC1

    def _dispatch(self) -> None:
        p = bytes(self._payload)
        try:
            if self._pkt_type == PacketType.POINT and self.on_point:
                x, y, z, ox, oy, oz = struct.unpack_from('<ffffff', p)
                self.on_point(PointPacket(x, y, z, ox, oy, oz))
            elif self._pkt_type == PacketType.POSE and self.on_pose:
                self.on_pose(PosePacket(*struct.unpack_from('<ffffff', p)))
            elif self._pkt_type == PacketType.LOG and self.on_log:
                self.on_log(LogPacket(p.decode('utf-8', errors='replace')))
            elif self._pkt_type not in PacketType.__members__.values():
                self._dbg(f"Unknown packet type 0x{self._pkt_type:02X} (len={self._pkt_len})")
        except struct.error as exc:
            self._dbg(f"Struct decode error - type=0x{self._pkt_type:02X} len={self._pkt_len}: {exc}")

    def _dbg(self, msg: str) -> None:
        if self.on_debug:
            self.on_debug(msg)
