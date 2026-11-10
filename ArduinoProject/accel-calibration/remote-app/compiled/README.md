# compiled

The app as one Windows executable, `AccelCalib.exe`. It runs on a PC without Python: copy this
folder and double-click the exe.

The executable keeps its data next to itself:

| Here | Contents |
|---|---|
| `AccelCalib.exe` | the app |
| `calibrations/` | the records it saves (created on the first Save record) |
| `presets/` | its presets; the build copies the project's presets here. `Default` is loaded at start |
| `workspaces/` | its workspaces; the build copies the project's workspaces here. The first by name opens at start |
| `app_layout.ini` | what DearPyGui keeps of the windows on this PC (created on the first close) |

To build it again after a change, from the app folder with the venv active:

```
pip install -r requirements-dev.txt
python build_exe.py
```

The build replaces `AccelCalib.exe` and never touches `calibrations/` or existing presets and workspaces.
