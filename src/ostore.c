#include "ostore.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>

// handy macro's
// magic number reference


#define IS_ERROR(A) ( (A) < 0 )
#define IS_NOT_ERROR(A) ( (A) >= 0)

// Offset calculations for boostraping
#define FILE_LOCATION_FOR_FIRST_BLOCK (sizeof(TDskObjectStoreFileHeader))

#define FILE_LOCATION_FOR_NUMBER_OF_OBJECTS (\
     sizeof(TDskObjectStoreFileHeader) \
     + sizeof(TDskObjectStoreBlockHeader) )

#define FILE_LOCATION_FOR_TABLE_OF_OBJECTS_HEADER (\
     FILE_LOCATION_FOR_NUMBER_OF_OBJECTS \
     + sizeof(uint32_t) )

#define FILE_LOCATION_FOR_TRASH_HEADER (\
     FILE_LOCATION_FOR_TABLE_OF_OBJECTS_HEADER \
     + sizeof(uint32_t) )



// structure definitions
static const int8_t FILE_ID[4] = { 'O', 'S', 'T', 'R'};
static const int8_t BLOCK_ID[4] = { 'B', 'L', 'C', 'K' };
#define IDS_MATCH( A, B ) ( A[0] == B[0] && A[1] == B[1] && A[2] == B[2] && A[3] == B[3])
#define SET_ID( A, B ) {A[0] = B[0]; A[1] = B[1]; A[2] = B[2]; A[3] = B[3];}

static const uint32_t OBJECT_TABLE_ID = 0;
static const uint32_t TRASH_TABLE_ID = 1;
static const uint32_t DEFUALT_BLOCKSIZE = 128;
static const uint32_t VERSION = 1;
static const uint32_t INITIAL_NUMBER_OF_OBJECTS = 2;
static const uint32_t NO_BLOCK = 0xFFFFFFFF;

typedef struct
{
    int8_t          identifyingWord[4];
    uint32_t        versionNumner;
    uint32_t        blockSize;
    uint32_t        blocksInFile;
} TDskObjectStoreFileHeader;

typedef struct 
{
    TDskObjectStoreFileHeader   header;
    bool                        dityflag;
} TObjectStoreFileHeader;


typedef struct 
{
    int8_t          identifyingWord[4];
    uint32_t        blockFileIndex;
    uint32_t        id;
    uint32_t        sequenceNumber;
    uint32_t        next;
    uint32_t        last;
} TDskObjectStoreBlockHeader;


typedef struct 
{
    TOStoreObjID        id;
    uint32_t            headBlock;
    uint32_t            tailBlock;
    uint32_t            numberOfBlocks;
} TDskOStoreObjectHeader;

typedef struct
{
    TDskOStoreObjectHeader  header;
    bool                    dirtyFlag;
} TOStoreObjectHeader;



typedef struct
{
    FILE*                   fp;
    TOStreamMode            fileMode;
    TObjectStoreFileHeader  fileHeader;
    TOStoreObjectHeader     tableOfObjectsHeader;
    TOStoreObjectHeader     tashHeader;

    // working memory
    uint32_t                numberOfObjects;    
} TOStore;

// internal function definitions
static void* zeromalloc(size_t size);
static void zfree( void* ptr );
static int readFromFile(FILE* fp, uint32_t offset, uint32_t length, uint8_t* buffer);
static int writeToFile(FILE* fp, uint32_t offset, uint32_t length, const uint8_t* buffer);

// local handy macros
#define zmalloc( A ) zeromalloc(sizeof(A))
#define START int retval = 0
#define VALIDATE( A, B) if ( !(A) ) HANDLE_ERROR(B)
#define IF_NOT_OK_HANDLE_ERROR( A ) if ( (retval = (A)) != ERR_OK ) HANDLE_ERROR( retval )
#define IF_NOT_OK_HANDLE_ERROR if ( retval != ERR_OK ) HANDLE_ERROR( retval )
#define HANDLE_ERROR(A) { retval = A; goto HandleError; };
#define PROCESS_ERROR return retval; HandleError:
#define FINISH return retval;

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


    /*
    open the file
    if there is an error, stop and return error

    read the file header
    ensure the file header is ok, if not stop and return error.

    read the first block
    validate the first block to ensure it is ok, if not stop and return error.
    
    bootstrap
    load the headers for object table
    validate, if error stop and return error
    load the header for trash

    return ok / error.
    */

   
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
    firstBlockHeader.last = 0;
    firstBlockHeader.next = 0;
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

    /*
    create the file
    if there is an error, stop and return error

    write default file header.
    write default 1st block.
    write default object table header
    write default trash table header

    return ok / error.
    */
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
        
/*
    flush everything to the disk
    close the file
    clear the ostore object
*/
}

// Object Inspection
int ostore_enumerateObjects(TOStoreHnd oStore, uint32_t* numerOfObjects) {
    assert(oStore != NULL);
    assert(numerOfObjects != NULL);
}

int ostore_getObjectIdFromIndex(TOStoreHnd oStore, uint32_t objectIndex, TOStoreObjID* id) {
    assert(oStore != NULL);
    assert(id != NULL);
}

int ostore_objectIdExists(TOStoreHnd oStore, TOStoreObjID id) {
    assert(oStore != NULL);
}

// Object Management
int ostrore_addObjectWithId(TOStoreHnd oStore, TOStoreObjID id) {
    assert(oStore != NULL);
}

int ostrore_addObject(TOStoreHnd oStore, TOStoreObjID* id) {
    assert(oStore != NULL);
}

int ostore_removeObject(TOStoreHnd oStore, TOStoreObjID id) {
    assert(oStore != NULL);
}

// Object Operations
int ostoreobj_setLength(TOStoreHnd oStore, TOStoreObjID id, uint32_t lengthRequested) {
    /*
    read object header
    if length is within the last block of space, do nothing
    if length is less than the last block:
        calculate new block required, calculate delta
        walk backwards from end of block list
            until we find the last block needed
        set last block needed to the tail of the empty block list
        tidy up last block with -1 blocks for next.
        update object header
        write new object header
        return space allocated
    if length is greater than the last block:
        calculate new blocks required
        take as many blocks as possible from t
    */
}

int ostoreobj_getLength(TOStoreHnd oStore, TOStoreObjID id, uint32_t* length) {

}

// Reading and Writing Data
int ostoreobj_read(TOStoreHnd oStore, TOStoreObjID id, uint32_t position, uint32_t length, void* destination) {

}

int ostoreobj_write(TOStoreHnd oStore, TOStoreObjID id, uint32_t position, const void* source, uint32_t length) {

}


/******************************INTERNAL FUNCTIONS HERE********************************************/

int readObjectHeader(const TOStoreHnd oStore, TOStoreObjID id, TDskObjectStoreBlockHeader* header){
    assert(oStore);
    assert(header);
    START;
    const TOStore* store = (*oStore);
    uint32_t offset = sizeof(uint32_t);
    
    retval = ERR_NOT_FOUND;
    for(uint32_t i = 0; i < store->numberOfObjects; i++) {
        memset(header, 0, sizeof(TDskObjectStoreBlockHeader));
        retval = readWithHeader(store, store->tableOfObjectsHeader.header, offset, sizeof(TDskObjectStoreBlockHeader), header);
        IF_NOT_OK_HANDLE_ERROR(retval);
        if ( header->id == id)
            break;
    }

    PROCESS_ERROR;

/*
    clear the header
    then read the number of entiries in the object header object
    for each object header
        calculate offset
        read header item into header ptr
        readWithHeader()
        if header ptr now matches the id of the header we are after then stop

*/
    FINISH;
}


// Reading and Writing Data
int readWithHeader(TOStore* store, const TDskObjectStoreBlockHeader* header, uint32_t position, uint32_t length, void* destination) {
    /*
    if ( read goes over the length of the block) then
        return an error code
    else
        read the data into the supplied buffer
        
        calculate block to start reading in based on the offset
        while (there is still data to read)
            read data from that block into buffer, update offset
            move to next block

    */
   
}



int readFromFile(FILE* fp, uint32_t offset, uint32_t length, uint8_t* buffer) {
    int retval = fseek(fp, offset, SEEK_SET);
    if (retval == 0) {
        memset(buffer, 0, length);
        retval = fread(buffer, length, 1, fp);
        if ( retval != 1 ) {
            retval = ERR_OVERFLOW;
        }
    } else {
        retval = ERR_OVERFLOW;
    }

    return retval;
}

int writeToFile(FILE* fp, uint32_t offset, uint32_t length, const uint8_t* buffer) {
    int retval = fseek(fp, offset, SEEK_SET);
    if (retval == 0) {
        
        retval = fwrite(buffer, length, 1, fp);
        if ( retval != 1 ) {
            retval = ERR_OVERFLOW;            
        } else {
            retval = fflush(fp); // check error response
        }
        
    } else {
        retval = ERR_OVERFLOW;
    }

    return retval;    
}

int addBlockToFile(FILE* fp, const TDskObjectStoreBlockHeader* header, uint32_t size) {
    int retval = fseek(fp, 0, SEEK_END);
    if (retval == 0 ) {
        retval = fwrite(header, sizeof(TDskObjectStoreBlockHeader),1, fp);
        if ( retval != 1) {
            retval = ERR_OVERFLOW;
        } else {
            uint8_t val = 0;
            retval = fwrite(&val, sizeof(uint8_t), size, fp);
            if ( retval != size ) {
                retval = ERR_OVERFLOW;
            } else {
                retval = fflush(fp);
            }            
        }
    }
}


void* zeromalloc(size_t size) {
    void* retval = malloc(size);
    if ( retval != NULL ) {
        memset(retval, 0, size);
    }
    return retval;
}

void zfree( void* ptr ) {
    if ( ptr != NULL ) {
        free(ptr);
    }
}
