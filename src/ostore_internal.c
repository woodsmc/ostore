#include "ostore_internal.h"



int setLengthWithIndex(TOStore* store, TDskObjIndex* head, uint32_t lengthRequested) {
    START;
    uint32_t numberOfBlockRequested = lengthRequested / store->fileHeader.header.blockSize;    

    if (numberOfBlockRequested > head->numberOfBlocks) {
        // add blocks
        uint32_t blocksToAdd = numberOfBlockRequested - head->numberOfBlocks;
        retval = growLengthWithIndex(store, head, blocksToAdd);
        IF_NOT_OK_HANDLE_ERROR(retval);
    } else if (numberOfBlockRequested < head->numberOfBlocks) {
        // remove available blocks from trash
        uint32_t blocksToRemove = head->numberOfBlocks - numberOfBlockRequested;
        retval = shirnkLengthWithIndex(store, head, blocksToRemove);
        IF_NOT_OK_HANDLE_ERROR(retval);
    }

    PROCESS_ERROR;

    FINISH;
}

int growLengthWithIndex(TOStore* store, TDskObjIndex* head, uint32_t blocksToAdd) {
    START;
    uint32_t blocksAvailableFromTrash = store->tashHeader.header.numberOfBlocks;
    uint32_t blocksToTakeFromTrash = blocksAvailableFromTrash;
    uint32_t blocksToAddToFile= 0;
    uint32_t sequenceNumber = 0;
    uint32_t lastBlockNumber = 0;

    if (blocksToAdd > blocksAvailableFromTrash) {
        blocksToTakeFromTrash = blocksAvailableFromTrash;
        blocksToAddToFile = blocksToAdd - blocksAvailableFromTrash;
    } else /* if ( blocksToAdd <= blocksAvailableFromTrash) */{
        blocksToTakeFromTrash = blocksToAdd;
        blocksToAddToFile = 0;
    }

    LOCAL_MEMZ(TDskObjectStoreBlockHeader,blockhead);
    // consume anything in the trash.
    if ( blocksToTakeFromTrash > 0) {        
        retval = removeBlocks(store, head, &blockhead, blocksToTakeFromTrash);
        IF_NOT_OK_HANDLE_ERROR(retval);
        retval = addBlocksToIndex(store, head, &blockhead);
        IF_NOT_OK_HANDLE_ERROR(retval);
    }

    

    LOCAL_MEMZ(TDskObjectStoreBlockHeader, brandNewBlocks);
    bool startOfNewBlocksSaved = false;
    // add a linked list of empty blocks to the end of the file    
    memset(&blockhead, 0, sizeof(TDskObjectStoreBlockHeader));
    while ( blocksToAddToFile > 0) {        
        blockhead.blockFileIndex = store->fileHeader.header.blocksInFile++; 
        if(blockhead.sequenceNumber == 0) {
            blockhead.last = NO_BLOCK;
        } else {
            blockhead.last = lastBlockNumber;
        }
        
        if ( blocksToAddToFile > 1 ) {
            blockhead.next = blockhead.sequenceNumber + 1;
        } else {
            blockhead.next = NO_BLOCK;
        }
        if (!startOfNewBlocksSaved) {
            memcpy(&brandNewBlocks, &blockhead, sizeof(TDskObjectStoreBlockHeader));
        }
        startOfNewBlocksSaved = true;
        retval = addBlockToFile(store->fp, &blockhead, store->fileHeader.header.blockSize);
        IF_NOT_OK_HANDLE_ERROR(retval);
        blockhead.sequenceNumber++;
        lastBlockNumber = blockhead.blockFileIndex;
        blocksToAddToFile--;
    }

    // then add them to the index we are dealing with
    if ( startOfNewBlocksSaved) {        
        retval = addBlocksToIndex(store, head, &brandNewBlocks);
        IF_NOT_OK_HANDLE_ERROR(retval);
    }

    PROCESS_ERROR;

    FINISH;
}



int shirnkLengthWithIndex(TOStore* store, TDskObjIndex* head, uint32_t blocksToRemove) {
    // remove the necessary blocks from one object, then add them to the other.
    START;
    LOCAL_MEMZ(TDskObjectStoreBlockHeader, blocks);

    retval = removeBlocks(store, head, &blocks, blocksToRemove);
    IF_NOT_OK_HANDLE_ERROR(retval);
    retval = addBlocksToIndex(store, head, &blocks);

    PROCESS_ERROR;

    FINISH;
}




/*
    This function removes blocks from an object. It does not add them to another block (the free chain)
    it just leaves them dangling. 
*/
 int removeBlocks(TOStore* store, TDskObjIndex* index, TDskObjectStoreBlockHeader* freeBlocks, uint32_t numberOfBlocksToRemove) {
    assert(store);
    assert(index);
    assert(freeBlocks);
    assert(numberOfBlocksToRemove > 0);
    assert(numberOfBlocksToRemove < index->numberOfBlocks);

    START;
    LOCAL_MEMZ(TDskObjectStoreBlockHeader, currentBlock);
    retval = readBlockHeader(store, &currentBlock, index->tailBlock);
    uint32_t counter = numberOfBlocksToRemove;
    while(counter != 0) {            
        retval = readBlockHeader(store, &currentBlock, currentBlock.last);
        IF_NOT_OK_HANDLE_ERROR(retval);
        counter--;
    }

    retval = readBlockHeader(store, freeBlocks, currentBlock.last);
    IF_NOT_OK_HANDLE_ERROR(retval);
    
    // update blocks and write them back to disk, then update header
    index->tailBlock = currentBlock.blockFileIndex;
    freeBlocks->last = NO_BLOCK;
    currentBlock.next = NO_BLOCK;
    
    retval = writeBlockHeader(store, freeBlocks);
    IF_NOT_OK_HANDLE_ERROR(retval);

    retval = writeBlockHeader(store, &currentBlock);
    IF_NOT_OK_HANDLE_ERROR(retval);

    // write the update to the index
    index->numberOfBlocks -= numberOfBlocksToRemove;
    retval = writeObjectIndex(&store, index->id, index);
    IF_NOT_OK_HANDLE_ERROR(retval);
    
    // blocks are now removed and floating.
    PROCESS_ERROR;
    FINISH;
}

/*
    This function adds the newBlocks to the tail of the existing blocks / header code.
    Must deal with the head being emnpty, both head and tail pointing to NO_BLOCK...
    Must also resequence the blocks.
*/
int addBlocksToIndex(TOStore* store, TDskObjIndex* index, TDskObjectStoreBlockHeader* newBlocks) {
    assert(store);
    assert(index);
    assert(newBlocks);

    START;

    LOCAL_MEMZ(TDskObjectStoreBlockHeader, currentBlock);
    uint32_t tailBlock = 0;
    uint32_t sequenceNumber = 0;
    uint32_t numberOfNewBlocks = 0;

    // update head and write it.
    if ( index->headBlock == NO_BLOCK) {
        index->headBlock;
        // there is no need to update the last block 
        newBlocks->last = NO_BLOCK;        
    } else {
        // update the last block
        retval = readBlockHeader(store, &currentBlock, index->tailBlock);
        IF_NOT_OK_HANDLE_ERROR(retval);
        sequenceNumber = currentBlock.sequenceNumber;
        currentBlock.next = newBlocks->blockFileIndex;
        retval = writeBlockHeader(store, &currentBlock);
        IF_NOT_OK_HANDLE_ERROR(retval);
    }

    // loop through new blocks, update sequence, get the tailBlock
    // 
    memcpy(&currentBlock, newBlocks, sizeof(TDskObjectStoreBlockHeader));
    currentBlock.sequenceNumber = sequenceNumber++;
    tailBlock = currentBlock.blockFileIndex;
    retval = writeBlockHeader(store, &currentBlock);
    IF_NOT_OK_HANDLE_ERROR(retval);
    numberOfNewBlocks = 1;

    while(currentBlock.next != NO_BLOCK) {
        retval = readBlockHeader(store, &currentBlock, currentBlock.next);        
        IF_NOT_OK_HANDLE_ERROR(retval);
        tailBlock = currentBlock.blockFileIndex;
        currentBlock.sequenceNumber = sequenceNumber++;
        currentBlock.id = index->id;
        numberOfNewBlocks++;
        retval = writeBlockHeader(store, &currentBlock);
        IF_NOT_OK_HANDLE_ERROR(retval);
    };

    index->tailBlock = tailBlock;
    index->numberOfBlocks = numberOfNewBlocks;
    retval = writeObjectIndex(&store, index->id, index);
    IF_NOT_OK_HANDLE_ERROR(retval);

    PROCESS_ERROR;
    FINISH;
}


int writeWithIndex(TOStore* store, const TDskObjIndex* header, uint32_t position, uint32_t length, void* source) {
    START;
    uint32_t sequenceNumber = 0;
    uint32_t startingBlock = position / store->fileHeader.header.blockSize;

    // fast forward to the starting block
    // this iterates forward, but could be smart and work backward if that is less steps (TODO)
    uint32_t index = header->headBlock;
    LOCAL_MEMZ(TDskObjectStoreBlockHeader, currentBlockHdr);
    retval = readBlockHeader(store, &currentBlockHdr, index);
    IF_NOT_OK_HANDLE_ERROR(retval);

    while(sequenceNumber != startingBlock && index != NO_BLOCK) {
        INIT(TDskObjectStoreBlockHeader, currentBlockHdr);
        retval = readBlockHeader(store, &currentBlockHdr, index);
        IF_NOT_OK_HANDLE_ERROR(retval);
        sequenceNumber++;

        if ( sequenceNumber !=  startingBlock ) {
            index = currentBlockHdr.next;
            VALIDATE(index != NO_BLOCK, ERR_CORRUPT);
        }
    }

    
    // write in each subsequent block
    uint8_t* sourceArray = (uint8_t*)source;
    uint32_t offset = position - (startingBlock * store->fileHeader.header.blockSize);
    uint32_t lengthRemaining = length;    
    uint32_t lengthAvailableInThisBlock = store->fileHeader.header.blockSize - offset;
    uint32_t bytesToWrite = (lengthRemaining < lengthAvailableInThisBlock) ? lengthRemaining : lengthAvailableInThisBlock;
    uint32_t sourceOffset = 0;
    while(lengthRemaining > 0) {
        uint32_t positionInFile = CONVERT_TO_FILE_OFFSET(currentBlockHdr.blockFileIndex, store->fileHeader.header.versionNumner, offset);
        retval = writeToFile(store->fp, positionInFile, bytesToWrite, &sourceArray[sourceOffset]);
        IF_NOT_OK_HANDLE_ERROR(retval);
        offset = 0;
        sourceOffset += bytesToWrite;
        lengthRemaining -= bytesToWrite;
        bytesToWrite = (lengthRemaining < store->fileHeader.header.blockSize) ? lengthRemaining : store->fileHeader.header.blockSize;
        if ( lengthRemaining > 0 ) {
            VALIDATE(currentBlockHdr.next != NO_BLOCK, ERR_CORRUPT);
            retval = readBlockHeader(store, &currentBlockHdr, currentBlockHdr.next);
            IF_NOT_OK_HANDLE_ERROR(retval);
        }
    }


    PROCESS_ERROR;

    FINISH;
}

int addObjectIndex(TOStore* store, TDskObjIndex* header) {
    // ensure that there is enough space for the object header
    // then add the header at the next index    
    // update counter in file.
    // update in memory data structure

    assert(store != NULL);
    assert(header != NULL);

    START;
    uint32_t originalObjectCount = store->numberOfObjects;
    uint32_t nextObjectCount = originalObjectCount + 1;
    uint32_t spaceRequired = sizeof(uint32_t) + ( (nextObjectCount+1) * sizeof(TDskObjIndex));
    retval = setLengthWithIndex(store, &(store->tableOfObjectsHeader.header), spaceRequired);
    IF_NOT_OK_HANDLE_ERROR(retval);

    uint32_t offset = sizeof(uint32_t) + (nextObjectCount * sizeof(TDskObjIndex));
    retval = writeWithIndex(store, &(store->tableOfObjectsHeader.header), offset, sizeof(TDskObjIndex), header);
    IF_NOT_OK_HANDLE_ERROR(retval);

    retval = writeWithIndex(store, &(store->tableOfObjectsHeader.header), 0, sizeof(uint32_t), &nextObjectCount);
    IF_NOT_OK_HANDLE_ERROR(retval);

    store->numberOfObjects = nextObjectCount;
    PROCESS_ERROR;
    // remove object, if created - that is done by simply setting the counter back 
    // to the origin value and ensuring the file and disk are aligned.
    store->numberOfObjects = originalObjectCount;
    retval = writeWithIndex(store, &(store->tableOfObjectsHeader.header), 0, sizeof(uint32_t), &originalObjectCount);
    FINISH;
}

int writeObjectIndex(TOStoreHnd oStore, TOStoreObjID id, TDskObjIndex* header) {
    assert(oStore);
    assert(header);
    START;

    TOStore* store = (*oStore);
    // try to find the object header, and get the offset location for the write

    LOCAL_MEMZ(TDskObjIndex, tempObjHeader);
    bool found = false;
    uint32_t offset = sizeof(uint32_t);
    for(uint32_t i = 0; i < store->numberOfObjects; i++) {
        offset = (i * sizeof(TDskObjIndex)) + sizeof(uint32_t);
        memset(&tempObjHeader, 0, sizeof(TDskObjIndex));
        retval = readWithIndex(store, &store->tableOfObjectsHeader.header, offset, sizeof(TDskObjIndex), &tempObjHeader);
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
        uint32_t totalSpaceNeeded = sizeof(uint32_t) + ( numberOfObjects * sizeof(TDskObjIndex));
        if ( totalSpaceNeeded > (store->tableOfObjectsHeader.header.numberOfBlocks * store->fileHeader.header.blockSize) ) {
            retval = ostoreobj_setLength(oStore, store->tableOfObjectsHeader.header.id, totalSpaceNeeded);
            IF_NOT_OK_HANDLE_ERROR(retval);
        }

        // increment the number of objects and update the file to reflect it
        retval = writeWithIndex(store, &store->tableOfObjectsHeader.header, 0, sizeof(uint32_t), &numberOfObjects);
        IF_NOT_OK_HANDLE_ERROR(retval);
    }

    // 
    // overwrite if found

    PROCESS_ERROR;
    FINISH;
}

int readObjectIndex(TOStoreHnd oStore, TOStoreObjID id, TDskObjIndex* header){
    assert(oStore);
    assert(header);
    START;
    TOStore* store = (*oStore);
    uint32_t offset = sizeof(uint32_t);
    
    retval = ERR_NOT_FOUND;
    for(uint32_t i = 0; i < store->numberOfObjects; i++) {
        offset = (i * sizeof(TDskObjIndex)) + sizeof(uint32_t);
        memset(header, 0, sizeof(TDskObjIndex));
        retval = readWithIndex(store, &store->tableOfObjectsHeader.header, offset, sizeof(TDskObjIndex), header);
        IF_NOT_OK_HANDLE_ERROR(retval);
        if ( header->id == id)
            break;
    }

    PROCESS_ERROR;

    FINISH;
}



// Reading and Writing Data
int readWithIndex(TOStore* store, const TDskObjIndex* header, uint32_t position, uint32_t length, void* destination) {
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
        IF_NOT_OK_HANDLE_ERROR(retval);
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
