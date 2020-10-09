#!/bin/bash
mkdir -p bin
gcc -I ../inc -I ../export  ../src/ostore_internal.c ../src/ostore.c ./makeempty.c -o bin/test_makeempty
