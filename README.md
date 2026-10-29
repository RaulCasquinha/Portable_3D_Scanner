# TFM
Trabalho final de mestrado

# Tasks to do
## Hardware
### Sensors
- [ ] LiDAR Driver
- [ ] IMU Driver
- [ ] Main App


### Extra features
- [ ] Make handheld holder modular

## Software
- [ ] Point Collecting app
- [ ] Store point cloud a standard file format
- [ ] Debug app
- [ ] Conversion from point cloud to mesh


### Extra features
- [ ] Make wireless connection


## Report
- [ ] State of the art


# Documentation
## 3D models
I made the 3D handle in a paper so i cound visualize the size, then i measured it with a calliper and made a digital twin, then it was printed in PLA 1.75mm on my Ender 3 V2.

## Firmware
The firmware needs to be able to send points to a serial port

## Software
The software needs to be able to process the data and create a model out of it

## File Structure

```
Portable_3D_Scanner/
├── 3DModels/                              # All 3D design and print files
│   ├── Body.FCStd                         # FreeCAD source file for the handheld body
│   ├── Body.stl                           # STL export of the body for slicing/printing
│   ├── MountedPiece.stl                   # STL for the sensor mounting bracket
│   ├── CE3E3V2_Body.gcode                 # G-code for printing the body on Ender 3 V2
│   ├── CE3E3V2_MountedPiece.gcode         # G-code for printing the mount on Ender 3 V2
│   └── ExtraModels/                       # Downloaded 3rd-party component 3D models
│       ├── esp32-s3-wroom-1-1.*/          # ESP32-S3-WROOM-1 module model
│       ├── female-header-pins-2.*/        # Female pin header models (2–10 pin)
│       ├── lsm6ds3-imu-1.*/              # LSM6DS3 IMU module model
│       ├── universal_board_40x60mm-1.*/   # 40×60 mm prototype board model
│       └── vl53l0x-4.*/                  # VL53L0X ToF distance sensor model
├── docs/                                  # Project documentation
│   ├── datasheets/                        # Component datasheets (PDF)
│   │   ├── DS_vl53l0x-1.pdf              # VL53L0X ToF sensor datasheet
│   │   └── esp-dev-kits-en-master-esp32s3.pdf  # ESP32-S3 dev kit documentation
│   ├── diagrams/                          # Architecture and design diagrams
│   │   ├── software_architecture.d2      # D2 source for the software architecture diagram
│   │   └── software_architecture.svg     # Rendered SVG of the architecture diagram
│   └── report/                            # LaTeX project report
│       ├── paper.tex                      # Main LaTeX source file
│       ├── paper.pdf                      # Compiled report PDF
│       └── bibliography/                  # BibTeX references
│           ├── references.bib             # Bibliography entries
│           └── unresolvedReferences.md    # Notes on references not yet added to .bib
├── ArduinoProject/                        # Arduino/firmware project
│   ├── TFM.ino                            # Arduino sketch file (entry point for Arduino IDE)
│   ├── main.cpp                           # Legacy firmware source
│   └── src/                              # Firmware source code
│       └── main.cpp                       # Main firmware entry point
├── PythonProject/                         # Python project (point cloud processing)
├── shell.nix                              # Nix shell for a reproducible development environment
└── README.md                              # This file
```