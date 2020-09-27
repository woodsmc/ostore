#!/bin/bash

gcc -I ../inc ../src/ostore_internal.c ../src/ostore.c ./makeempty.c -o test_makeempty
