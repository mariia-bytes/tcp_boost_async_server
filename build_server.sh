#!/bin/bash

# remove previous build artifacts
rm -f async_server
rm -rf build  # remove old build directory if it exists
rm -rf logs # remove old logs difectory if it exists

# create a new build directory
mkdir -p build
cd build || exit

# cun CMake to generate build files
cmake ..

# compile the project
cmake --build .

# check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful: async_server created."
    mv async_server ..
else
    echo "Compilation failed."
    exit 1
fi

cd ..