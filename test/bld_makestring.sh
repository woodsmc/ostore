#!/bin/bash

gcc -Wall -I ../inc -I ../export ../src/ostore_internal.c ../src/ostore.c ./makestring.c -o test_makestring
