#!/bin/bash
mkdir -p bin
gcc -Wall -I ../inc -I ../export ../src/ostore_internal.c ../src/ostore.c ./makestring.c -o bin/test_makestring
