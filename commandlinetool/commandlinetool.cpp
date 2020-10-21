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
//#include "ostorecmdconfig.h" <-- turn this back on!
#include "ostore.h"
#include "iobase.h"
#include "debug.h"

#ifndef CMAKE_BUILD_ON
#define ostorecmd_VERSION_MAJOR 1
#define ostorecmd_VERSION_MINOR 2
#endif // CMAKE_BUILD_ON

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

static CIOBaseRead* makeReader(const TParameters& params) {
    CIOBaseRead* retval = NULL;
    switch(params.m_type) {
        case EFile:
        retval = new CIOInputFile();
        break;
        case EText:
        retval = new CIOInputText();
        break;
    }

    if ( retval ) {
        retval->setup(params);
    }
    return retval;
}

static CIOBaseWrite* makeWriter(const TParameters& params) {
    CIOBaseWrite* retval = NULL;
    switch(params.m_type) {
        case EFile:
        retval = new CIOOutputFile();
        break;
        case EText:
        retval = new CIOOutputText();
        break;
    }

    if ( retval ) {
        retval->setup(params);
    }
    return retval;
}

int error(const TParameters& parameters) {
    printf(ERROR_STOP_TXT);
    return 1;
}

int createNewStore(const TParameters& parameters) {
    TOStoreHnd store = NULL;
    printf("creating %s... ", parameters.m_filename.c_str());
    int error = ostore_create(parameters.m_filename.c_str(), &store);

    if(error != ERR_OK) {
        printf("[error %d]\n", error);
        return 1;
    }
    ostore_close(&store);
    printf("[ok]\n");
    return 0;
}

static const char* indexType(uint32_t in) {
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

static void readstore(TOStoreHnd store) {

	uint32_t max = 0;
    printf("number of objects... ");
	int error = ostore_enumerateObjects(store, &max);
    if ( error != 0 ) {
        printf( "[error %d]\n", error);
        return;
    }
    printf("[ok %d objects]\n", max);
    printf("------------------------------------\n");
    printf("  Index |   ID   |  Type  | Length  \n");
    printf("------------------------------------\n");
	uint32_t i = 0;
    for(i = 0; i < max; i++) {
		TOStoreObjID id = 0;
        uint32_t length = 0;
		error = ostore_getObjectIdFromIndex(store, i, &id);
        if ( error != 0 ) break;
		error = ostore_getLength(store, id, &length);
        if ( error != 0 ) break;
        printf("%8u|%8u|%8s|%8u\n", i, id, indexType(i), length);
	}

    if (error != 0) {
        printf("There was an error reading index %d.\n", i);
    }
	printf("------------------------------------\n");
}

int listContentsOfStore(const TParameters& parameters) {

	TOStoreHnd store = NULL;
    printf("opening %s... ", parameters.m_filename.c_str());
	int error = ostore_open(parameters.m_filename.c_str(), EReadOnly, &store);
    if(error != ERR_OK) {
        printf("[error %d]\n", error);
        return 1;
    }
    printf("[ok]\n");

    readstore(store);

	ostore_close(&store);
    return 0;
}

int extractObject(const TParameters& parameters) {
    FNPRT;
    TOStoreHnd store = NULL;
    printf("opening %s... ", parameters.m_filename.c_str());
    uint8_t* buffer = new uint8_t[IO_STEAM_BUFFER_SIZE];
    if (buffer == NULL) {
        printf("not enough memory available!\n");
        return 1;
    }

    PRINTF("---------\n");
	int error = ostore_open(parameters.m_filename.c_str(), EReadOnly, &store);
    PRINTF("---------\n");
    if(error != ERR_OK) {
        printf("[error %d]\n", error);
        return 1;
    }
    printf("[ok]\n");    

    CIOBaseWrite* writer = makeWriter(parameters);
    if ( writer == NULL) {
        return 1;
    }

    uint32_t lengthOfObject = 0;
    printf("checking for object with id %u and getting it's length ...", parameters.m_id);
    error = ostore_getLength(store, parameters.m_id, &lengthOfObject);
    if ( error != 0 ) {
        if ( error == ERR_NOT_FOUND) {
            printf("[not found]\n");
        } else {
            printf("[error %d]\n", error);
        }
        delete [] buffer;
        delete writer;
        return 1;
    }
    printf("length is %u [ok]\n", lengthOfObject);
    printf("reading object...");
    writer->start();
    uint32_t bytesRead = 0;
    uint32_t bytesRemaining = lengthOfObject;
    while(bytesRead < lengthOfObject) {
        memset(buffer, 0, IO_STEAM_BUFFER_SIZE);
        uint32_t bytesToReadThisTime = IO_STEAM_BUFFER_SIZE;
        if ( bytesRemaining < IO_STEAM_BUFFER_SIZE) {
            bytesToReadThisTime = bytesRemaining;
        }
        PRINTF("ostore_read(store, parameters.m_id, bytesRead[%u], bytesToReadThisTime[%u], buffer)\n", bytesRead, bytesToReadThisTime);
        error = ostore_read(store, parameters.m_id, bytesRead, bytesToReadThisTime, buffer);
        if ( error != 0 ) {
            printf("[error %d]", error);
            writer->stop();
            delete [] buffer;
            delete writer;            
            return 1;
        }

        writer->next(buffer, bytesToReadThisTime);
        bytesRead += bytesToReadThisTime;
        bytesRemaining -= bytesToReadThisTime;
    }
    printf("[ok]\n");
    writer->stop();
    delete writer;
    delete [] buffer;
    return 0;
}

int insertObject(const TParameters& parameters) {
    TOStoreHnd store = NULL;
    printf("opening %s... ", parameters.m_filename.c_str());
	int error = ostore_open(parameters.m_filename.c_str(), EReadWrite, &store);
    if(error != ERR_OK) {
        printf("[error %d]\n", error);
        return 1;
    }
    printf("[ok]\n");
    CIOBaseRead* reader = makeReader(parameters);
	if ( reader == NULL ) {
        return 1;
    }

    printf("removing existing object with ID %d... ", parameters.m_id);
    error = ostore_removeObject(store, parameters.m_id);
    if ( error == ERR_NOT_FOUND ) {
        printf(" [not found - new entry]\n");
    } else {
        printf(" [found and removed]\n");
    }

    reader->start();
    printf("adding object with ID %d, and length %u...", parameters.m_id, reader->length());
    error = ostrore_addObjectWithId(store, parameters.m_id, reader->length());
    if ( error != 0 ) {
        printf("[error %d]\n", error);
        ostore_close(&store);
        delete reader;
        return 1;
    }
    printf("[ok]\n");
    printf("writing data to object %d.", parameters.m_id);
    uint32_t offset = 0;
    error = 0;
    while(reader->more() && error == 0) {
        const uint8_t* ptr = NULL;
        uint32_t length = 0;
        reader->next(ptr, length);
        printf(">DEBUG length = %d\n", length);
        if ( ptr != NULL && length > 0) {
            printf(">DEBUG writing...\n");
            error = ostore_write(store, parameters.m_id, offset, ptr, length);
            offset += length;
            printf(".");
        }
    }

    if ( error != 0) {
        printf(" [error %d]\n", error);
    } else {
        printf(" [ok]\n");
    }

	ostore_close(&store);
    delete reader;
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
