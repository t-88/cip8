#!/bin/bash

set -e

gcc main.c -o main -lSDL2
./main
rm main