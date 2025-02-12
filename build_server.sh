#!/bin/bash

# Remove previous build artifacts
rm -f async_server clients_log.txt
rm -rf build  # Remove old build directory if it exists

# Create a new build directory
mkdir -p build
cd build || exit

# Run CMake to generate build files
cmake ..

# Compile the project
cmake --build .

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful: async_server created."
    mv async_server ..
else
    echo "Compilation failed."
    exit 1
fi

cd ..