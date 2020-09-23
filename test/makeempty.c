#include <stdio.h>
#include "ostore.h"

int main(int argc, const char* argv[]) {
	TOStoreHnd store = NULL;
	int error = ERR_OK;

	printf("before 0x%08lx\n", (size_t)store);
	printf("creating an store...\n");
	error = ostore_create("/home/chris/development/ostore/test/test.store", &store);
	printf("completed with %d \n", error);
	printf("after 0x%08lx\n", (size_t)store);

	ostore_close(&store);
	printf("closed store");
	printf("after 0x%08lx\n", (size_t)store);
	return 0;
}


