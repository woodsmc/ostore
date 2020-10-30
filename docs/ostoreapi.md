# OStore API

## Overview

Flat file structures make storing of variable length objects (strings, arrays etc) difficult to do without rewriting the entire file. Rewriting the file is a time, and resource consuming effort. OStore addresses this by providing a structured file format which allows multiple "objects" of variable length to be stored within a single file without the need to rewrite the entire file.

## Purpose of this Document

This document describes the API provided by OStore. It does not define the file format which is detailed in a separate document. **TODO: INSERT LINK**

## Document Version History

| Version | Date            | Author      | Comment                                            |
| ------- | --------------- | ----------- | -------------------------------------------------- |
| 0.1     | August 4 2020   | Chris Woods | Initial concept documented                         |
| 0.2     | August 27 2020  | Chris Woods | Refined as part of implementation                  |
| 0.3     | October 4 2020  | Chris Woods | Updated API function names                         |
| 0.4     | October 29 2020 | Chris Woods | Small correction to function names for consistency |



# Concepts

The OStore library is based around the concept of "objects".  A single OStore file will contain a series of objects. Each object is a variable length entity. This could be an array, a data structure, a string or byte sequence. As a user we can open a file, enumerate though the stored objects, open objects query their length, and then read and write from the object.

<insert image of file containing multiple objects>

## How Objects are Handled

File Systems today inherit concepts that originated with tape drives. The [POSIX / 'C' file API](https://en.wikipedia.org/wiki/C_file_input/output) presents a streaming interface to files, with an implicit "head" then API then supports moving forward, backward or to specific locations within the file before performing read or write operations. This is akin to fast-forwarding or rewinding tapes to get to the correct location before performing a read / write operation. In today's modern systems with storage provided by non-mechanical media and nonvolatile storage this is crazy, and it doesn't reflect how modern software operates, or how the underlying physical media performs. 

OStore  instead presents an API which is more akin to RAM access. Thus objects can be allocated and then freed. Read and write operations within an object are performed with an API more akin to [`memcpy`](https://cplusplus.com/reference/cstring/memcpy/).

## Object Definition

An Object has the following properties:

- ID : an unsigned 32 bit value which acts as a UUID for an Object.
- Length : an unsigned 32 bit value which provides the length of an object in bytes.
- A sequence of bytes which can read from and written to.

Objects need to be "Opened" prior to reading or writing, and close when operations on them have completed. This allows the library to load the necessary information about the object into RAM and to release this data from RAM when the object is closed. Additionally the object can be truncated upon closing. This can facilitate the release of space on the disk.

# API Documentation

## Data Structures

## Store Handling

### Data Types

#### `TOStreamMode`

```c
typedef enum {
    EReadOnly,
    EReadWrite
} TOStreamMode;
```

This enumerated data type is used by `ostore_open` to indicate how a file should be opened, either as a read only mode (`EReadOnly`) or in a Read and Write mode (`EReadWrite`).

### Functions

#### `ostore_open`

```c
int ostore_open(const char* filename, TOStreamMode mode, TOStoreHnd* oStore)
```

**Function Description:** Opens an OStore file. The file can be opened in read only, or read/write modes. If the file doesn't exist then an error will be returned.
**Parameters:** 

- `filename` : a string representation of the filename to open
- `mode` : See data structures above, this is the mode to open the file.
- `oStore`: A pointer to a OStore handle, this handle will be set correctly upon success

**Returns**

- On success:
  - `oStore` contains a handle to a valid `oStore` object
  - returns `0`
- On Failure:
  - `oStore` value is undefined (do not use it)
  - returns a system error code indicating failure

Notes:

This function will assert on:

- `oStore` contains NULL
- `filename` is NULL
- `mode` is not one of the pre-supplied enum values



#### `ostore_create`

```c
int ostore_create(const char* filename, TOStoreHnd* oStore)
```

**Function Description:** Creates an OStore file. If the file cannot be created,  then an error will be returned. By default this opens the file with `EReadWrite`.
**Parameters:** 

- `filename` : a string representation of the filename to open
- `oStore`: A pointer to a OStore handle, this handle will be set correctly upon success

**Returns**

- On success:
  - `oStore` contains a handle to a valid `oStore` object
  - returns `0`
- On Failure:
  - `oStore` value is undefined (do not use it)
  - returns a system error code indicating failure

Notes:

This function will assert on:

- `oStore` contains NULL
- `filename` is NULL



#### `ostore_close`

```c
void ostore_close(TOStoreHnd* oStore);
```

**Function Description:** Closes an open OStore file.
**Parameters:** 

- `oStore`: A pointer to an open OStore handle.

**Returns**

- Nothing

Notes:

This function will assert on:

- `oStore` contains NULL or is not valid

  

## Object Inspection

#### `ostore_enumerateObjects`

```c
int ostore_enumerateObjects(TOStoreHnd oStore, uint32_t* numerOfObjects);
```

**Function Description:** Enumerates the available objects within an open OStore file.
**Parameters:** 

- `oStore`: A pointer to a OStore handle, this handle will be set correctly upon success
- `numberOfObjects` : A pointer to an unsigned 32 bit value. Upon success this will contain the number of objects found within the file.

**Returns**

- On success:
  - `numberOfObjects` contains the number of objects found in the file.
  - returns `0`
- On Failure:
  - `numberOfObjects` is undefined.
  - returns a system error code indicating failure

Notes:

This function will assert on:

- `oStore` is not valid.
- `numberOfObjects` is NULL.

#### `ostore_getObjectIdFromIndex`

```c
int ostore_getObjectIdFromIndex(TOStoreHnd oStore, uint32_t objectIndex,
                                TOStoreObjID* id);
```

**Function Description:** Returns the ID for the object indicated by `objectIndex`. The ID value is needed for object handling.
**Parameters:** 

- `oStore`: A pointer to a OStore handle, this handle will be set correctly upon success
- `objectIndex` : The index value of the object to get the ID for.
- `id` : A pointer to an `TOStoreObjID` , on success this contains a valid ID for an object.

**Returns**

- On success:
  - `id` contains the ID number for the requested object
  - returns `0`
- On Failure:
  - `numberOfObjects` is undefined.
  - returns a system error code indicating failure

Notes:

This function will assert on:

- `oStore` is not valid.
- `numberOfObjects` is out of range, you've requested an index which is not present within the file.
- `id` : The pointer is NULL

#### `ostore_getObjectIdFromIndex`

```c
int ostore_objectIdExists(TOStoreHnd oStore, TOStoreObjID id);
```

**Function Description:** Indicates if an object with the `id` exists in the store.
**Parameters:** 

- `oStore`: A pointer to a OStore handle, this handle will be set correctly upon success
- `id` : The ID to check for.

**Returns**

- On success:
  - returns `0`
- On Failure:
  - returns a system error code indicating failure

Notes:

This function will assert on:

- `oStore` is not valid.

## Object Manipulation

### Creating and Destroying Objects

#### `ostrore_addObjectWithId`

```c
int ostore_addObjectWithId(TOStoreHnd oStore, TOStoreObjID id, uint32_t length);
```

**Function Description:** Adds an object to the store with a specified ID.

**Parameters:** 

- `oStore`: An OStore handle, for an open OStore.
- `id` : The ID to assign to the object.
- `length` : The amount of space to allocate to the object, can be zero

**Returns**

- On success:
  - returns `0`, and the object with the `id` has been created with the requested length.
- On Failure:
  - returns a system error code indicating failure, no object is created.

Notes:

This function will assert on:

- `oStore` is not valid.
- `id` : The ID already exists.
- The file has been opened in read only (`EReadOnly`) mode.

#### `ostrore_addObject`

```c
int ostrore_addObject(TOStoreHnd oStore, TOStoreObjID* id, uint32_t length);
```

**Function Description:** Adds an object to the store with a specified ID.

**Parameters:** 

- `oStore`: An OStore handle, for an open OStore.
- `id` : A pointer to an ID. On success this will be populated with the ID of the Object that was created.
- `length` : The amount of space to allocate to the object, can be zero.

**Returns**

- On success:
  - returns `0`, and the object with the `id` returned in the value pointed to by `id` has been created with zero length.
  - `id`: is set with the ID of the object created
- On Failure:
  - returns a system error code indicating failure, no object is created.
  - `id`: is undefined.

Notes:

This function will assert on:

- `oStore` is not valid.
- `id` : The pointer is NULL.
- The file has been opened in read only (`EReadOnly`) mode.

#### `ostore_removeObject`

```c
int ostore_removeObject(TOStoreHnd ostore, TOStoreObjID id);
```

**Function Description:** Removes an object from the store.
**Parameters:** 

- `oStore`: An OStore handle, for an open OStore.
- `id` : The ID of the object to be removed.

**Returns**

- On success:
  - returns `0`
  - The Object associated with the ID is removed.
- On Failure:
  - returns a system error code indicating failure

Notes:

This function will assert on:

- `oStore` is not valid.
- `id` : The ID does not exist.
- The file has been opened in read only (`EReadOnly`) mode.

### Object Operations

#### `ostore_setLength`

```c
int ostore_setLength(TOStoreHnd oStore, TOStoreObjID id, uint32_t length);
```

**Function Description:** Sets the length of an object within the store. This can be used to extend the objects length and force allocate more space to it, or alternatively it can be used to truncate the object and release space that has been assigned to it.
**Parameters:** 

- `oStore`: An OStore handle, for an open OStore.
- `id` : The ID of the object to be manipulated.
- `length` : The length to set in bytes.

**Returns**

- On success:
  - returns `0`
  - The length requested is set for the object with the `id` specified.
- On Failure:
  - returns a system error code indicating failure, the length of the object will remain the same.

Notes:

This function will assert on:

- `oStore` is not valid.
- `id` : The ID does not exist.

#### `ostore_getLength`

```c
int ostore_getLength(TOStoreHnd oStore, TOStoreObjID id, uint32_t* length);
```

**Function Description:** Gets the length of the object within the store. 
**Parameters:** 

- `oStore`: An OStore handle, for an open OStore.
- `id` : The ID of the object to be manipulated.
- `length` : A pointer to a `uint32_t` which will contain the length in bytes of the requested object.

**Returns**

- On success:
  - returns `0`
  - The length of the requested object is stored in the value pointed to by `length` 
- On Failure:
  - returns a system error code indicating failure
  - value of `length` is unspecified.

Notes:

This function will assert on:

- `oStore` is not valid.
- `id` : The ID does not exist.
- The file has been opened in read only (`EReadOnly`) mode.

### Reading and Writing Data

#### `ostore_read`

```c
int ostore_read(TOStoreHnd oStore, TOStoreObjID id, uint32_t position, uint32_t length,
                   void* destination);
```

**Function Description:** Reads data from the object and stores it in the `destination` buffer. Note that no range checking occurs on the destination buffer, the caller must ensure that `destination` has enough space for the number of bytes requested in `length`.  A range check does occur within the OStore. If the number of bytes requested from the point of `position` exceeds the objects length then an assert will occur. The user of the function should ensure this doesn't occur by querying the length of the object.

**Parameters:** 

- `oStore`: An OStore handle, for an open OStore.
- `id` : The ID of the object to be read.
- `position`: The offset from the start of the object from which to read, note that `0` indicates the start of the object.
- `length` : The number of bytes to read starting from the offset given by `position`.

**Returns**

- On success:
  - returns `0`
  - The requested bytes are stored in the `destination` buffer.
- On Failure:
  - returns a system error code indicating failure
  - value of the buffer pointed to by `destination` is left unchanged.

Notes:

This function will assert on:

- `oStore` is not valid.
- `id` : The ID does not exist.
- `position` + `length` exceeds the length of the current object.
- `destination` is NULL.

#### `ostoreobj_write`

```c
int ostoreobj_write(TOStoreHnd oStore, TOStoreObjID id, uint32_t position, 
                    const void* source, uint32_t length);
```

**Function Description:** Writes `length` bytes taken from the memory location `source` and writes them to the object, at the `position` specified. If the object is not large enough to store the requested bytes from `position` + `length` then the buffer will be extended to match this new length.

**Parameters:** 

- `oStore`: An OStore handle, for an open OStore.
- `id` : The ID of the object to be read.
- `position`: The offset from the start of the object from which to read, note that `0` indicates the start of the object.
- `source` : The in memory bytes to write to the store.
- `length` : The number of bytes to write starting from the offset given by `position`.

**Returns**

- On success:
  - returns `0`
  - The requested bytes pointed to by `source` are written to the object at the given position. 
- On Failure:
  - returns a system error code indicating failure
  - No bytes are stored and the object within the store remains unchanged.

Notes:

This function will assert on:

- `oStore` is not valid.
- `id` : The ID does not exist.
- `source` is NULL.
- The file has been opened in read only (`EReadOnly`) mode.