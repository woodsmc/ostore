#!/bin/bash

gcc -I ../inc -I ../export  ../src/ostore_internal.c ../src/ostore.c ./readstore.c -o test_readstore
