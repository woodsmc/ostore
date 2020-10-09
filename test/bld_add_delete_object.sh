#!/bin/bash

gcc -Wall -I ../inc -I ../export  ../src/ostore_internal.c ../src/ostore.c ./add_delete_object.c -o test_add_delete_obj
