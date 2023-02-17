# Xash3D: Ray Traced
[![GitHub Actions Status](https://github.com/FWGS/xash3d-fwgs/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/FWGS/xash3d-fwgs/actions/workflows/c-cpp.yml) 

Xash3D: Ray Traced is a variation of the Xash3D FWGS engine, with a [custom path traced renderer](https://github.com/sultim-t/RayTracedGL1).

## Install
Latest compiled build for Half-Life 1 can be found at https://github.com/sultim-t/xash-rt/releases

## Build
### Windows (Visual Studio)
1) Clone this git repository
1) Setup dependencies
	- [RayTracedGL1](https://github.com/sultim-t/RayTracedGL1/releases/download/hl1-dev) (`RTGL1_SDK_PATH` environment variable should point to it)
	- [SDL2 2.0.14](http://libsdl.org/release/SDL2-devel-2.0.14-VC.zip) (should be put into the `SDL2_VC` folder)
	- [Half-Life 1 SDK for Xash](https://github.com/sultim-t/hlsdk-xash3d/releases)
	- for more info, look Github Actions scripts ([c-cpp.yml](https://github.com/sultim-t/xash-rt/blob/ref_rt/.github/workflows/c-cpp.yml), [deps_win32.sh](https://github.com/sultim-t/xash-rt/blob/ref_rt/scripts/gha/deps_win32.sh))
1) Open Visual Studio solution `xash3d-fwgs.sln`
1) Build solution
