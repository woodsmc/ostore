#include "ostore.h"
#include <stdio.h>
#include <string.h>

static const char* const FILENAME = "test.store";
static const char* const STRING_TO_SAVE = "Hey, this is an example string to store!";
static const TOStoreObjID ID = 42;

int main(int argc, const char* argv[]) {
    TOStoreHnd store = NULL;
    int error = ostore_create(FILENAME, &store);
    if (error != ERR_OK || store == NULL) {
        printf("There was an error creating an object store %d\n", error);
        return error;
    }
    const uint32_t size = strlen(STRING_TO_SAVE) + 1; // null terminator
    error = ostore_addObjectWithId(store, ID, size);
    if ( error != ERR_OK) {
        printf("there was an error saving the object %d", error);
    } else {
        error = ostore_write(store, ID, 0, STRING_TO_SAVE, size);
        if ( error != ERR_OK ) {
            printf("there was an error writing to the store %d", error);
        }
    }
    ostore_close(&store);
    printf("String saved in the test store\n");
    return 0;
}
