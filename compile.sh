#!/bin/sh
gcc main.c -O3 -DDROIDIAN -o particles -I/usr/include/SDL2  -lSDL2main -lSDL2 -lm -lpthread
