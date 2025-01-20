#!/bin/bash

# Run five instances of daytime_client in the background
./daytime_client & 
./daytime_client & 
./daytime_client & 
./daytime_client & 
./daytime_client & 

# Wait for all background processes to finish
wait
