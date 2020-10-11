#!/bin/bash
mkdir -p bin
g++ -Wall -I ../inc -I ../export  ../src/ostore_internal.c ../src/ostore.c ./commandlinetool.cpp -o ./bin/ostore
