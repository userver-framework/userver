#!/bin/sh

if [ "$COMPILER" = gcc ]; then
    echo CC=cc CXX=g++
elif [ "$COMPILER" = clang ]; then
    echo CC=clang-18 CXX=clang++-18
else
    echo >&2 "Unknown COMPILER=$COMPILER"
    exit 1
fi
