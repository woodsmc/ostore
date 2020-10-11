/******************************************************************************
* OStore - a simple resliant binary object storage format                     *
*                                                                             *
* Copyright Notice and License                                                *
*  (c) Copyright 2020 Chris Woods.                                            *
*                                                                             *
*  Licensed under the Apache License, Version 2.0 (the "License"); you may    *
*  not use this file except in compliance with the License. You may obtain a  *
*  copy of the License at :  [http://www.apache.org/licenses/LICENSE-2.0]     *
*                                                                             *
*  Unless required by applicable law or agreed to in writing, software        *
*  distributed under the License is distributed on an "AS IS" BASIS,          *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
*  See the License for the specific language governing permissions and        *
*  limitations under the License.                                             *
*                                                                             *
******************************************************************************/

#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdint.h>

#include "parameters.h"
#include "ostorecmdconfig.h"
#include "ostore.h"

#define BANNER_TXT "oStore Command Line Tool, version %d.%d (c) Copyright Chris Woods 2020\n"\
"oStore library version %d.%d (c) Copyright Chris Woods 2020\n"

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
    printf(BANNER_TXT, 
        ostorecmd_VERSION_MAJOR, ostorecmd_VERSION_MINOR,
        ostore_version_major(), ostore_version_minor());
    TParameters parameters;
    parameters.populate(argc, argv);    
    parameters.validate();
    parameters.print();
    
    int retval = STOREFUNCTIONS[parameters.m_function](parameters);

    return retval;
}