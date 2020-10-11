#include "ostore_internal.h"

static const int8_t BLOCK_FILL[4] = { 'E', 'M', 'P', 'Y'};

int setLengthWithIndex(TOStoreHnd store, TDskObjIndex* head, uint32_t lengthRequested) {
    START;
    uint32_t numberOfBlockRequested = REQUIRED_BLOCKS_FOR_BYTES(store, lengthRequested);

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

int growLengthWithIndex(TOStoreHnd store, TDskObjIndex* head, uint32_t blocksToAdd) {
    START;
    uint32_t blocksAvailableFromTrash = store->tashHeader.header.numberOfBlocks;
    uint32_t blocksToTakeFromTrash = blocksAvailableFromTrash;
    uint32_t blocksToAddToFile= 0;

    if (blocksToAdd > blocksAvailableFromTrash) {
        // if we need more blocks, than the number available from the trash list
        // then take all available from trash, and work out the number to add
        // as new.
        blocksToTakeFromTrash = blocksAvailableFromTrash;
        blocksToAddToFile = blocksToAdd - blocksAvailableFromTrash;
    } else /* if ( blocksToAdd <= blocksAvailableFromTrash) */{
        // if we need less blocks than the total available from trash
        // then take the entire requested amount from trash, we don't
        // need to add any new blocks.
        blocksToTakeFromTrash = blocksToAdd;
        blocksToAddToFile = 0;
    }

    // First: Add the blocks we've available in trash
    LOCAL_MEMZ(TDskObjectStoreBlockHeader,blockhead);
    // consume anything in the trash.
    if ( blocksToTakeFromTrash > 0) {
        retval = removeBlocksFromIndex(store, &store->tashHeader.header, &blockhead, blocksToTakeFromTrash);
        IF_NOT_OK_HANDLE_ERROR(retval);
        retval = addBlocksToIndex(store, head, &blockhead);
        IF_NOT_OK_HANDLE_ERROR(retval);
    }


    // Second: Add the required number of new blocks to the file.
    // then once added to the file, add them to the index.
    if (blocksToAddToFile > 0) {
        LOCAL_MEMZ(TDskObjectStoreBlockHeader, brandNewBlocks);
        bool startOfNewBlocksSaved = false;
        // add a linked list of empty blocks to the end of the file
        memset(&blockhead, 0, sizeof(TDskObjectStoreBlockHeader));
        SET_ID(blockhead.identifyingWord, BLOCK_ID);
        blockhead.sequenceNumber = 0;
        while ( blocksToAddToFile > 0) {
            // the new block has an index, update the file counter (in RAM)
            // and set the new block index.
            blockhead.blockFileIndex = store->fileHeader.header.blocksInFile++;
            // set the block's last pointer, to the last added block header
            // except if this is the first block we are adding.
            if(blockhead.sequenceNumber == 0) {
                blockhead.last = NO_BLOCK;
            } else {
                blockhead.last = blockhead.blockFileIndex - 1;
            }

            // if we've still got at least one more block to add then we know
            // that the next block number will be
            if ( blocksToAddToFile > 1 ) {
                blockhead.next = blockhead.blockFileIndex + 1;
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
            blocksToAddToFile--;
        }

        // update the file header to show new total block count
        retval = updateFileHeader(store);
        IF_NOT_OK_HANDLE_ERROR(retval);

        // then add them to the index we are dealing with
        retval = addBlocksToIndex(store, head, &brandNewBlocks);
        IF_NOT_OK_HANDLE_ERROR(retval);


    }


    PROCESS_ERROR;

    FINISH;
}



int shirnkLengthWithIndex(TOStoreHnd store, TDskObjIndex* head, uint32_t blocksToRemove) {
    // remove the necessary blocks from one object, then add them to the trash
    START;
    LOCAL_MEMZ(TDskObjectStoreBlockHeader, blocks);

    retval = removeBlocksFromIndex(store, head, &blocks, blocksToRemove);
    IF_NOT_OK_HANDLE_ERROR(retval);
    retval = addBlocksToIndex(store, &store->tashHeader.header, &blocks);
    IF_NOT_OK_HANDLE_ERROR(retval);

    PROCESS_ERROR;

    FINISH;
}




/*
    This function removes blocks from an object. It does not add them to another block (the free chain)
    it just leaves them dangling.
*/
 int removeBlocksFromIndex(TOStoreHnd store, TDskObjIndex* index, TDskObjectStoreBlockHeader* freeBlocks, uint32_t numberOfBlocksToRemove) {
    assert(store);
    assert(index);
    assert(freeBlocks);
    assert(numberOfBlocksToRemove > 0);
    assert(numberOfBlocksToRemove <= index->numberOfBlocks);

    START;
    if (numberOfBlocksToRemove == index->numberOfBlocks) { // we are removing all blocks! :-)
        // this is easy, we only need to update the index
        // everything else (the blocks and their double linked list) stays the same.
        // before we do, we need to make sure that freeBlocks contains the head block
        retval = readBlockHeader(store, freeBlocks, index->headBlock);
        IF_NOT_OK_HANDLE_ERROR(retval);

        index->numberOfBlocks = 0;
        index->headBlock = NO_BLOCK;
        index->tailBlock = NO_BLOCK;


    } else { // we are doing a partial removal.
        LOCAL_MEMZ(TDskObjectStoreBlockHeader, currentBlock);
        retval = readBlockHeader(store, &currentBlock, index->tailBlock);
        uint32_t counter = numberOfBlocksToRemove;
        while(counter != 0) {
            retval = readBlockHeader(store, &currentBlock, currentBlock.last);
            IF_NOT_OK_HANDLE_ERROR(retval);
            counter--;
        }
        
        // the currentBlock == the last block we should have on the index
        // the first new "freeBlocks" is the last block
        VALIDATE(currentBlock.next == NO_BLOCK, ERR_CORRUPT); // if this happens the file is corrupt.
        retval = readBlockHeader(store, freeBlocks, currentBlock.next);
        IF_NOT_OK_HANDLE_ERROR(retval);

        // update blocks and write them back to disk, then update header
        index->tailBlock = currentBlock.blockFileIndex;
        freeBlocks->last = NO_BLOCK;
        currentBlock.next = NO_BLOCK;
        index->numberOfBlocks -= numberOfBlocksToRemove;

        retval = writeBlockHeader(store, freeBlocks);
        IF_NOT_OK_HANDLE_ERROR(retval);

        retval = writeBlockHeader(store, &currentBlock);
        IF_NOT_OK_HANDLE_ERROR(retval);        
    }



    // write the update to the index    
    retval = writeObjectIndex(store, index->id, index);
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
int addBlocksToIndex(TOStoreHnd store, TDskObjIndex* index, TDskObjectStoreBlockHeader* newBlocks) {
    assert(store);
    assert(index);
    assert(newBlocks);

    START;

    LOCAL_MEMZ(TDskObjectStoreBlockHeader, currentBlock);
    LOCAL_MEMZ(TDskObjectStoreBlockHeader, previousBlock);

    uint32_t sequenceNumber = 0;
    uint32_t numberOfNewBlocks = 0;
    uint32_t previousIndex = index->tailBlock;
    uint32_t currentIndex = newBlocks->blockFileIndex;


    // if the index contains no blocks, then headBlock == NO_BLOCK
    // reset the headBlock to point to the newBlocks to add
    if (index->headBlock == NO_BLOCK) { // block is empty
        index->headBlock = newBlocks->blockFileIndex;
    }

    while(currentIndex != NO_BLOCK) {

        if ( previousIndex != NO_BLOCK ) {
            retval = readBlockHeader(store, &previousBlock, previousIndex);
            IF_NOT_OK_HANDLE_ERROR(retval);
            sequenceNumber = previousBlock.sequenceNumber + 1;
        }

        retval = readBlockHeader(store, &currentBlock, currentIndex);
        IF_NOT_OK_HANDLE_ERROR(retval);

        if ( previousIndex != NO_BLOCK) {
            previousBlock.next = currentIndex;
            previousBlock.id = index->id;
        }

        currentBlock.last = previousIndex;
        currentBlock.sequenceNumber = sequenceNumber;
        currentBlock.id = index->id;

        // save our states
        if ( previousIndex != NO_BLOCK) {
            retval = writeBlockHeader(store, &previousBlock);
            IF_NOT_OK_HANDLE_ERROR(retval);
        }

        retval = writeBlockHeader(store, &currentBlock);
        IF_NOT_OK_HANDLE_ERROR(retval);

        // previous iteration
        previousIndex = currentIndex;

        // iterate currentIndex
        currentIndex = currentBlock.next;
        numberOfNewBlocks++;
    }

    // update the index with the number of last block address
    // and update the total number of blocks
    index->tailBlock = previousIndex;
    index->numberOfBlocks += numberOfNewBlocks;

    // write the index
    retval = writeObjectIndex(store, index->id, index);
    IF_NOT_OK_HANDLE_ERROR(retval);

    PROCESS_ERROR;
    FINISH;
}


int writeWithIndex(TOStoreHnd store, const TDskObjIndex* header, uint32_t position, uint32_t length, void* source) {
    START;

    // pre allocated local variables
    uint8_t* sourceArray = NULL;
    uint32_t offset = 0;
    uint32_t lengthRemaining = 0;
    uint32_t lengthAvailableInThisBlock = 0;
    uint32_t bytesToWrite = 0;
    uint32_t sourceOffset = 0;

    // start of work here.
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
    // initialize variables here
    sourceArray = (uint8_t*)source;
    offset = position - (startingBlock * store->fileHeader.header.blockSize);
    lengthRemaining = length;
    lengthAvailableInThisBlock = store->fileHeader.header.blockSize - offset;
    bytesToWrite = (lengthRemaining < lengthAvailableInThisBlock) ? lengthRemaining : lengthAvailableInThisBlock;
    sourceOffset = 0;
    // loop and work here
    while(lengthRemaining > 0) {
        uint32_t positionInFile = CONVERT_TO_FILE_OFFSET(currentBlockHdr.blockFileIndex, store->fileHeader.header.blockSize, offset);
        retval = writeToFile(store->fp, positionInFile, bytesToWrite, &sourceArray[sourceOffset]);
        IF_NOT_OK_HANDLE_ERROR(retval);
        offset = 0;
        sourceOffset += bytesToWrite;
        lengthRemaining -= bytesToWrite;
        bytesToWrite = (lengthRemaining < store->fileHeader.header.blockSize) ? lengthRemaining : store->fileHeader.header.blockSize;
        if ( lengthRemaining > 0 ) {
            VALIDATE(currentBlockHdr.next == NO_BLOCK, ERR_CORRUPT);
            retval = readBlockHeader(store, &currentBlockHdr, currentBlockHdr.next);
            IF_NOT_OK_HANDLE_ERROR(retval);
        }
    }


    PROCESS_ERROR;

    FINISH;
}

int addObjectIndex(TOStoreHnd store, TDskObjIndex* header) {
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
    uint32_t offset = 0;
    retval = setLengthWithIndex(store, &(store->tableOfObjectsHeader.header), spaceRequired);
    IF_NOT_OK_HANDLE_ERROR(retval);

    offset = sizeof(uint32_t) + (nextObjectCount * sizeof(TDskObjIndex));
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

int writeObjectCount(TOStoreHnd store) {
    assert(store);
    START;
    retval = writeWithIndex(store, &store->tableOfObjectsHeader.header, 0, sizeof(uint32_t), &store->numberOfObjects);
    IF_NOT_OK_HANDLE_ERROR(retval);

    PROCESS_ERROR;
    FINISH;
}

int writeObjectIndex(TOStoreHnd oStore, TOStoreObjID id, TDskObjIndex* header) {
    assert(oStore);
    assert(header);
    START;

    // try to find the object header, and get the offset location for the write
    LOCAL_MEMZ(TDskObjIndex, tempObjHeader);
    bool found = false;
    uint32_t offset = sizeof(uint32_t);
    for(uint32_t i = 0; i < oStore->numberOfObjects; i++) {
        offset = (i * sizeof(TDskObjIndex)) + sizeof(uint32_t);
        memset(&tempObjHeader, 0, sizeof(TDskObjIndex));
        retval = readWithIndex(oStore, &oStore->tableOfObjectsHeader.header, offset, sizeof(TDskObjIndex), &tempObjHeader);
        IF_NOT_OK_HANDLE_ERROR(retval);
        if ( tempObjHeader.id == id) {
            found = true;
            retval = writeWithIndex(oStore, &oStore->tableOfObjectsHeader.header, offset, sizeof(TDskObjIndex), header);
            IF_NOT_OK_HANDLE_ERROR(retval);
            break;
        }
    }

    // if object header can not be found add it
    if(!found) {
        // check to see if there is space for the new header
        // if no, add the space
        uint32_t numberOfObjects = oStore->numberOfObjects + 1;
        uint32_t totalSpaceNeeded = sizeof(uint32_t) + ( numberOfObjects * sizeof(TDskObjIndex));
        if ( totalSpaceNeeded > (oStore->tableOfObjectsHeader.header.numberOfBlocks * oStore->fileHeader.header.blockSize) ) {
            retval = ostore_setLength(oStore, oStore->tableOfObjectsHeader.header.id, totalSpaceNeeded);
            IF_NOT_OK_HANDLE_ERROR(retval);
        }

        // increment the number of objects and update the file to reflect it
        retval = writeWithIndex(oStore, &oStore->tableOfObjectsHeader.header, 0, sizeof(uint32_t), &numberOfObjects);
        IF_NOT_OK_HANDLE_ERROR(retval);
        oStore->numberOfObjects = numberOfObjects;
        offset = (( numberOfObjects - 1) * sizeof(TDskObjIndex) ) + sizeof(uint32_t);
        retval = writeWithIndex(oStore, &oStore->tableOfObjectsHeader.header, offset, sizeof(TDskObjIndex), header);
        IF_NOT_OK_HANDLE_ERROR(retval);
    }

    PROCESS_ERROR;
    FINISH;
}

int readObjectIndex(TOStoreHnd oStore, TOStoreObjID id, TDskObjIndex* header){
    assert(oStore);
    assert(header);
    START;

    uint32_t offset = sizeof(uint32_t);

    retval = ERR_NOT_FOUND;
    for(uint32_t i = 0; i < oStore->numberOfObjects; i++) {
        offset = (i * sizeof(TDskObjIndex)) + sizeof(uint32_t);
        memset(header, 0, sizeof(TDskObjIndex));
        retval = readWithIndex(oStore, &oStore->tableOfObjectsHeader.header, offset, sizeof(TDskObjIndex), header);
        IF_NOT_OK_HANDLE_ERROR(retval);
        if ( header->id == id)
            break;
    }

    PROCESS_ERROR;

    FINISH;
}



// Reading and Writing Data
int readWithIndex(TOStoreHnd store, const TDskObjIndex* header, uint32_t position, uint32_t length, void* destination) {
   START;

    uint32_t objectSizeInBytes = store->fileHeader.header.blockSize * header->numberOfBlocks;
    uint32_t endOfRead = position + length;
    uint32_t sequenceBlock = position / store->fileHeader.header.blockSize;
    uint32_t index = 0;
    uint32_t consumedData = 0;
    uint32_t offsetInBlock = 0;
    uint8_t* dest = NULL;
    uint32_t lengthRemaining = 0;

    VALIDATE( endOfRead > objectSizeInBytes, ERR_OVERFLOW );
    LOCAL_MEMZ(TDskObjectStoreBlockHeader, currentBlockHdr);

    // read in the initial first block sequenceBlock == 0
    index = header->headBlock;
    retval = readBlockHeader(store, &currentBlockHdr, index);
    IF_NOT_OK_HANDLE_ERROR(retval);

    // this iterates forward to the required block in the chain
    index = currentBlockHdr.next;
    while(sequenceBlock > 0 && index != NO_BLOCK) {
        INIT(TDskObjectStoreBlockHeader, currentBlockHdr);
        retval = readBlockHeader(store, &currentBlockHdr, index);
        IF_NOT_OK_HANDLE_ERROR(retval);
        sequenceBlock--;
        if ( sequenceBlock > 0 ) {
            VALIDATE(index != NO_BLOCK, ERR_CORRUPT);
            index = currentBlockHdr.next;
        }
    }

    // we have found the right block, now we need to read until we've consumed all data
    memset(destination, 0, length);
    consumedData = 0;
    offsetInBlock = position - (sequenceBlock * store->fileHeader.header.blockSize);
    dest = (uint8_t*) destination;
    lengthRemaining = length;

    while (consumedData != length ) {
        // determine data we can read from this block here
        uint32_t dataToReadFromThisBlock = lengthRemaining;
        uint32_t dataAvailableInThisBlock = store->fileHeader.header.blockSize - offsetInBlock;

        if ( dataToReadFromThisBlock > dataAvailableInThisBlock)
            dataToReadFromThisBlock = dataAvailableInThisBlock;

        // convert to actual file position
        uint32_t positionInFile = CONVERT_TO_FILE_OFFSET(currentBlockHdr.blockFileIndex, store->fileHeader.header.blockSize, offsetInBlock);
        //printf("reading from position %d; block %d\n", positionInFile);
        // read the data into the destination at the correct location
        retval = readFromFile(store->fp, positionInFile, dataToReadFromThisBlock, &dest[consumedData]);
        IF_NOT_OK_HANDLE_ERROR(retval);

        // update our records on what we've read so far
        consumedData += dataToReadFromThisBlock;
        lengthRemaining -= dataToReadFromThisBlock;

        // in prep for reading more data, read in the next block header in the stream
        if ( currentBlockHdr.next != NO_BLOCK ) {
            uint32_t next = currentBlockHdr.next;
            INIT(TDskObjectStoreBlockHeader, currentBlockHdr);
            retval = readBlockHeader(store, &currentBlockHdr, next);
            IF_NOT_OK_HANDLE_ERROR(retval);
        }


    }


    PROCESS_ERROR;
    FINISH;
}



int readBlockHeader(TOStoreHnd store, TDskObjectStoreBlockHeader* header, uint32_t index) {
    START;
    uint32_t offset = OFFSET_FOR_BLOCK(index, store->fileHeader.header.blockSize);
    uint8_t* byteHeader = (uint8_t*)header;
    retval = readFromFile(store->fp, offset, sizeof(TDskObjectStoreBlockHeader), byteHeader);
    IF_NOT_OK_HANDLE_ERROR(retval);
    VALIDATE(header->blockFileIndex != index, ERR_CORRUPT);

    PROCESS_ERROR;
    FINISH;
}


int writeBlockHeader(TOStoreHnd store, const TDskObjectStoreBlockHeader* header) {
    uint32_t offset = OFFSET_FOR_BLOCK(header->blockFileIndex, store->fileHeader.header.blockSize);
    uint8_t* byteHeader = (uint8_t*)header;
    int retval = writeToFile(store->fp, offset, sizeof(TDskObjectStoreBlockHeader), byteHeader);
    return retval;
}


int readFromFile(FILE* fp, uint32_t offset, uint32_t length, uint8_t* buffer) {
    int retval = fseek(fp, offset, SEEK_SET);
    if (retval == 0) {
        memset(buffer, 0, length);
        retval = fread(buffer, length, 1, fp);
        if ( retval != 1 ) {
            retval = ERR_OVERFLOW;
        } else {
            retval = ERR_OK;
        }
    } else {
        retval = ERR_OVERFLOW;
    }

    return retval;
}

int updateFileHeader(TOStoreHnd store) {
    START;

    uint8_t* dataPtr = (uint8_t*)&store->fileHeader.header;
    retval = writeToFile(store->fp, 0, sizeof(TDskObjectStoreFileHeader), dataPtr);
    IF_NOT_OK_HANDLE_ERROR(retval);

    PROCESS_ERROR;
    FINISH;
}

int writeToFile(FILE* fp, uint32_t offset, uint32_t length, const uint8_t* buffer) {
    assert(fp);
    assert(buffer);
    int retval = fseek(fp, offset, SEEK_SET);
    if (retval == 0) {
        retval = fwrite(buffer, length, 1, fp);
        if ( retval != 1 ) {
            retval = ERR_OVERFLOW;
        } else {
            retval = fflush(fp); // check error response
            if ( retval != 0 ) {
                retval = ERR_OVERFLOW;
            }
        }
    } else {
        retval = ERR_OVERFLOW;
    }

    return retval;
}

int addBlockToFile(FILE* fp, const TDskObjectStoreBlockHeader* header, uint32_t size) {
    int retval = fflush(fp);

    retval = fseek(fp, 0, SEEK_END); // move to the end of the file.
    if (retval == 0 ) {
        retval = fwrite(header, sizeof(TDskObjectStoreBlockHeader),1, fp);

        if ( retval != 1) {
            retval = ERR_OVERFLOW;
        } else {
            // if written the header ok, then pad with empty space.
            size_t numberToWrite = size / sizeof(BLOCK_FILL);
            retval = 1;
            while (retval == 1 && numberToWrite > 0){
                retval = fwrite(&BLOCK_FILL[0], sizeof(BLOCK_FILL), 1, fp);
                numberToWrite--;
            }
            //printf("retval = %d, numberToWrite = %d\n", retval, numberToWrite);
            if ( retval != 1) {
                retval = ERR_OVERFLOW;
            } else {
                retval = fflush(fp); //assert(false);
            }
        }
    }
    return retval;
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
