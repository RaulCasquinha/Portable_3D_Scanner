# ArduinoProject

ESP32-S3 firmware for the portable 3D scanner, developed in Arduino IDE.

## Structure

```
ArduinoProject/
├── PortableScanner3D/          Main Arduino sketch (open this in Arduino IDE)
│   ├── PortableScanner3D.ino   Entry point — setup(), loop(), accumulator logic
│   └── src/                    Layered source modules (see src/README.md)
├── Doxyfile                    Doxygen configuration for API docs
└── generate_docs.sh            Script to regenerate HTML docs into doxy_docs/
```

## Building

1. Open Arduino IDE via `nix-shell shell.nix` from the repo root
2. Open `PortableScanner3D/PortableScanner3D.ino`
3. Install required libraries via **Tools → Manage Libraries**:
   - **VL53L0X** by Pololu
   - **Arduino_LSM6DS3** by Arduino
4. Select board: **ESP32-S3 Dev Module**
5. Set **Tools → USB CDC On Boot → Enabled**
6. Upload

## Generating API docs

```bash
cd ArduinoProject
bash generate_docs.sh
# Opens doxy_docs/html/index.html
```

`doxy_docs/` is git-ignored — regenerate locally as needed.
