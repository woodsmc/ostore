#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ostore.h"

static const char* TEST_STRING = "Once, long ago, the Danish land owned the sway of a mighty monarch,"
"Scyld Scefing, the founder of a great dynasty, the Scyldings. This"
"great king Scyld had come to Denmark in a mysterious manner, since no"
"man knew whence he sprang. As a babe he drifted to the Danish shore in"
"a vessel loaded with treasures; but no man was with him, and there was"
"no token to show his kindred and race. When Scyld grew up he increased"
"the power of Denmark and enlarged her borders; his fame spread far and"
"wide among men, and his glory shone undimmed until the day when, full"
"of years and honours, he died, leaving the throne securely established"
"in his family. Then the sorrowing Danes restored him to the mysterious"
"ocean from which he had come to them. Choosing their goodliest ship,"
"they laid within it the corpse of their departed king, and heaped"
"around him all their best and choicest treasures, until the venerable"
"countenance of Scyld looked to heaven from a bed of gold and jewels;"
"then they set up, high above his head, his glorious gold-wrought"
"banner, and left him alone in state. The vessel was loosed from the"
"shore where the mourning Danes bewailed their departing king, and"
"drifted slowly away to the unknown west from which Scyld had sailed to"
"his now sorrowing people; they watched until it was lost in the"
"shadows of night and distance, but no man under heaven knoweth what"
"shore now holds the vanished Scyld. The descendants of Scyld ruled and"
"prospered till the days of his great-grandson Hrothgar, one of a"
"family of four, who can all be identified historically with various"
"Danish kings and princes.";

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


