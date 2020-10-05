#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ostore.h"


static const char* STORE_FILENAME = "/home/chris/development/ostore/test/test_shink.store";
static const TOStoreObjID STRING_ID = 0x77777777;
static const char* TEST_STRING = "Once, long ago, the Danish land owned the sway of a mighty monarch,\n"
"Scyld Scefing, the founder of a great dynasty, the Scyldings. This\n"
"great king Scyld had come to Denmark in a mysterious manner, since no\n"
"man knew whence he sprang. As a babe he drifted to the Danish shore in\n"
"a vessel loaded with treasures; but no man was with him, and there was\n"
"no token to show his kindred and race. When Scyld grew up he increased\n"
"the power of Denmark and enlarged her borders; his fame spread far and\n"
"wide among men, and his glory shone undimmed until the day when, full\n"
"of years and honours, he died, leaving the throne securely established\n"
"in his family. Then the sorrowing Danes restored him to the mysterious\n"
"ocean from which he had come to them. Choosing their goodliest ship,\n"
"they laid within it the corpse of their departed king, and heaped\n"
"around him all their best and choicest treasures, until the venerable\n"
"countenance of Scyld looked to heaven from a bed of gold and jewels;\n"
"then they set up, high above his head, his glorious gold-wrought\n"
"banner, and left him alone in state. The vessel was loosed from the\n"
"shore where the mourning Danes bewailed their departing king, and\n"
"drifted slowly away to the unknown west from which Scyld had sailed to\n"
"his now sorrowing people; they watched until it was lost in the\n"
"shadows of night and distance, but no man under heaven knoweth what\n"
"shore now holds the vanished Scyld. The descendants of Scyld ruled and\n"
"prospered till the days of his great-grandson Hrothgar, one of a\n"
"family of four, who can all be identified historically with various\n"
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
		int error2 = ostore_getLength(store, id, &length);
		printf("[%s]  [%d][%d] object id %d | %x  has length %d | %x\n", indexType(i), error, error2, id, id, length, length);
	}
	printf("---------------------------------------------------------\n");
	return error;
}

int makestore(const char* filename) {
    TOStoreHnd store = NULL;
	int error = ERR_OK;
    error = ostore_create(filename, &store);
    assert(error == ERR_OK);
    const uint32_t SIZE = strlen(TEST_STRING) + 1;
    error = ostrore_addObjectWithId(store, STRING_ID, SIZE);
    assert(error == ERR_OK);
    error = ostore_write(store, STRING_ID, 0, TEST_STRING, SIZE);
    assert(error == ERR_OK);
    ostore_close(&store);
    return error;
}

int main(int argc, const char* argv[]) {
	
	TOStoreHnd store = NULL;
	int error = ERR_OK;
    error = ostore_create(STORE_FILENAME, &store);
    assert(error == ERR_OK);
    const uint32_t SIZE = strlen(TEST_STRING) + 1;
    error = ostrore_addObjectWithId(store, STRING_ID, SIZE);
    assert(error == ERR_OK);
    error = ostore_write(store, STRING_ID, 0, TEST_STRING, SIZE);
    assert(error == ERR_OK);
    
    printf("store created:\n");
	printf("---------------------------------------------------------\n");
    error = readstore(store);
    assert(error == ERR_OK);
    uint32_t length = 0;
    error = ostore_getLength(store, STRING_ID, &length);
    assert(error == ERR_OK);
    uint32_t shortLength = length / 2;
    printf("shrinking id %d from size %u to size %u\n", STRING_ID, length, shortLength);
    error = ostore_setLength(store, STRING_ID, shortLength);
    printf("completed with %d error\n", error);
	printf("---------------------------------------------------------\n");
    error = readstore(store);
	ostore_close(&store);
	printf("closed store");
	printf("after 0x%08lx\n", (size_t)store);

	return 0;
}


