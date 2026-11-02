{ pkgs ? import <nixpkgs> {} }:

let
  # FHS chroot so Arduino IDE finds all its native libraries
  arduinoFHS = pkgs.buildFHSEnv {
    name = "arduino-ide-env";
    targetPkgs = pkgs: with pkgs; [
      arduino-ide
      udev alsa-lib zlib glib
      xorg.libX11 xorg.libXext xorg.libXrender xorg.libXtst xorg.libXi
      fontconfig gtk3
      doxygen
    ];
    runScript = "arduino-ide";
  };

  # Python environment — pin to 3.12; open3d has no wheel for 3.13
  pythonEnv = pkgs.python312.withPackages (p: with p; [
    numpy
    pyserial
  ]);
in
pkgs.mkShell {
  buildInputs = [
    arduinoFHS
    pythonEnv
    # OpenGL / window-system libs required by DearPyGUI at runtime
    pkgs.libGL
    pkgs.xorg.libX11
    pkgs.libxi          # was xorg.libXi — renamed in nixpkgs 26.05
    pkgs.xorg.libXcursor
    pkgs.xorg.libXrandr
    pkgs.xorg.libXinerama
    pkgs.glib           # libgthread-2.0 — needed by open3d
    pkgs.udev           # libudev.so.1 — needed by open3d
    pkgs.libusb1        # libusb-1.0.so.0 — needed by open3d
    pkgs.tbb            # libtbb.so — needed by open3d
    pkgs.zlib           # libz.so.1
    pkgs.vulkan-loader  # libvulkan.so.1 — open3d rendering backend
    pkgs.libidn2.out    # libidn2.so.0 — needed by open3d (bin is default output; .out has the .so)
  ];

  shellHook = ''
    echo "Portable 3D Scanner dev shell"
    echo "  Arduino IDE  →  arduino-ide-env"
    echo "  Python       →  $(python3 --version)"
    echo "  Docs         →  cd ArduinoProject && ./generate_docs.sh"

    VENV="$PWD/PythonProject/.venv"
    if [ ! -d "$VENV" ]; then
      echo "Creating Python venv and installing packages..."
      python3.12 -m venv "$VENV" --system-site-packages
      "$VENV/bin/pip" install dearpygui trimesh rtree open3d
    fi
    source "$VENV/bin/activate"

    # Expose libstdc++, OpenGL, X11, udev to pre-compiled wheels.
    # On NixOS these are not in /usr/lib so the bundled .so can't find them.
    export LD_LIBRARY_PATH="${pkgs.stdenv.cc.cc.lib}/lib:${pkgs.libGL}/lib:${pkgs.xorg.libX11}/lib:${pkgs.libxi}/lib:${pkgs.glib}/lib:${pkgs.udev}/lib:${pkgs.libusb1}/lib:${pkgs.tbb}/lib:${pkgs.zlib}/lib:${pkgs.vulkan-loader}/lib:${pkgs.libidn2.out}/lib:$LD_LIBRARY_PATH"
  '';
}
