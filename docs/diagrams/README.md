# diagrams

Software architecture and data-flow diagrams for the Portable 3D Scanner.  
All sources are D2 (`.d2`) — rendered SVGs are committed alongside them.

## Files

D2 sources are committed; rendered SVGs go into `out/` (git-ignored — regenerate locally).

| Source | Description |
|--------|-------------|
| `system_overview.d2` | High-level: hardware → firmware → host PC, three tiers |
| `software_architecture.d2` | Full layered architecture with all modules and data flows |
| `packet_protocol.d2` | Binary packet framing, packet types, and decoder state machine |
| `measurement_pipeline.d2` | Sequence diagram: one distance reading → 3D point → cluster → mesh |

## Rendering to out/

```bash
cd docs/diagrams
d2 --layout=tala system_overview.d2        out/system_overview.svg
d2 --layout=tala software_architecture.d2  out/software_architecture.svg
d2 --layout=tala packet_protocol.d2        out/packet_protocol.svg
d2 --layout=tala measurement_pipeline.d2   out/measurement_pipeline.svg
```

D2 is available in the nix shell (`nix-shell shell.nix` from the repo root). TALA produces significantly better layouts for complex diagrams than the default engine.

## Keep in sync

Update the relevant `.d2` file and re-render the SVG whenever:
- A new source module is added or removed
- The data flow between modules changes
- The packet format changes
