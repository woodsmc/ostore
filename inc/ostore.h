#ifndef OSTORE_H__
#define OSTORE_H__

/******************************************************************************
* OStore - a simple resliant binary object storage format                     *
*                                                                             *
* Copyright Notice and License                                                *
*  (c) Copyright 2020 Chris Woods.                                            *
*                                                                             *
*  Licensed under the Apache License, Version 2.0 (the "License"); you may    *
*  not use this file except in compliance with the License. You may obtain a  *
*  copy of the License at :  [http://www.apache.org/licenses/LICENSE-2.0]     *
*                                                                             *
*  Unless required by applicable law or agreed to in writing, software        *
*  distributed under the License is distributed on an "AS IS" BASIS,          *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
*  See the License for the specific language governing permissions and        *
*  limitations under the License.                                             *
*                                                                             *
******************************************************************************/



#include <stdint.h>

#define ERR_OK 0                // no error
#define ERR_NOT_FOUND -1        // not found
#define ERR_GEN -2              // general unknown error
#define ERR_OVERFLOW -3         // overflow
#define ERR_MEM -4              // malloc fails
#define ERR_CORRUPT -5          // data is corrupted

typedef enum {
    EReadOnly,
    EReadWrite
} TOStreamMode;

typedef struct TOStore;
typedef TOStore* TOStoreHnd;
typedef struct TOStoreObj;
typedef uint32_t TOStoreObjID;

// Store Handling
int ostore_open(const char* filename, TOStreamMode mode, TOStoreHnd* oStore);
int ostore_create(const char* filename, TOStoreHnd* oStore);
void ostore_close(TOStoreHnd* oStore);

// Object Inspection
int ostore_enumerateObjects(TOStoreHnd oStore, uint32_t* numberOfObjects);
int ostore_getObjectIdFromIndex(TOStoreHnd oStore, uint32_t objectIndex, TOStoreObjID* id);
int ostore_objectIdExists(TOStoreHnd oStore, TOStoreObjID id);

// Object Management
int ostrore_addObjectWithId(TOStoreHnd oStore, TOStoreObjID id);
int ostrore_addObject(TOStoreHnd oStore, TOStoreObjID* id);
int ostore_removeObject(TOStoreHnd oStore, TOStoreObjID id);

// Object Operations
int ostoreobj_setLength(TOStoreHnd oStore, TOStoreObjID id, uint32_t length);
int ostoreobj_getLength(TOStoreHnd oStore, TOStoreObjID id, uint32_t* length);

// Reading and Writing Data
int ostoreobj_read(TOStoreHnd oStore, TOStoreObjID id, uint32_t position, uint32_t length, void* destination);
int ostoreobj_write(TOStoreHnd oStore, TOStoreObjID id, uint32_t position, const void* source, uint32_t length);


#endif // OSTORE_H__