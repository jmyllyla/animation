#!/bin/bash
rm -rf build/animation
cmake -S . -B build
cmake --build build
./build/animation