#!/bin/bash -xe

CFLAGS=`pkg-config --cflags opencv`
LIBS=`pkg-config --libs opencv`
LIBS+=" -lm -lpthread"

g++ -O3 $CFLAGS $1.cpp -o $1 $LIBS
