#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ostore.h"


static const char* STORE_FILENAME = "/home/chris/development/ostore/test/test_add_delete.store";
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

static const char* TEST_STRING_2 = "Marty: It's from the Doc! (reading) \""
"Dear Marty, if my calculations are correct you will receive this letter "
"immediately after you saw the DeLorean struck by lightning. First let me "
"assure you that I am alive and well. I've been living happily these past "
"eight months in the year 1885. The lightning bolt-\" [looks at the date] "
"1885?! (at bottom of letter) \"September 1885!\" Argh! ";

static const TOStoreObjID STRING_ID2 = 88;

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

void dumpstore(const char* filename) {
	TOStoreHnd store = NULL;
	printf("open an store...\n");
	int error = ostore_open(filename, EReadOnly, &store);
	assert(error == ERR_OK);
	printf("completed with %d \n", error);
	printf("after 0x%08lx\n", (size_t)store);
	
	printf("---------------------------|%s|-----------------------------\n", filename);
	uint32_t max = 0;
	error = ostore_enumerateObjects(store, &max);
	assert(error == ERR_OK);
	printf("[%d]  number of objects %d\n", error, max);

	for(uint32_t i = 0; i < max; i++) {
		TOStoreObjID id = 0;
		error = ostore_getObjectIdFromIndex(store, i, &id);
		uint32_t length = 0;
		int error2 = ostore_getLength(store, id, &length);
		printf("[%s]  [%d][%d] object id %d | %x  has length %d | %x\n", indexType(i), error, error2, id, id, length, length);

	}
	printf("----------------------------|%s|----------------------------\n", filename);
	ostore_close(&store);
	printf("closed store");
	printf("after 0x%08lx\n", (size_t)store);
}

void makestore(const char* filename) {
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
}

void addobject(const char* filename) {
	printf("addobject...\n");
    TOStoreHnd store = NULL;
	int error = ERR_OK;
    error = ostore_open(filename, EReadWrite, &store);
    assert(error == ERR_OK);
    const uint32_t SIZE = strlen(TEST_STRING_2) + 1;
    error = ostrore_addObjectWithId(store, STRING_ID2, SIZE);
    assert(error == ERR_OK);
    error = ostore_write(store, STRING_ID2, 0, TEST_STRING_2, SIZE);
    assert(error == ERR_OK);
	error = readstore(store);
	char* tmp = malloc(SIZE);
	assert(tmp);
	memset(tmp, 0, SIZE);
	error = ostore_read(store, STRING_ID2, 0, SIZE-1, tmp);
	printf("[%d] read object %u : [%s]\n", error, STRING_ID2, tmp);
	free(tmp); tmp = NULL;
    ostore_close(&store);	
}

int main(int argc, const char* argv[]) {
	
	TOStoreHnd store = NULL;
	int error = ERR_OK;
	makestore(STORE_FILENAME);
    printf("store created.. now to reopen..\n");
	printf("--------------------------------------------------------\n");
	error = ostore_open(STORE_FILENAME, EReadWrite, &store);
	printf("[%d] reopened store\n", error);
	assert(error == ERR_OK);
    error = readstore(store);
    assert(error == ERR_OK);
	printf("removing object...\n");
    error = ostore_removeObject(store, STRING_ID);
	printf("[%d] removed object %u | 0x%x\n", error, STRING_ID, STRING_ID);
    assert(error == ERR_OK);
	printf("---------------------------------------------------------\n");
    error = readstore(store);
	ostore_close(&store);
	printf("closed store");
	printf("after 0x%08lx\n", (size_t)store);
	printf("---------------------------------------------------------\n");
	dumpstore(STORE_FILENAME);

	addobject(STORE_FILENAME);
	printf("---------------------------------------------------------\n");
	dumpstore(STORE_FILENAME);	

	return 0;
}


