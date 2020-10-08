#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ostore.h"

#define FNPRT printf(">FUNCTION:>%s\n", __PRETTY_FUNCTION__)

static const char* STORE_FILENAME = "/home/chris/development/ostore/test/test_add_multiple.store";

static const uint32_t STRINGS = 4;
static const TOStoreObjID STRINGID[] = {
    0x77777777,
    88,
    22,
    10
};
static const char* STRING[] = { "Once, long ago, the Danish land owned the sway of a mighty monarch,\n"
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
"Danish kings and princes.",

"Marty: It's from the Doc! (reading) \n"
"\"Dear Marty, if my calculations are correct you will receive this letter \n"
"immediately after you saw the DeLorean struck by lightning. First let me \n"
"assure you that I am alive and well. I've been living happily these past \n"
"eight months in the year 1885. The lightning bolt-\" [looks at the date] \n"
"1885?! (at bottom of letter) \"September 1885!\" Argh! ",

"A small test string",

"The game is based on the Alien films, specifically Aliens, and also on the 8-bit-era games Laser Squad and Paradroid[1] (although the game bears some gameplay similarities with Gauntlet, with which it has been compared,[2] as well an obvious comparison with Sega's Alien Syndrome, and Pandora's Into the Eagle's Nest[citation needed]). Alien Breed consists of the player or players having to find the lift down to the next level, occasionally setting the self-destruct sequence to blow up the level above them. The players collect or purchase a variety of weapons from the space station's computer terminals. In some versions of the game, these so-called Intex terminals provide additional features such as a clone of the classic computer game Pong. Credits found on the ground have to be saved for these weapons and other enhancements, each giving the players an edge over the gradually more and more powerful alien forces. In advanced levels, players are occasionally trapped in enclosed spaces with huge \"boss\" aliens."
};

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

void readstore(TOStoreHnd store) {
    FNPRT;
	printf("---------------------------------------------------------\n");
	uint32_t max = 0;
	int error = ostore_enumerateObjects(store, &max);
	printf("[%d]  number of objects %d\n", error, max);

	for(uint32_t i = 0; i < max; i++) {
		TOStoreObjID id = 0;
		error = ostore_getObjectIdFromIndex(store, i, &id);
        assert(error == ERR_OK);
		uint32_t length = 0;
		int error2 = ostore_getLength(store, id, &length);
		printf("[%s]  [%d][%d] object id %d | %x  has length %d | %x\n", indexType(i), error, error2, id, id, length, length);
        if ( !(id == 0 || id == 1) ) {
            char* buffer = malloc(length);
            assert(buffer);
            memset(buffer, 0, length);
            error = ostore_read(store, id, 0, length, buffer);
            assert(error == ERR_OK);
            printf("[%d]>>>\n%s\n<<<\n", error, buffer);
            free(buffer);
            buffer = NULL;
        } else {
            printf("skip..\n");
        }
	}
	printf("---------------------------------------------------------\n");
}


void dumpstore(const char* filename) {
	TOStoreHnd store = NULL;
	printf("open an store...\n");
	int error = ostore_open(filename, EReadOnly, &store);
	assert(error == ERR_OK);
	printf("completed with %d \n", error);
	printf("after 0x%08lx\n", (size_t)store);

    readstore(store);

	ostore_close(&store);
	printf("closed store");
	printf("after 0x%08lx\n", (size_t)store);
}


void makestore(const char* filename) {
    FNPRT;
    TOStoreHnd store = NULL;
	int error = ERR_OK;
    error = ostore_create(filename, &store);
    printf("[%d] store created %s\n", error, filename);
    assert(error == ERR_OK);

    for(uint32_t i =0 ; i < STRINGS; i++) {
        const uint32_t SIZE = strlen(STRING[i]) + 1;
        error = ostrore_addObjectWithId(store, STRINGID[i], SIZE);
        printf("[%d] object added [%u]\n", error, STRINGID[i]);
        assert(error == ERR_OK);
        error = ostore_write(store, STRINGID[i], 0, STRING[i], SIZE);
        printf("[%d] written data (%u) to the store\n", error, SIZE);
        assert(error == ERR_OK);
    }

    printf("store constructed, test before closing...\n");
    readstore(store);
    printf("test complete, close!\n");
    ostore_close(&store);
    printf("store created %s\n", filename);
}



int main(int argc, const char* argv[]) {
	FNPRT;

    makestore(STORE_FILENAME);
    dumpstore(STORE_FILENAME);

    printf("try deleting a node, and make sure remaining objects are ok\n");


	TOStoreHnd store = NULL;
	printf("open an store...\n");
	int error = ostore_open(STORE_FILENAME, EReadWrite, &store);
	assert(error == ERR_OK);
	printf("completed with %d \n", error);
	printf("after 0x%08lx\n", (size_t)store);
    printf("DELETING NOW... %d\n", STRINGID[2]);
    error = ostore_removeObject(store, STRINGID[2]);
    printf("[%d] removed object %u\n", error, STRINGID[2]);
    printf("check store that deletion worked ok, before closing...\n");
    readstore(store);    
    ostore_close(&store);
	printf("closed store");
	printf("after 0x%08lx\n", (size_t)store);
    
    printf("check store was written correctly...\n");
    dumpstore(STORE_FILENAME);
	return 0;
}


