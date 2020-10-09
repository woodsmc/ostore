#!/bin/bash
mkdir -p bin
gcc -Wall -I ../inc -I ../export  ../src/ostore_internal.c ../src/ostore.c ./add_multiple_obj.c -o bin/test_add_multiple_obj
