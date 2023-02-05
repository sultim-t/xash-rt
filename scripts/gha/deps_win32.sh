#!/bin/bash

curl http://libsdl.org/release/SDL2-devel-$SDL_VERSION-VC.zip -o SDL2.zip
unzip -q SDL2.zip
mv SDL2-$SDL_VERSION SDL2_VC

curl -L "https://github.com/sultim-t/RayTracedGL1/releases/download/v3.0.0-dev/RayTracedGL1-Bundle_050223.zip" > RTGL1.zip
unzip -q RTGL1.zip -d RTGL1

mkdir -p RTGL1/Build/x64-Release
mv RTGL1/bin/RayTracedGL1.lib RTGL1/Build/x64-Release/RayTracedGL1.lib
mv RTGL1/bin/RayTracedGL1.dll RTGL1/Build/x64-Release/RayTracedGL1.dll
