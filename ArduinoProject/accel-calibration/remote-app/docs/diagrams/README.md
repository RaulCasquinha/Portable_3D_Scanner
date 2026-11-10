# docs/diagrams

The diagrams, written in [D2](https://d2lang.com). The `.d2` files are the sources; `out/` holds
the images compiled from them.

| Source | Shows |
|---|---|
| `architecture.d2` | the layers of `src/`, their modules and windows, and who uses whom |

Compile them all after every change (Git Bash on Windows, or any bash):

```
bash docs/diagrams/compile.sh
```

`compile.sh` empties `out/` and writes `out/<name>.svg` and `out/<name>.png` for every `.d2`
here, so a new diagram only needs its `.d2` file. It needs `d2` on the PATH.

The protocol between the app and the firmware, and the whole system, are drawn in the
project root, [`../../../docs/diagrams/`](../../../docs/diagrams/README.md).

One idea per diagram; implementation details stay in the code.
