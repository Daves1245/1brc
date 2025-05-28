#!/bin/bash

# Check if measurements.txt already exists in /tmp
if [ ! -f "/tmp/measurements.txt" ]; then
    echo "copying measurements.txt to /tmp..."
    cp data/measurements.txt /tmp
else
    echo "measurements already in /tmp, skipping."
fi

if [ ! -f "/tmp/measurements10k.txt" ]; then
    echo "copying measurements10k.txt to /tmp..."
    cp data/measurements10k.txt /tmp
else
    echo "measurements10k already in /tmp, skipping."
fi



# compile
echo "compiling"
echo "gcc -Ofast -Wall -Wextra -funroll-loops -march=native main.c -o run..."
gcc -Ofast -Wall -Wextra -funroll-loops -march=native main.c -o run
echo "done."
