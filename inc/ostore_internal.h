#ifndef OSTOREINTERNAL_H__
#define OSTOREINTERNAL_H__

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include "ostore.h"


// local handy macros
#define INIT(TYPE, VAR) memset(&VAR, 0, sizeof(TYPE))
#define INIT_HEAP(TYPE, VAR) memset(VAR, 0, sizeof(TYPE))
#define LOCAL_MEMZ(TYPE, VAR) TYPE VAR; memset(&VAR, 0, sizeof(TYPE))
#define HEAP_MEMZ(TYPE, VAR) TYPE* VAR = malloc(sizeof(TYPE)); VALIDATE(VAR != NULL, ERR_MEM); memset(VAR, 0, sizeof(TYPE))
#define zmalloc( A ) zeromalloc(sizeof(A))
#define START int retval = 0
#define VALIDATE( A, B) if ( (A) ) HANDLE_ERROR(B)
#define IF_NOT_OK_HANDLE_ERROR( A ) if ( (retval = (A)) != ERR_OK ) HANDLE_ERROR( retval )
//#define IF_NOT_OK_HANDLE_ERROR if ( retval != ERR_OK ) HANDLE_ERROR( retval )
#define HANDLE_ERROR(A) { retval = A; goto HandleError; };
#define PROCESS_ERROR return retval; HandleError:
#define FINISH return retval;

// File Structures
#define IS_ERROR(A) ( (A) < 0 )
#define IS_NOT_ERROR(A) ( (A) >= 0)

// Offset calculations for boostraping
#define FILE_LOCATION_FOR_FIRST_BLOCK (sizeof(TDskObjectStoreFileHeader))

#define FILE_LOCATION_FOR_NUMBER_OF_OBJECTS (\
     sizeof(TDskObjectStoreFileHeader) \
     + sizeof(TDskObjectStoreBlockHeader) )

#define FILE_LOCATION_FOR_TABLE_OF_OBJECTS_INDEX (\
     FILE_LOCATION_FOR_NUMBER_OF_OBJECTS \
     + sizeof(uint32_t) )

#define FILE_LOCATION_FOR_TRASH_INDEX (\
     FILE_LOCATION_FOR_TABLE_OF_OBJECTS_INDEX \
     + sizeof(TDskObjIndex) )

#define OFFSET_FOR_BLOCK(INDEX, SIZE) (sizeof(TDskObjectStoreFileHeader) + (INDEX * (sizeof(TDskObjectStoreBlockHeader) + SIZE)))

#define CONVERT_TO_FILE_OFFSET( INDEX, SIZE, OFFSET) (OFFSET_FOR_BLOCK(INDEX, SIZE) + sizeof(TDskObjectStoreBlockHeader) + OFFSET)

#define PHYSICAL_BLOCKSIZE(STORE) (STORE->fileHeader.header.blockSize + sizeof(TDskObjectStoreBlockHeader) )

// default internal IDs
static const uint32_t OBJECT_TABLE_ID = 0;
static const uint32_t TRASH_TABLE_ID = 1;
static const uint32_t DEFUALT_BLOCKSIZE = 128;
static const uint32_t VERSION = 1;
static const uint32_t INITIAL_NUMBER_OF_OBJECTS = 2;
static const uint32_t NO_BLOCK = 0xFFFFFFFF;

// structure definitions
static const int8_t FILE_ID[4] = { 'O', 'S', 'T', 'R'};
static const int8_t BLOCK_ID[4] = { 'B', 'L', 'C', 'K' };
#define IDS_MATCH( A, B ) ( A[0] == B[0] && A[1] == B[1] && A[2] == B[2] && A[3] == B[3])
#define SET_ID( A, B ) {A[0] = B[0]; A[1] = B[1]; A[2] = B[2]; A[3] = B[3];}



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
    bool                        dirtyflag;
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
} TDskObjIndex;

typedef struct
{
    TDskObjIndex  header;
    bool                    dirtyFlag;
} TObjIndex;



typedef struct _TOStore
{
    FILE*                   fp;
    TOStreamMode            fileMode;
    TObjectStoreFileHeader  fileHeader;
    TObjIndex               tableOfObjectsHeader;
    TObjIndex               tashHeader;
    // working memory
    uint32_t                numberOfObjects;    
} TOStore;






// internal function definitions
void* zeromalloc(size_t size);
void zfree( void* ptr );


int addBlocksToIndex(TOStoreHnd store, TDskObjIndex* head, TDskObjectStoreBlockHeader* newBlocks);
int removeBlocksFromIndex(TOStoreHnd store, TDskObjIndex* index, TDskObjectStoreBlockHeader* freeBlocks, uint32_t numberOfBlocksToRemove);

int setLengthWithIndex(TOStoreHnd store, TDskObjIndex* head, uint32_t lengthRequested);
int growLengthWithIndex(TOStoreHnd store, TDskObjIndex* head, uint32_t blocksToAdd);
int shirnkLengthWithIndex(TOStoreHnd store, TDskObjIndex* head, uint32_t blocksToRemove);

int readObjectIndex(TOStoreHnd oStore, TOStoreObjID id, TDskObjIndex* header);
int writeObjectIndex(TOStoreHnd oStore, TOStoreObjID id, TDskObjIndex* header);
int addObjectIndex(TOStoreHnd store, TDskObjIndex* header);

int readWithIndex(TOStoreHnd store, const TDskObjIndex* header, uint32_t position, uint32_t length, void* destination);
int writeWithIndex(TOStoreHnd store, const TDskObjIndex* header, uint32_t position, uint32_t length, void* source);


int addBlockToFile(FILE* fp, const TDskObjectStoreBlockHeader* header, uint32_t size);
int writeBlockHeader(TOStoreHnd store, const TDskObjectStoreBlockHeader* header);
int readBlockHeader(TOStoreHnd store, TDskObjectStoreBlockHeader* header, uint32_t index);
int updateFileHeader(TOStoreHnd store);
int writeObjectCount(TOStoreHnd store);


int readFromFile(FILE* fp, uint32_t offset, uint32_t length, uint8_t* buffer);
int writeToFile(FILE* fp, uint32_t offset, uint32_t length, const uint8_t* buffer);
#endif // OSTOREINTERNAL_H__