# Data / TransportLayer

Lowest-level data layer: wraps the Arduino `Serial` interface for raw byte transmission.

## Responsibility

Owns baud-rate initialisation and provides a single `send()` primitive that the application layer calls to push bytes onto the wire. Keeping this as its own class decouples the rest of the stack from the concrete serial backend, making it straightforward to swap in USB-CDC, BLE, or Wi-Fi later.

## Dependencies

- `Serial` (Arduino built-in)

## Usage

```cpp
Data_TransportLayer transport;
transport.begin(115200);
transport.send(buf, len);
```
