#!/bin/bash
mkdir -p bin
gcc -Wall -I ../inc -I ../export  ../src/ostore_internal.c ../src/ostore.c ./add_delete_object.c -o ./bin/test_add_delete_obj
