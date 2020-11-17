# Object Store (oStore)

Storing variable length data, strings, arrays of objects, etc, is often part of a IoT Device's configuration. These can be things such as domain names to connect to, or MQTT topics to listen to. Storing these variable length data sets in a single file is problematic, it can require the rewriting of an entire file when just one setting or parameter has changed, or storing data scattered across multiple files. This can be really cumbersome to manage.

Large sets of data can be impossible to hold in RAM simultaneously, as the platform simply may not have enough available RAM to hold everything at once. Indeed, for periodic connections where, for instance MQTT sessions are setup at specified intervals and then disconnected not all the information needs to held in RAM all the time anyway. 

## Original Motivation

I searched for solutions to this problem [I posted on Reddit in the C](https://www.reddit.com/r/C_Programming/comments/i31zwf/library_to_help_store_multiple_objects_inside_a/?utm_source=share&utm_medium=web2x&context=3) , [IoT](https://www.reddit.com/r/IOT/comments/i3ikwn/library_to_help_store_multiple_objects_inside_a/) , and [the Embedded subreddits](https://www.reddit.com/r/embedded/comments/jo3wmz/a_key_value_store_for_embedded_devices_any/). The discussions highlighted the small niche of the problem space. Ether devices are very small, in which connectivity is via a local link to a gateway device which does all the heavy lifting and needs the larger set of configuration settings, or the devices are larger and more capable. The more capable devices could use something like SQLite for storage. 

However, for me, there is a middle ground. A set of devices which are cheap, connect to the internet, but don't have the capability to run SQLite.

In the forum posts we looked at alternative solutions, but I didn't feel we found one that would allow the device to handle a large set of data without having to store it all in RAM, or have double the available persistent storage.

My ideal solution would be a storage solution that would work on these middle ground devices, and up, including Raspberry Pi's and the like. 

### Alternatives

The alternatives I considered where:

- [RocksDB](https://rocksdb.org/) : Like the API, but too heavy for the platform
- [Sophia Embeddable Key Value Store](http://sophia.systems/) : Looks complicated for what I needed
- [SQLite](https://sqlite.org/index.html) Well loved and for good reason, but a bit too heavy for what I need
- C++ std::map  - Nice look up speed, but everything is held in RAM, I'd like the std::map on disk / file.

### Why OStore

"Because it is there" - is often the answer when you ask a mountaineer why they climb. Hey, writing this was a bit of fun, and I wanted to have a go. It might be useful to others, so I'll share it here. In addition the alternatives outlined above all seemed to not quite fit. So, hey, why not try.

## Outline Requirements

The original requirements were to store basic connectivity settings, these would include:

- Server name / IP address
- Port number
- MQTT credentials
- MQTT topics and QoS

The OStore code can be directly included in a project, or built as a library and linked to on Linux based systems. To provides both a command line tool for inspecting and creating objects that can be used as part of the built process for projects which need to built an initial configuration file / initial set of settings. 

## How OStore Works

OStore provides storage for variable length objects, where each object is identified by a 32 bit UUID. We do not store strings as variable names, but users could implement their own hash function to generate a 32 bit UUID from a string. OStore provides a simple API for storing variable length data within a single file. Each entry is identified by a unique 32 bit number and can be access by using this object ID. Objects can be added, removed and resized at will. 

## Detailed Documentation

More detailed documentation can be found at the following locations:

- [Command Line Tool and Usage](docs/ostorecommandlinetool.md)
- [API Specification](docs/ostoreapi.md)
- [File Format Specification](docs/ostorefileformat.md)  



## Examples

### Writing a new object

```c
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
```



### Updating an object

... To do... 

### Reading an object

... To do.. 

