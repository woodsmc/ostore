#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ostore.h"

const char* indexType(uint32_t in) {
	switch(in) {
		case 0:
		return "index";
		break;
		case 1:
		return "trash";
		break;
		default:
		return " user";
	}
}

int readstore(TOStoreHnd store) {
	printf("read store:\n");
	printf("---------------------------------------------------------\n");
	uint32_t max = 0;
	int error = ostore_enumerateObjects(store, &max);
	printf("[%d]  number of objects %d\n", error, max);

	for(uint32_t i = 0; i < max; i++) {
		TOStoreObjID id = 0;
		error = ostore_getObjectIdFromIndex(store, i, &id);
		uint32_t length = 0;
		int error2 = ostoreobj_getLength(store, id, &length);
		printf("[%s]  [%d][%d] object id %d | %x  has length %d | %x\n", indexType(i), error, error2, id, id, length, length);
	}
	printf("---------------------------------------------------------\n");
	return error;
}

int main(int argc, const char* argv[]) {
	const char* filename = "/home/chris/development/ostore/test/test_string.store";
	TOStoreHnd store = NULL;
	int error = ERR_OK;

	printf("before 0x%08lx\n", (size_t)store);
	printf("creating an store...\n");
	error = ostore_create(filename, &store);
	printf("completed with %d \n", error);
	printf("after 0x%08lx\n", (size_t)store);

	error = readstore(store);

	printf("creating, a new object to store a string..\n");
	const TOStoreObjID STRING_ID = 0x77777777;
	const char* TEST_STRING = "Hello world this is a test string, let's store this then read it back!";
	const uint32_t SIZE = strlen(TEST_STRING) + 1;
	char* string = malloc(SIZE);
	memset(string, 0, SIZE);
	error = ostrore_addObjectWithId(store, STRING_ID, SIZE);
	printf("created with error %d\n", error);
	// this should cause the creation of a new block in the file and an entry in the table.
	// the block should be visible in a hex editor...

	
	error = readstore(store);
	printf("read the store %d \n", error);

	// now write to it... 
	error = ostoreobj_write(store, STRING_ID, 0, TEST_STRING, SIZE);
	printf("written with error %d\n", error);

	memset(string, 0, SIZE);
	error = ostoreobj_read(store, STRING_ID, 0, SIZE, string);
	printf("[%d] read [%s]\n", error, string);

	ostore_close(&store);
	printf("closed store");
	printf("after 0x%08lx\n", (size_t)store);
	
	printf("---------------------------------------------------------\n");
	printf("open an store...\n");
	error = ostore_open(filename, EReadOnly, &store);
	printf("completed with %d \n", error);
	printf("after 0x%08lx\n", (size_t)store);
	
	memset(string, 0, SIZE);
	error = ostoreobj_read(store, STRING_ID, 0, SIZE, string);
	printf("[%d] read [%s]\n", error, string);
	free(string);

	ostore_close(&store);
	printf("closed store");
	printf("after 0x%08lx\n", (size_t)store);

	return 0;
}


