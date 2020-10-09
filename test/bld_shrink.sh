#!/bin/bash

gcc -Wall -I ../inc -I ../export ../src/ostore_internal.c ../src/ostore.c ./shrinkobject.c -o test_shrink
