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

#define OFFSET_FOR_BLOCK(INDEX, SIZE) (sizeof(TDskObjectStoreFileHeader) + (INDEX * (sizeof(TDskObjectStoreBlockHeader) + SIZE)))

#define CONVERT_TO_FILE_OFFSET( INDEX, SIZE, OFFSET) (OFFSET_FOR_BLOCK(INDEX, SIZE) + OFFSET)


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
#define INIT(TYPE, VAR) memset(&VAR, 0, sizeof(TYPE))
#define INIT_HEAP(TYPE, VAR) memset(VAR, 0, sizeof(TYPE))
#define LOCAL_MEMZ(TYPE, VAR) TYPE VAR; memset(&VAR, 0, sizeof(TYPE))
#define HEAP_MEMZ(TYPE, VAR) TYPE* VAR = malloc(sizeof(TYPE)); VALIDATE(VAR != NULL, ERR_MEM); memset(VAR, 0, sizeof(TYPE))

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
    START;
    TOStore* store = *oStore;
    LOCAL_MEMZ(TDskOStoreObjectHeader, head);
    retval = readObjectHeader(oStore, id, &head);
    IF_NOT_OK_HANDLE_ERROR(retval);
    PROCESS_ERROR;
    FINISH;
}


static int setLengthWithHeader(TOStore* store, TDskOStoreObjectHeader* head, uint32_t lengthRequested) {

    START;
    uint32_t numberOfBlockRequested = lengthRequested / store->fileHeader.header.blockSize;

    /*
    Calculate the number of blocks needed
    If this is greater than the blocks currently allocated, then start adding blocks
        1. take as any of the trash blocks as we need.
        2. add new blocks, as many as are needed.
    
    If this is less than the blocks currentl allocated, then remove the blocks and add them to trash
        - find the 
    */
    

    if (numberOfBlockRequested > head->numberOfBlocks) {
        // add blocks
    } else if (numberOfBlockRequested < head->numberOfBlocks) {
        // remove blocks
        uint32_t blocksToRemove = head->numberOfBlocks - numberOfBlockRequested;
        uint32_t counter = blocksToRemove;




    }

    PROCESS_ERROR;

    FINISH;
}

/*
    This function removes blocks from an object. It does not add them to another block (the free chain)
    it just leaves them dangling. 
*/
static int removeBlocks(TOStore* store, TDskOStoreObjectHeader* head, TDskObjectStoreBlockHeader* freeBlocks, uint32_t numberOfBlocksToRemove) {
    START;
    LOCAL_MEMZ(TDskObjectStoreBlockHeader, currentBlock);
    retval = readBlockHeader(store, &currentBlock, head->tailBlock);
    uint32_t counter = numberOfBlocksToRemove;
    while(counter != 0) {            
        retval = readBlockHeader(store, &currentBlock, currentBlock.last);
        IF_NOT_OK_HANDLE_ERROR(retval);
        counter--;
    }

    retval = readBlockHeader(store, freeBlocks, currentBlock.last);
    IF_NOT_OK_HANDLE_ERROR(retval);
    
    // update blocks and write them back to disk, then update header
    head->tailBlock = currentBlock.blockFileIndex;
    freeBlocks->last = NO_BLOCK;
    currentBlock.next = NO_BLOCK;
    
    retval = writeBlockHeader(store, freeBlocks);
    IF_NOT_OK_HANDLE_ERROR(retval);

    retval = writeBlockHeader(store, &currentBlock);
    IF_NOT_OK_HANDLE_ERROR(retval);

    // write the update to the header (head)
    retval = writeObjectHeader(&store, store->tableOfObjectsHeader.header.id, head);
    IF_NOT_OK_HANDLE_ERROR(retval);
    

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
    LOCAL_MEMZ(TDskOStoreObjectHeader, head);
    retval = readObjectHeader(oStore, id, &head);
    IF_NOT_OK_HANDLE_ERROR(retval);

    /// read data now with header.
    retval = readWithHeader(*oStore, &head, position, length, destination);

    PROCESS_ERROR;
    FINISH;
}

int ostoreobj_write(TOStoreHnd oStore, TOStoreObjID id, uint32_t position, const void* source, uint32_t length) {

}



static int readWithHeader(TOStore* store, const TDskOStoreObjectHeader* header, uint32_t position, uint32_t length, void* destination);

/******************************INTERNAL FUNCTIONS HERE********************************************/

int writeWithHeader(TOStore* store, const TDskOStoreObjectHeader* header, uint32_t position, uint32_t length, void* source) {
    /*
    determine the number of blocks that are needed to be added
    
    if more blocks are needed, then
        get as many of the re
    */
}

int writeObjectHeader(TOStoreHnd oStore, TOStoreObjID id, TDskOStoreObjectHeader* header) {
    assert(oStore);
    assert(header);
    START;

    TOStore* store = (*oStore);
    // try to find the object header, and get the offset location for the write

    LOCAL_MEMZ(TDskOStoreObjectHeader, tempObjHeader);
    bool found = false;
    uint32_t offset = sizeof(uint32_t);
    for(uint32_t i = 0; i < store->numberOfObjects; i++) {
        offset = (i * sizeof(TDskOStoreObjectHeader)) + sizeof(uint32_t);
        memset(&tempObjHeader, 0, sizeof(TDskOStoreObjectHeader));
        retval = readWithHeader(store, &store->tableOfObjectsHeader.header, offset, sizeof(TDskOStoreObjectHeader), &tempObjHeader);
        IF_NOT_OK_HANDLE_ERROR(retval);
        if ( header->id == id) {
            found = true;
            break;
        }
    }     
    // if object header can not be found add it
    if(!found) {
        // check to see if there is space for the new header
        // if no, add the space
        uint32_t numberOfObjects = store->numberOfObjects + 1;
        uint32_t totalSpaceNeeded = sizeof(uint32_t) + ( numberOfObjects * sizeof(TDskOStoreObjectHeader));
        if ( totalSpaceNeeded > (store->tableOfObjectsHeader.header.numberOfBlocks * store->fileHeader.header.blockSize) ) {
            retval = ostoreobj_setLength(oStore, store->tableOfObjectsHeader.header.id, totalSpaceNeeded);
            IF_NOT_OK_HANDLE_ERROR(retval);
        }

        // increment the number of objects and update the file to reflect it
        retval = writeWithHeader(store, &store->tableOfObjectsHeader.header, 0, sizeof(uint32_t), &numberOfObjects);
        IF_NOT_OK_HANDLE_ERROR(retval);
    }

    // 
    // overwrite if found

    PROCESS_ERROR;
    FINISH;
}

int readObjectHeader(TOStoreHnd oStore, TOStoreObjID id, TDskOStoreObjectHeader* header){
    assert(oStore);
    assert(header);
    START;
    TOStore* store = (*oStore);
    uint32_t offset = sizeof(uint32_t);
    
    retval = ERR_NOT_FOUND;
    for(uint32_t i = 0; i < store->numberOfObjects; i++) {
        offset = (i * sizeof(TDskOStoreObjectHeader)) + sizeof(uint32_t);
        memset(header, 0, sizeof(TDskOStoreObjectHeader));
        retval = readWithHeader(store, &store->tableOfObjectsHeader.header, offset, sizeof(TDskOStoreObjectHeader), header);
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
int readWithHeader(TOStore* store, const TDskOStoreObjectHeader* header, uint32_t position, uint32_t length, void* destination) {
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
   START;

   uint32_t objectSizeInBytes = store->fileHeader.header.blockSize * header->numberOfBlocks;
   uint32_t endOfRead = position + length;
   uint32_t sequenceBlock = position / store->fileHeader.header.blockSize;
   

   VALIDATE( endOfRead > objectSizeInBytes, ERR_OVERFLOW );
   LOCAL_MEMZ(TDskObjectStoreBlockHeader, currentBlockHdr);
   
   // this iterates forward, but could be smart and work backward if that is less steps (TODO)
   uint32_t index = header->headBlock;
   while(sequenceBlock > 0 && index != NO_BLOCK) {
       INIT(TDskObjectStoreBlockHeader, currentBlockHdr);
       retval = readBlockHeader(store, &currentBlockHdr, index);
       sequenceBlock--;
       if ( sequenceBlock > 0 ) {
           index = currentBlockHdr.next;
           VALIDATE(index != NO_BLOCK, ERR_CORRUPT);
       }
   }

    // we have found the right block, now we need to read until we've consumed all data
    memset(destination, 0, length);
    uint32_t consumedData = 0;
    uint32_t offsetInBlock = position - (sequenceBlock * store->fileHeader.header.blockSize);
    uint8_t* dest = (uint8_t*) destination;
    uint32_t lengthRemaining = length;

    while (consumedData != length ) {
        // determine data we can read from this block here
        uint32_t dataToReadFromThisBlock = lengthRemaining;
        uint32_t dataAvailableInThisBlock = store->fileHeader.header.blockSize - offsetInBlock;

        if ( dataToReadFromThisBlock > dataAvailableInThisBlock)
            dataToReadFromThisBlock = dataAvailableInThisBlock;

        // convert to actual file position
        uint32_t positionInFile = CONVERT_TO_FILE_OFFSET(currentBlockHdr.blockFileIndex, store->fileHeader.header.versionNumner, offsetInBlock);
        // read the data into the destination at the correct location
        retval = readFromFile(store->fp, positionInFile, dataToReadFromThisBlock, &dest[consumedData]);
        IF_NOT_OK_HANDLE_ERROR(retval);

        // update our records on what we've read so far
        consumedData = dataToReadFromThisBlock;
        lengthRemaining -= consumedData;

        // in prep for reading more data, read in the next block header in the stream
        if ( currentBlockHdr.next != NO_BLOCK ) {
            uint32_t next = currentBlockHdr.next;
            INIT(TDskObjectStoreBlockHeader, currentBlockHdr);
            retval = readBlockHeader(store, &currentBlockHdr, next);
        }

       
    }


   PROCESS_ERROR;
   FINISH;
}



int readBlockHeader(TOStore* store, TDskObjectStoreBlockHeader* header, uint32_t index) {    
    START;
    uint32_t offset = OFFSET_FOR_BLOCK(index, store->fileHeader.header.blockSize);
    retval = readFromFile(store->fp, offset, sizeof(TDskObjectStoreBlockHeader), header);
    IF_NOT_OK_HANDLE_ERROR(retval);
    VALIDATE(header->blockFileIndex == index, ERR_CORRUPT);

    PROCESS_ERROR;
    FINISH;
}


int writeBlockHeader(TOStore* store, const TDskObjectStoreBlockHeader* header) {    
    header->blockFileIndex;
    uint32_t offset = OFFSET_FOR_BLOCK(header->blockFileIndex, store->fileHeader.header.blockSize);
    int retval = writeToFile(store->fp, offset, sizeof(TDskObjectStoreBlockHeader), &header);
    return retval;
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
