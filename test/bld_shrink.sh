#!/bin/bash

gcc -Wall -I ../inc ../src/ostore_internal.c ../src/ostore.c ./shrinkobject.c -o test_shrink