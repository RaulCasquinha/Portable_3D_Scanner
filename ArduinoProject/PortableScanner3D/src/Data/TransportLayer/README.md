# Data / TransportLayer

Lowest-level data layer: wraps the Arduino `Serial` interface (mapped to USB-CDC on ESP32-S3) for raw byte transmission.

## Responsibility

Owns baud-rate initialisation and provides a single `send()` primitive that the application layer uses to push bytes onto the wire. Decoupling this from `Data_AppLayer` makes it straightforward to swap in BLE, Wi-Fi, or UART later without touching the packet framing logic.

## Dependencies

- `Serial` (Arduino built-in — USB-CDC when **Tools → USB CDC On Boot → Enabled**)

## Usage

```cpp
Data_TransportLayer transport;
transport.begin(115200);
transport.send(buf, len);
```
