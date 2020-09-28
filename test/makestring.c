#include <stdio.h>
#include <string.h>
#include "ostore.h"

int main(int argc, const char* argv[]) {
	TOStoreHnd store = NULL;
	int error = ERR_OK;

	printf("before 0x%08lx\n", (size_t)store);
	printf("creating an store...\n");
	error = ostore_create("/home/chris/development/ostore/test/test_string.store", &store);
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

	printf("creating, a new object to store a string..\n");
	const TOStoreObjID STRING_ID = 42;
	const char* TEST_STRING = "Hello world this is a test string, let's store this then read it back!";
	const uint32_t SIZE = strlen(TEST_STRING) + 1;
	error = ostrore_addObjectWithId(store, STRING_ID, SIZE);
	printf("created with error %d\n", error);
	// this should cause the creation of a new block in the file and an entry in the table.
	// the block should be visible in a hex editor... 

	ostore_close(&store);
	printf("closed store");
	printf("after 0x%08lx\n", (size_t)store);
	return 0;
}


