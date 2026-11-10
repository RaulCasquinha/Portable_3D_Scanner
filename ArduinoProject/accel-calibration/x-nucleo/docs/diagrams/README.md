# docs/diagrams

The firmware diagrams, written in [D2](https://d2lang.com). The `.d2` files are the sources;
`out/` holds the images compiled from them.

| Source | Shows |
|---|---|
| `architecture.d2` | the layers of the firmware, their modules, and who uses whom |

Compile them after every change (Git Bash, or any bash):

```
bash Accel_Calib_stm32/docs/diagrams/compile.sh
```

The script empties `out/` and writes `out/<name>.svg` and `.png` for every `.d2` here.
