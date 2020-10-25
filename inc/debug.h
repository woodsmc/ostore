#pragma once
#include <stdio.h>

#ifdef DEBUG_ON
#define PRINTF printf("> FUNCTION %s:> | ", __PRETTY_FUNCTION__ ); printf("| "); printf
#else
#define PRINTF
#endif // DEBUG_ON
