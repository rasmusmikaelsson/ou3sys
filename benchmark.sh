#!/bin/bash

# Stop on error
set -e
	
# Path to benchmark
DIR="/usr"

# Build program
make clean
make

echo "threads time"

for T in $(seq 1 100); do
	TIME=$(/usr/bin/time -f "%e" ./mdu -j "$T" "$DIR" 2>&1 > /dev/null)
	echo "$T $TIME"
done







