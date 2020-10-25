#pragma once
#include <stdio.h>

struct _FN_DEBUG_ {
    const char* name;
    static size_t depth;
    _FN_DEBUG_(const char* name) {
        this->name = name;
        fill();
        printf("> FUNCTION:> %s\n", name);
        depth++;
    }

    ~_FN_DEBUG_() {
        depth--;
        fill();
        printf("< FUNCTION:< %s\n", name);
        
    }
    
    static void fill() {
        for(int i = 0; i < depth; i++) {
            printf(" ");
        }
    }
};

#ifdef DEBUG_ON
#define FNPRT  _FN_DEBUG_ _dbg_( __PRETTY_FUNCTION__)
#define PRINTF _FN_DEBUG_::fill(); printf("| "); printf
#else 
#define FNPRT 
#define PRINTF
#endif // DEBUG_ON