{ pkgs ? import <nixpkgs> {} }:

(pkgs.buildFHSEnv {
  name = "arduino-ide-env";
  targetPkgs = pkgs: with pkgs; [
    arduino-ide
    udev
    alsa-lib
    zlib
    glib
    xorg.libX11
    xorg.libXext
    xorg.libXrender
    xorg.libXtst
    xorg.libXi
    fontconfig
    gtk3
  ];
  runScript = "arduino-ide";
}).env

