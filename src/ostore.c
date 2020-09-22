#include "ostore.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>

#include "ostore_internal.h"


// Store Handling
int ostore_open(const char* filename, TOStreamMode mode, TOStoreHnd* oStore) {
    assert(oStore != NULL);
    assert(filename != NULL);
    assert(mode == EReadOnly || mode == EReadWrite);
    START;

    TOStore* store = zmalloc(TOStore);
    if ( store == NULL )
        HANDLE_ERROR(ERR_MEM);
    
    const char* fileMode = NULL;
    switch(mode) {
        EReadWrite:
        fileMode = "br+";
        break;

        EReadOnly:
        fileMode = "br";
        break;
    }
    store->fileMode = mode;

    store->fp = fopen(filename, fileMode);
    VALIDATE(store->fp == NULL, ERR_NOT_FOUND);

    retval = readFromFile(store->fp, 0, sizeof(TDskObjectStoreFileHeader), &store->fileHeader.header);
    IF_NOT_OK_HANDLE_ERROR(retval);
    VALIDATE( IDS_MATCH(FILE_ID, store->fileHeader.header.identifyingWord), ERR_CORRUPT);
    VALIDATE(store->fileHeader.header.versionNumner = VERSION, ERR_CORRUPT);

    TDskObjectStoreBlockHeader firstBlockHeader;
    memset(&firstBlockHeader, 0, sizeof(TDskObjectStoreBlockHeader));
    retval = readFromFile(store->fp, FILE_LOCATION_FOR_FIRST_BLOCK, sizeof(TDskObjectStoreBlockHeader), &firstBlockHeader);
    IF_NOT_OK_HANDLE_ERROR(retval);
    VALIDATE( IDS_MATCH(BLOCK_ID, firstBlockHeader.identifyingWord), ERR_CORRUPT);
    VALIDATE( firstBlockHeader.id = OBJECT_TABLE_ID, ERR_CORRUPT);
    VALIDATE( firstBlockHeader.last == 0, ERR_CORRUPT);
    VALIDATE( firstBlockHeader.sequenceNumber == 0, ERR_CORRUPT);

    // boot strapping here
    retval = readFromFile(store->fp, FILE_LOCATION_FOR_NUMBER_OF_OBJECTS, sizeof(uint32_t), &store->numberOfObjects);
    IF_NOT_OK_HANDLE_ERROR(retval);

    retval = readFromFile(store->fp, FILE_LOCATION_FOR_TABLE_OF_OBJECTS_INDEX, sizeof(TDskObjIndex), &store->tableOfObjectsHeader.header);
    IF_NOT_OK_HANDLE_ERROR(retval);

    retval = readFromFile(store->fp, FILE_LOCATION_FOR_TRASH_INDEX, sizeof(TDskObjIndex), &store->tashHeader.header);
    IF_NOT_OK_HANDLE_ERROR(retval);
    

    (*oStore) = store;

    PROCESS_ERROR;
    if ( store && store->fp) {
        fclose(store->fp);        
    }
    zfree(store);

   FINISH;
}

int ostore_create(const char* filename, TOStoreHnd* oStore) {
    assert(oStore != NULL);
    assert(filename != NULL);
    START;


    TOStore* store = zmalloc(TOStore);
    if ( store == NULL )
        HANDLE_ERROR(ERR_MEM);
    
    store->fileHeader.header.blocksInFile = 1;
    store->fileHeader.header.blockSize = DEFUALT_BLOCKSIZE;
    SET_ID(store->fileHeader.header.identifyingWord, FILE_ID);
    store->fileHeader.header.versionNumner = VERSION;
    store->numberOfObjects = INITIAL_NUMBER_OF_OBJECTS;
    
    store->tableOfObjectsHeader.header.headBlock = 0;
    store->tableOfObjectsHeader.header.tailBlock = 0;
    store->tableOfObjectsHeader.header.id = OBJECT_TABLE_ID;
    store->tableOfObjectsHeader.header.numberOfBlocks = 1;

    store->tashHeader.header.headBlock = NO_BLOCK;
    store->tashHeader.header.id = TRASH_TABLE_ID;
    store->tashHeader.header.numberOfBlocks = 0;
    store->tashHeader.header.tailBlock = NO_BLOCK;


    TDskObjectStoreBlockHeader firstBlockHeader;
    memset(&firstBlockHeader, 0, sizeof(TDskObjectStoreBlockHeader));
    firstBlockHeader.blockFileIndex = 0;
    firstBlockHeader.id = OBJECT_TABLE_ID;
    SET_ID(firstBlockHeader.identifyingWord, BLOCK_ID);
    firstBlockHeader.last = NO_BLOCK;
    firstBlockHeader.next = NO_BLOCK;
    firstBlockHeader.sequenceNumber = 0;


    store->fp = fopen(filename, "bw+");
    VALIDATE(store->fp == NULL, ERR_CORRUPT);
    retval = writeToFile(store->fp, 0, sizeof(TDskObjectStoreFileHeader), &store->fileHeader.header);
    IF_NOT_OK_HANDLE_ERROR(retval);
    
    retval = addBlockToFile(store->fp, &firstBlockHeader, DEFUALT_BLOCKSIZE);
    IF_NOT_OK_HANDLE_ERROR(retval);
    
    retval = writeToFile(store->fp, FILE_LOCATION_FOR_NUMBER_OF_OBJECTS, sizeof(uint32_t),  &store->numberOfObjects);

    retval = writeToFile(store->fp, FILE_LOCATION_FOR_TABLE_OF_OBJECTS_INDEX, sizeof(TDskObjIndex), &store->tableOfObjectsHeader.header);
    IF_NOT_OK_HANDLE_ERROR(retval);

    retval = writeToFile(store->fp, FILE_LOCATION_FOR_TRASH_INDEX, sizeof(TDskObjIndex), &store->tashHeader.header);
    IF_NOT_OK_HANDLE_ERROR(retval);

    store->fileMode = EReadWrite;

    PROCESS_ERROR;
    if ( store && store->fp) {
        fclose(store->fp);        
    }
    zfree(store);   
   FINISH;
}

void ostore_close(TOStoreHnd* oStore) {
    assert(oStore != NULL);
    if (*oStore) {
        fclose((*oStore)->fp);
    }
    zfree( *oStore );
    *oStore = NULL;
}

// Object Inspection
int ostore_enumerateObjects(TOStoreHnd oStore, uint32_t* numberOfObjects) {
    assert(oStore != NULL);
    assert(numberOfObjects != NULL);
    TOStore* store = (*oStore);
    assert(store != NULL);
    (*numberOfObjects) = store->numberOfObjects;
    return 0;
}

int ostore_getObjectIdFromIndex(TOStoreHnd oStore, uint32_t objectIndex, TOStoreObjID* id) {
    assert(oStore != NULL);
    assert(*oStore != NULL);
    assert(id != NULL);    

    START;
    TOStore* store = (*oStore);
    assert(objectIndex < store->numberOfObjects);
    uint32_t offset = sizeof(uint32_t);
    
    retval = ERR_NOT_FOUND;
    for(uint32_t i = 0; i < store->numberOfObjects; i++) {
        LOCAL_MEMZ(TDskObjIndex, header);
        offset = (i * sizeof(TDskObjIndex)) + sizeof(uint32_t);        
        retval = readWithIndex(store, &store->tableOfObjectsHeader.header, offset, sizeof(TDskObjIndex), &header);
        IF_NOT_OK_HANDLE_ERROR(retval);
        if (i == objectIndex) {
            (*id) = header.id;
            break;
        }
    }

    PROCESS_ERROR;

    FINISH;    
}

int ostore_objectIdExists(TOStoreHnd oStore, TOStoreObjID id) {
    assert(oStore != NULL);
    assert(*oStore != NULL);

    START;
    TOStore* store = (*oStore);
    uint32_t offset = sizeof(uint32_t);
    bool found = false;

    for(uint32_t i = 0; i < store->numberOfObjects; i++) {
        LOCAL_MEMZ(TDskObjIndex, header);
        offset = (i * sizeof(TDskObjIndex)) + sizeof(uint32_t);        
        retval = readWithIndex(store, &store->tableOfObjectsHeader.header, offset, sizeof(TDskObjIndex), &header);
        IF_NOT_OK_HANDLE_ERROR(retval);
        if (header.id == id) {
            found = true;
            retval = ERR_OK;
            break;
        }
    }

    if ( !found ) {
        retval = ERR_NOT_FOUND;
    }

    PROCESS_ERROR;

    FINISH;    
}

// Object Management
int ostrore_addObjectWithId(TOStoreHnd oStore, TOStoreObjID id, uint32_t length) {
    assert(oStore != NULL);
    assert(*oStore != NULL);
    TOStore* store = (*oStore);
    // check to make sure an object with the same id doesn't exist
    START;
    retval = ostore_objectIdExists(oStore, id);
    if ( retval != ERR_NOT_FOUND) {
        retval = ERR_ALREADY_EXISTS;        
    } else {
        retval = ERR_OK;
    }
    IF_NOT_OK_HANDLE_ERROR(retval);
    
    // add the object header entry
    LOCAL_MEMZ(TDskObjIndex, index);
    index.id = id;
    index.numberOfBlocks = 0;
    index.headBlock = NO_BLOCK;
    index.tailBlock = NO_BLOCK;

    retval = writeObjectIndex(oStore, id, &index);
    IF_NOT_OK_HANDLE_ERROR(retval);

    // assign space to the 
    uint32_t blocksToAdd = length / store->fileHeader.header.blockSize;
    if (blocksToAdd == 0 ) blocksToAdd = 1;

    retval = growLengthWithIndex(*oStore, &index, blocksToAdd);
    IF_NOT_OK_HANDLE_ERROR(retval);

    PROCESS_ERROR;
    FINISH;
}



int ostore_removeObject(TOStoreHnd oStore, TOStoreObjID id) {
    assert(oStore != NULL);
    assert(*oStore != NULL);
    // find the object with the matching id to remove
    // then compact the array to remove it
    // then update the counters

    START;
    TOStore* store = (*oStore);     
    bool found = false;
    bool startingWrite = false;
    uint32_t index = 0;
    uint32_t i = -1;
    uint32_t reducedObjectCount = store->numberOfObjects - 1;
    uint32_t originalObjectCount = store->numberOfObjects;
    LOCAL_MEMZ(TDskObjIndex, headerToDelete);

    for(i = 0; i < store->numberOfObjects; i++) {        
        uint32_t offset = (i * sizeof(TDskObjIndex)) + sizeof(uint32_t);
        memset(&headerToDelete, 0, sizeof(TDskObjIndex));
        retval = readWithIndex(store, &store->tableOfObjectsHeader.header, offset, sizeof(TDskObjIndex), &headerToDelete);
        IF_NOT_OK_HANDLE_ERROR(retval);
        if (headerToDelete.id == id) {
            found = true;
            index = i;
            break;
        }
    }

    if (!found) {
        IF_NOT_OK_HANDLE_ERROR(ERR_NOT_FOUND);
    }

    startingWrite = true;
    for(i = index; i < reducedObjectCount; i++) {
        uint32_t offsetOld = (i * sizeof(TDskObjIndex)) + sizeof(uint32_t);
        uint32_t offsetNew = ( (i+1) * sizeof(TDskObjIndex)) + sizeof(uint32_t);
        LOCAL_MEMZ(TDskObjIndex, header);                
        retval = readWithIndex(store, &store->tableOfObjectsHeader.header, offsetNew, sizeof(TDskObjIndex), &header);
        IF_NOT_OK_HANDLE_ERROR(retval);
        retval = writeWithIndex(store, &store->tableOfObjectsHeader.header, offsetOld, sizeof(TDskObjIndex), &header);        
        IF_NOT_OK_HANDLE_ERROR(retval);
    }

    // update the counters
    store->numberOfObjects--;
    // save to disk

    PROCESS_ERROR;
    // ugh... ok, in this situation we need to restore the missing data
    // easiest way to do this is to have a copy of the modified data and
    // restore it.
    // this would be:
    //  - the entry being deleted (as it is first to be overwritten)
    //    : headerToDelete
    //  - the clobber loop simply produces duplicates of entries on failure
    //    if we find a 
    // search for
    if (startingWrite) {
        
    }
    FINISH;     
    
}

// Object Operations
int ostoreobj_setLength(TOStoreHnd oStore, TOStoreObjID id, uint32_t lengthRequested) {
    START;
    TOStore* store = *oStore;
    LOCAL_MEMZ(TDskObjIndex, head);
    retval = readObjectIndex(oStore, id, &head);
    IF_NOT_OK_HANDLE_ERROR(retval);
    retval = setLengthWithIndex(store, &head, lengthRequested);
    PROCESS_ERROR;
    FINISH;
}

int ostoreobj_getLength(TOStoreHnd oStore, TOStoreObjID id, uint32_t* length) {
    assert(oStore);
    assert(id != OBJECT_TABLE_ID || id != TRASH_TABLE_ID);
    assert(length);
    (*length) = 0;
    START;
    TOStore* store  = oStore;

    LOCAL_MEMZ(TDskObjIndex, head);
    retval = readObjectIndex(oStore, id, &head);
    IF_NOT_OK_HANDLE_ERROR(retval);
    (*length) = head.numberOfBlocks * store->fileHeader.header.blockSize;

    PROCESS_ERROR;

    FINISH;

}

// Reading and Writing Data
int ostoreobj_read(TOStoreHnd oStore, TOStoreObjID id, uint32_t position, uint32_t length, void* destination) {
    assert(oStore);
    assert(id != OBJECT_TABLE_ID || id != TRASH_TABLE_ID);
    assert(destination);

    START;
    TOStore* store = (*oStore);
    assert(store);
    LOCAL_MEMZ(TDskObjIndex, head);
    retval = readObjectIndex(oStore, id, &head);
    IF_NOT_OK_HANDLE_ERROR(retval);

    /// read data now with header.
    retval = readWithIndex(*oStore, &head, position, length, destination);

    PROCESS_ERROR;
    FINISH;
}

int ostoreobj_write(TOStoreHnd oStore, TOStoreObjID id, uint32_t position, const void* source, uint32_t length) {
    assert(oStore);
    assert(id != OBJECT_TABLE_ID || id != TRASH_TABLE_ID);
    assert(source);

    START;
    TOStore* store = (*oStore);
    assert(store);
    LOCAL_MEMZ(TDskObjIndex, head);
    retval = readObjectIndex(oStore, id, &head);
    IF_NOT_OK_HANDLE_ERROR(retval);

    // if there are more blocks needed, than are available, assert, this should be checked before invoking.
    uint32_t totalLength = position + length;
    uint32_t availableSpace = store->fileHeader.header.blockSize * head.numberOfBlocks;
    assert(totalLength <= availableSpace);
    retval = writeWithIndex(store, &head, position, length, source);

    PROCESS_ERROR;
    FINISH;    
}