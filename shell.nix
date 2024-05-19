with import <nixpkgs> {};
{ pkgs ? import <nixpkgs> {} }:

stdenv.mkDerivation {
  name = "cryptanalysislib";
  src = ./.;

  buildInputs = [ 
    cmake
  	git 
	gnumake 
	cmake 
    gcc
    capstone
    pkg-config
    glfw
    wayland-scanner
    freetype
    xorg.libX11
    xorg.libXau
    dbus
    linuxKernel.packages.linux_6_1.perf
    tbb
    gdb
    elfutils
  ];

  shellHook = '' 
    CC=gcc
    CXX=g++
  '';
}
