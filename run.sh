#!/bin/bash
rm -rf build/animation
cmake -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" -DCMAKE_C_COMPILER="$(xcrun --find cc)" -S . -B build
cmake --build build
./build/animation