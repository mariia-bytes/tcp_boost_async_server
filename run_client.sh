#!/bin/bash

# Run five instances of async_client in the background
./async_client & 
./async_client & 
./async_client & 
./async_client & 
./async_client & 

# Wait for all background processes to finish
wait
