# src

The code of the app, in layers. A module imports only from the layers below its own, so each
layer can be read, tested and reused without the ones above it.

| Layer | Folder | Job | May use |
|---|---|---|---|
| view | [`gui/`](gui/README.md) | the DearPyGui windows | everything below |
| controller | [`controller/`](controller/README.md) | app state and commands, no GUI | services, storage, session, protocol, transport, processing |
| services | [`services/`](services/README.md) | use cases as functions | storage, session, processing |
| storage | [`storage/`](storage/README.md) | files: records, presets, paths | session, processing |
| session | [`session/`](session/README.md) | state built from the board's messages | protocol, processing |
| protocol | [`protocol/`](protocol/README.md) | lines <-> typed messages | numpy |
| transport | [`transport/`](transport/README.md) | the serial port | pyserial |
| processing | [`processing/`](processing/README.md) | calibration maths: a reusable library | numpy |

The diagram is [`../docs/diagrams/out/architecture.svg`](../docs/diagrams/out/architecture.svg).

The conventions are the same everywhere:
- Each module lists its public names in `__all__`; names starting with `_` are private.
- Every module, class and function has a docstring.
- Names carry their units: `_g`, `_mg`, `_ug`, `_c` (degC), `_s`, `_deg`, `_rad`.
