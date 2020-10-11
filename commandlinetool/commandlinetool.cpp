#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdint.h>

#include "parameters.h"

#define BANNER_TXT "oStore Command Line Tool Version 1.0 | by Chris Woods 2020\n"
#define ERROR_STOP_TXT  "There is an error, and I can not continue.\n"

typedef int (*TStoreFunction)(const TParameters& parameters);

int error(const TParameters& parameters);
int createNewStore(const TParameters& parameters);
int listContentsOfStore(const TParameters& parameters);
int extractObject(const TParameters& parameters);
int insertObject(const TParameters& parameters);

static const TStoreFunction STOREFUNCTIONS[] = {
    &error,
    &error,
    &createNewStore,
    &listContentsOfStore,
    &extractObject,
    &insertObject
};

int error(const TParameters& parameters) {
    printf(ERROR_STOP_TXT);
    return 1;
}

int createNewStore(const TParameters& parameters) {
    printf("TO BE DONE : %s\n", __PRETTY_FUNCTION__);
    return 0;
}

int listContentsOfStore(const TParameters& parameters) {
    printf("TO BE DONE : %s\n", __PRETTY_FUNCTION__);
    return 0;
}

int extractObject(const TParameters& parameters) {
    printf("TO BE DONE : %s\n", __PRETTY_FUNCTION__);
    return 0;
}

int insertObject(const TParameters& parameters) {
    printf("TO BE DONE : %s\n", __PRETTY_FUNCTION__);
    return 0;
}



int main(int argc, const char* argv[]) {
    printf(BANNER_TXT);
    TParameters parameters;
    parameters.populate(argc, argv);    
    parameters.validate();
    parameters.print();
    
    int retval = STOREFUNCTIONS[parameters.m_function](parameters);

    return retval;
}