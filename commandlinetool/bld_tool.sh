#!/bin/bash
mkdir -p bin
g++ -Wall -I ../inc -I ../export  -I . ../src/ostore_internal.c ../src/ostore.c ./parameters.cpp ./commandlinetool.cpp -o ./bin/ostore
