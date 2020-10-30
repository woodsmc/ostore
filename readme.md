# Object Store (oStore)

Storing variable length data, strings, arrays of objects, etc, is often part of a IoT Device's configuration. These can be things such as domain names to connect to, or MQTT topics to listen to. Storing these variable length data sets in a single file is problematic, it can require the rewriting of an entire file when just one setting or parameter has changed, or storing data scattered across multiple files. This can be really cumbersome.

I searched for solutions to this problem [I posted on Reddit in the C](https://www.reddit.com/r/C_Programming/comments/i31zwf/library_to_help_store_multiple_objects_inside_a/?utm_source=share&utm_medium=web2x&context=3) and in the [IoT Subreddits](https://www.reddit.com/r/IOT/comments/i3ikwn/library_to_help_store_multiple_objects_inside_a/) and even though we discussed a lot of ideas, we didn't find a viable solution. Most of the suggestions involved storing all the information in RAM and committing it to disk in one go. This is a viable solution on large systems, but for systems built on micro controllers with ~ 340kb of RAM that may not be a viable solution.

## The OStore Benefit

OStore solves this, it provides a simple API for storing variable length data within a single file. Each entry is identified by a unique 32 bit number and can be access by using this object ID. Objects can be added, removed and resized at will. 

### Writing a new object

```c
#include "ostore.h"
#include <stdio.h>
#include <string.h>

static const char* const FILENAME = "test.store";
static const char* const STRING_TO_SAVE = "Hey, this is an example string to store!";

int main(int argc, const char* argv[]) {
    TOStoreHnd store = NULL;
    int error = ostore_create(FILENAME, &store);
    if (error != ERR_OK || store == NULL) {
        printf("There was an error creating an object store %d\n", error);
        return error;
    }
    const uint32_t size = strlen(STRING_TO_SAVE) + 1; // null terminator
    error = ostore_addObjectWithId(store, STRING_TO_SAVE, size);
    if ( error != ERR_OK) {
        printf("there was an error saving the object %d", error);
    
    }
    ostore_close(&store);
    printf("String saved in the test store\n");
    return 0;
}
```



### Updating an object



### Reading an object

