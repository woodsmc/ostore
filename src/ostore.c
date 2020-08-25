#include "ostore.h"

// Store Handling
int ostore_open(const char* filename, TOStreamMode mode, TOStoreHnd* oStore) {
    /*
    if the file exits then
        open the file
        read in the file header important data
        &  populate the data structure
            byte sequence matches
            version number
            block size
            number of blocks matches
        validate that the header information makes sense.
            return error code if fails
       
        read the inital block
            read the block header
            & store locally
            read the number of entires in the Object Handling block
                number of objects
                sequence information for this Object handling block
                sequence information for the empty blocks

        store the 
            file pointer
            block size
            sequence information for the object handling block
            sequence information for the empty blocks
        
        
    if the file does not exist then
        create the file
        create the file header with 1 block
        create the first block with
            only one entry (this) for the object handling block sequence
        
        store the
            file pointer
            block size
            sequence information for the object handling block
            sequence information for the empty blocks
    */
}

void ostore_close(TOStoreHnd* oStore) {

}

// Object Inspection
int ostore_enumerateObjects(TOStoreHnd oStore, uint32_t* numerOfObjects) {

}

int ostore_getObjectIdFromIndex(TOStoreHnd oStore, uint32_t objectIndex, TOStoreObjID* id) {

}

int ostore_objectIdExists(TOStoreHnd oStore, TStoreObjectID id) {

}

// Object Management
int ostrore_addObjectWithId(TOStoreHnd oStore, TOStoreObjID id) {

}

int ostrore_addObject(TOStoreHnd oStore, TOStoreObjID* id) {

}

int ostore_removeObject(TOStoreHnd oStore, TOStoreObjID id) {

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
