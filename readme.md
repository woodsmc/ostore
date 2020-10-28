# Object Store (oStore)

Storing variable length data, strings, arrays of objects, etc, is often part of a IoT Device's configuration. These can be things such as domain names to connect to, or MQTT topics to listen to. Storing these variable length data sets in a single file is problematic, it can require the rewriting of an entire file when just one setting or parameter has changed, or storing data scattered across multiple files. This can be really cumbersome.

I searched for solutions to this problem [I posted on Reddit in the C](https://www.reddit.com/r/C_Programming/comments/i31zwf/library_to_help_store_multiple_objects_inside_a/?utm_source=share&utm_medium=web2x&context=3) and in the [IoT Subreddits](https://www.reddit.com/r/IOT/comments/i3ikwn/library_to_help_store_multiple_objects_inside_a/) and even though we discussed a lot of ideas, we didn't find a viable solution. Most of the suggestions involved storing all the information in RAM and committing it to disk in one go. This is a viable solution on large systems, but for systems built on micro controllers with ~ 340kb of RAM that may not be a viable solution.

## The OStore Benefit

OStore solves this, it provides a simple API for storing variable length data within a single file. Each entry is identified by a unique 32 bit number and can be access by using this object ID. Objects can be added, removed and resized at will. 

### Writing a new object



### Updating an object



### Reading an object

