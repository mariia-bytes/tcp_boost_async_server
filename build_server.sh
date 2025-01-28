#!/bin/bash

# remove async_server (the compiled binary from a previous run)
# and clients_log.txt (log file from previous execution) if they exist
rm -f async_server clients_log.txt

# compile async_server
g++ -std=c++23 -pthread -o async_server async_server.cpp

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful: async_server created."
else
    echo "Compilation failed."
    exit 1
fi