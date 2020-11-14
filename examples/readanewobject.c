#include "ostore.h"
#include <stdio.h>
#include <string.h>

static const char* const FILENAME = "test.store";
static const char* const STRING_TO_SAVE = "Hey, this is an example string to store!";
static const TOStoreObjID ID = 42;

int main(int argc, const char* argv[]) {
    TOStoreHnd store = NULL;
    int error = ostore_open(FILENAME, EReadOnly, &store);
    if (error != ERR_OK || store == NULL) {
        printf("There was an error opening the object store %d\n", error);
        return error;
    }
    const uint32_t size = 0;
    error = ostore_getLength(store, ID, &size);
    if ( error != ERR_OK) {
        printf("there was an error getting the object length %d", error);
    } else {
        char* buffer = malloc(size);
        if ( buffer == NULL ) {
            printf("unable to allocate enough memory for the buffer of size %u\n", size);
        } else {
            memset(buffer, 0, size);
            error = 
            if ( error != ERR_OK ) {
                printf("there was an error writing to the store %d", error);
            }
        }

    }
    ostore_close(&store);
    printf("String saved in the test store\n");
    return 0;
}
