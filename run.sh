#!/bin/bash

set -e

gcc main.c -Wall -o main -lSDL2
./main
rm main