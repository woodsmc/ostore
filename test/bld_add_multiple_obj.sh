#!/bin/bash

gcc -Wall -I ../inc ../src/ostore_internal.c ../src/ostore.c ./add_multiple_obj.c -o test_add_multiple_obj
