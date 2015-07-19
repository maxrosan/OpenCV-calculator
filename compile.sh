#!/bin/bash -xe

CFLAGS=`pkg-config --cflags opencv`
CFLAGS+=`Magick++-config --cppflags --cxxflags`
LIBS=`pkg-config --libs opencv libcurl`
LIBS+=" -lfftw3 "
LIBS+=`Magick++-config --ldflags --libs`
LIBS+=" -lm"

g++ -O3 $CFLAGS $1.cpp -o $1 $LIBS