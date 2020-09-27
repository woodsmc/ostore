#include <stdio.h>
#include "ostore.h"

int main(int argc, const char* argv[]) {
	TOStoreHnd store = NULL;
	int error = ERR_OK;
    const char* filename = NULL;
    /*
    if ( argc != 2) {
        printf("not enough arguments (%d)\n", argc);
        return 1;
    }
    filename = argv[1];
    */
    filename = "/home/chris/development/ostore/test/test.store";

	printf("open an store...\n");
	error = ostore_open(filename, EReadOnly, &store);
	printf("completed with %d \n", error);
	printf("after 0x%08lx\n", (size_t)store);

	uint32_t max = 0;
	error = ostore_enumerateObjects(store, &max);
	printf("number of objects %d\n", max);
	for(uint32_t i = 0; i < max; i++) {
		TOStoreObjID id = 0;
		error = ostore_getObjectIdFromIndex(store, i, &id);
		printf("error %d, ostore_getObjectIdFromIndex (id = %u) \n", error, id);
		uint32_t length = 0;
		error = ostoreobj_getLength(store, id, &length);
		printf("error %d, ostoreobj_getLength (length = %u)\n", error, length);
		printf("object id %d has length %d\n", id, length);

	}

	ostore_close(&store);
	printf("closed store");
	printf("after 0x%08lx\n", (size_t)store);
	return 0;
}


