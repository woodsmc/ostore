#include "ostore_internal.h"



int setLengthWithHeader(TOStore* store, TDskOStoreObjectHeader* head, uint32_t lengthRequested) {
    START;
    uint32_t numberOfBlockRequested = lengthRequested / store->fileHeader.header.blockSize;    

    if (numberOfBlockRequested > head->numberOfBlocks) {
        // add blocks
        uint32_t blocksToAdd = numberOfBlockRequested - head->numberOfBlocks;
        retval = growLengthWithHeader(store, head, blocksToAdd);
        IF_NOT_OK_HANDLE_ERROR(retval);
    } else if (numberOfBlockRequested < head->numberOfBlocks) {
        // remove available blocks from trash
        uint32_t blocksToRemove = head->numberOfBlocks - numberOfBlockRequested;
        retval = shirnkLengthWithHeader(store, head, blocksToRemove);
        IF_NOT_OK_HANDLE_ERROR(retval);
    }

    PROCESS_ERROR;

    FINISH;
}

int growLengthWithHeader(TOStore* store, TDskOStoreObjectHeader* head, uint32_t blocksToAdd) {
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
        retval = addBlocks(store, head, &blockhead);
        IF_NOT_OK_HANDLE_ERROR(retval);
    }

    retval = readBlockHeader(store, &blockhead, head->tailBlock);
    IF_NOT_OK_HANDLE_ERROR(retval);
    sequenceNumber = blockhead.sequenceNumber;
    lastBlockNumber = blockhead.id;

    // add the new stuff
    while ( blocksToAddToFile > 0) {
        blockhead.sequenceNumber++;
        blockhead.blockFileIndex = store->fileHeader.header.blocksInFile++;
        store->fileHeader.dirtyflag = true;
        blockhead.last = lastBlockNumber;
        if ( blocksToAddToFile > 1 ) {
            blockhead.next = sequenceNumber;
        } else {
            blockhead.next = NO_BLOCK;
        }
        addBlockToFile(store->fp, &blockhead, store->fileHeader.header.blockSize);

    }

    if ( store->fileHeader.dirtyflag) {
        store->fileHeader.dirtyflag = false;
        retval = writeToFile(store->fp, 0, sizeof(TDskObjectStoreFileHeader), &store->fileHeader.header);
        IF_NOT_OK_HANDLE_ERROR(retval);
    }

    retval = writeObjectHeader(&store, head->id, head);
    IF_NOT_OK_HANDLE_ERROR(retval);

    PROCESS_ERROR;

    FINISH;
}



int shirnkLengthWithHeader(TOStore* store, TDskOStoreObjectHeader* head, uint32_t blocksToRemove) {
    // remove the necessary blocks from one object, then add them to the other.
    START;
    LOCAL_MEMZ(TDskObjectStoreBlockHeader, blocks);

    retval = removeBlocks(store, head, &blocks, blocksToRemove);
    IF_NOT_OK_HANDLE_ERROR(retval);
    retval = addBlocks(store, head, &blocks);

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
    retval = writeObjectHeader(&store, head->id, head);
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
int addBlocks(TOStore* store, TDskOStoreObjectHeader* head, TDskObjectStoreBlockHeader* newBlocks) {
    START;

    LOCAL_MEMZ(TDskObjectStoreBlockHeader, currentBlock);
    uint32_t tailBlock = 0;
    uint32_t sequenceNumber = 0;

    // update head and write it.
    if ( head->headBlock == NO_BLOCK) {
        head->headBlock;
        // there is no need to update the last block 
        newBlocks->last = NO_BLOCK;        
    } else {
        // update the last block
        retval = readBlockHeader(store, &currentBlock, head->tailBlock);
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

    while(currentBlock.next != NO_BLOCK) {
        retval = readBlockHeader(store, &currentBlock, currentBlock.next);        
        IF_NOT_OK_HANDLE_ERROR(retval);
        tailBlock = currentBlock.blockFileIndex;
        currentBlock.sequenceNumber = sequenceNumber++;
        currentBlock.id = head->id;
        retval = writeBlockHeader(store, &currentBlock);
        IF_NOT_OK_HANDLE_ERROR(retval);
    };

    head->tailBlock = tailBlock;
    retval = writeObjectHeader(&store, head->id, head);
    IF_NOT_OK_HANDLE_ERROR(retval);

    PROCESS_ERROR;
    FINISH;
}


int writeWithHeader(TOStore* store, const TDskOStoreObjectHeader* header, uint32_t position, uint32_t length, void* source) {
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
