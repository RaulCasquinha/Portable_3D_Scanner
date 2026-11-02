# transport — Serial I/O

Non-blocking serial reader that feeds raw bytes to the protocol decoder.

## Files

| File | Description |
|------|-------------|
| `serial_transport.py` | `SerialTransport` — opens a pyserial port in a background thread; exposes `read_pending()` to drain buffered bytes into the caller |

## Design

- Background thread reads continuously; bytes are queued in a `collections.deque`
- `read_pending()` drains the queue — called each frame in the main render loop via `session.process_pending()`
- Connection errors (device unplug, reset) are caught silently and trigger a reconnect attempt
