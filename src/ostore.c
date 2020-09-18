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

    retval = readFromFile(store->fp, FILE_LOCATION_FOR_TABLE_OF_OBJECTS_HEADER, sizeof(TDskOStoreObjectHeader), &store->tableOfObjectsHeader.header);
    IF_NOT_OK_HANDLE_ERROR(retval);

    retval = readFromFile(store->fp, FILE_LOCATION_FOR_TRASH_HEADER, sizeof(TDskOStoreObjectHeader), &store->tashHeader.header);
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

    retval = writeToFile(store->fp, FILE_LOCATION_FOR_TABLE_OF_OBJECTS_HEADER, sizeof(TDskOStoreObjectHeader), &store->tableOfObjectsHeader.header);
    IF_NOT_OK_HANDLE_ERROR(retval);

    retval = writeToFile(store->fp, FILE_LOCATION_FOR_TRASH_HEADER, sizeof(TDskOStoreObjectHeader), &store->tashHeader.header);
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
        LOCAL_MEMZ(TDskOStoreObjectHeader, header);
        offset = (i * sizeof(TDskOStoreObjectHeader)) + sizeof(uint32_t);        
        retval = readWithHeader(store, &store->tableOfObjectsHeader.header, offset, sizeof(TDskOStoreObjectHeader), &header);
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

    retval = ERR_NOT_FOUND;
    for(uint32_t i = 0; i < store->numberOfObjects; i++) {
        LOCAL_MEMZ(TDskOStoreObjectHeader, header);
        offset = (i * sizeof(TDskOStoreObjectHeader)) + sizeof(uint32_t);        
        retval = readWithHeader(store, &store->tableOfObjectsHeader.header, offset, sizeof(TDskOStoreObjectHeader), &header);
        IF_NOT_OK_HANDLE_ERROR(retval);
        if (header.id == id) {
            found = true;
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
    // assign space to the 
    PROCESS_ERROR;
    FINISH;
}

int ostrore_addObject(TOStoreHnd oStore, TOStoreObjID* id, uint32_t length) {
    assert(oStore != NULL);
    assert(*oStore != NULL);
    // find the next available object id
    // add the object with the ID
}

int ostore_removeObject(TOStoreHnd oStore, TOStoreObjID id) {
    assert(oStore != NULL);
    assert(*oStore != NULL);
    // find the object with the matching id to remove
    // then compact the array to remove it
    // then update the counters
    
}

// Object Operations
int ostoreobj_setLength(TOStoreHnd oStore, TOStoreObjID id, uint32_t lengthRequested) {
    START;
    TOStore* store = *oStore;
    LOCAL_MEMZ(TDskOStoreObjectHeader, head);
    retval = readObjectHeader(oStore, id, &head);
    IF_NOT_OK_HANDLE_ERROR(retval);
    retval = setLengthWithHeader(store, &head, lengthRequested);
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

    LOCAL_MEMZ(TDskOStoreObjectHeader, head);
    retval = readObjectHeader(oStore, id, &head);
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
    LOCAL_MEMZ(TDskOStoreObjectHeader, head);
    retval = readObjectHeader(oStore, id, &head);
    IF_NOT_OK_HANDLE_ERROR(retval);

    /// read data now with header.
    retval = readWithHeader(*oStore, &head, position, length, destination);

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
    LOCAL_MEMZ(TDskOStoreObjectHeader, head);
    retval = readObjectHeader(oStore, id, &head);
    IF_NOT_OK_HANDLE_ERROR(retval);

    // if there are more blocks needed, than are available, assert, this should be checked before invoking.
    uint32_t totalLength = position + length;
    uint32_t availableSpace = store->fileHeader.header.blockSize * head.numberOfBlocks;
    assert(totalLength <= availableSpace);
    retval = writeWithHeader(store, &head, position, length, source);

    PROCESS_ERROR;
    FINISH;    
}