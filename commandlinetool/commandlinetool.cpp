#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdint.h>

enum TFunction {
    EMalformed = 0,
    ENoFunction,
    ECreate,
    EList,
    EExtract,
    EInsert    
};

static const char* COMMAND_TXT[] = {
    "Error",
    "Not specified",
    "Create new oStore",
    "List contents of oStore",
    "Extract an object from oStore",
    "Insert an object into oStore"
};

enum TType {
    ENoType,
    EText,
    EFile
};

static const char* TYPE_TXT[] = {
    "No type given",
    "Text from command line",
    "Data from a file"
};

struct TStringMap {
    int id;
    const char* string;
};

static const TStringMap FUNCTION_MAP[] = {
    {ENoFunction, NULL},
    {ECreate, "-create"},
    {ECreate, "-c"},
    {EList, "-list"},
    {EList, "-l"},
    {EExtract, "-extract"},
    {EExtract, "-e"},
    {EInsert, "-insert"},
    {EInsert, "-i"}
};

static const uint32_t FUNCTION_MAP_MAX = 9;

static const TStringMap TYPE_MAP[] = {
    {ENoType, NULL},
    {EText, "TEXT"},
    {EText, "T"},
    {EText, "t"},
    {EFile, "FILE"},
    {EFile, "F"},
    {EFile, "f"}
};

static const uint32_t TYPE_MAP_MAX = 7;

int processMap(const TStringMap map[], uint32_t mapSize, const char* search) {
    int retval = 0;
    // set retval to null value, if specified
    for(uint32_t i = 0; i < mapSize; i++) {
        if ( map[i].string == NULL) {
            retval = map[i].id;
            break;
        }
    }

    // search for match, if search is NULL, we've already found it.

    for(uint32_t i = 0; i < mapSize; i++) {
        if(map[i].string != NULL && search != NULL) {
            //printf("comparing [%s] == [%s]\n", search, map[i].string);
            int match = strcmp(search, map[i].string);
            if (match == 0 ) {
                retval = map[i].id;
                break;
            }
        }
    }

    return retval;
}

struct TParameters {
    std::string     m_filename;
    TFunction       m_function;
    uint32_t        m_id;
    TType           m_type;
    std::string     m_string;
    int             m_argc;

    TParameters();
    void            populate(int argc, const char* argv[]);
    void            validate();
    void            print() const;
};


TParameters::TParameters()
: m_filename("unknown"), m_string("unknown")
{
    m_function = EMalformed;
    m_type = ENoType;
    m_id = 0;
}

void TParameters::print() const {
    printf("Filename: %s\n", m_filename.c_str());
    printf("Function: %s\n", COMMAND_TXT[m_function]);
    printf("Object id: %u\n", m_id);
    printf("Data type: %s\n", TYPE_TXT[m_type]);
    printf("Data: %s\n", m_string.c_str());
    printf("Arguments given: %d\n", m_argc);
}

#define OBJECT_IS_NUM_TXT "An object ID must be a decemal number > 10\n"

void TParameters::populate(int argc, const char* argv[]) {
    m_argc = argc;
    m_function = ENoFunction; // assume none is specified
    //printf("m_argc = %d\n", m_argc);
    for(int i = 1; i < argc; i++) {
        switch (i) {
            case 1:
                m_filename = argv[i];
            break;
            case 2:
                //printf("searching for function [%s]\n", argv[i]);
                m_function  = (TFunction) processMap(FUNCTION_MAP, FUNCTION_MAP_MAX, argv[i]);
            break;

            case 3:
                try {
                    m_id = std::stoi( std::string(argv[i]) );
                    if ( m_id <= 10 ) {
                        throw std::invalid_argument(OBJECT_IS_NUM_TXT);
                    }
                } catch (const std::invalid_argument& exception) {
                    printf(OBJECT_IS_NUM_TXT);
                    m_function = EMalformed;
                }
            break;

            case 4:
                m_type = (TType) processMap(TYPE_MAP, TYPE_MAP_MAX, argv[i]);
            break;

            case 5:
                m_string.assign(argv[5]);
            break;
        }
    }

}

#define LIST_GUESS_TXT "You've not provided a function,\n" \
"I'm guessing you want to list the contents of a oStore.\n"

#define TOO_MANY_ARGS_TXT "You've provided more arguments than I know what to do with!\n"

#define TOO_FEW_ARGS_TXT "You are missing some arguments here, I'm not sure what to do.\n"
#define NO_FUNCTION_TXT "command: missing -> list?\n"
#define LIST_FUNCTION_TXT "command: list ostore\n"
#define CREATE_FUNCTION_TXT "command: create ostore\n"
#define EXTRACT_FUNCTION_TXT "command: extract\n"
#define TYPE_INFO_NOT_CLEAR_TXT "Did you mean TEXT|T or FILE|F? I'm confused."

void TParameters::validate() {
    switch (m_function) {
        case EMalformed:
        break;

        case ENoFunction:
        if (m_argc == 2) {
            m_function = EList;
            printf(LIST_GUESS_TXT);
        }

        case EList:
        case ECreate:
        if ( m_argc > 3) {
            m_function = EMalformed;
            printf(TOO_MANY_ARGS_TXT);
        }
        break;

        case EExtract:
        case EInsert:
        if (m_argc > 6) {
            m_function = EMalformed;
            printf(TOO_MANY_ARGS_TXT);
        }
        if (m_argc < 6 ) {
            m_function = EMalformed;
            printf(TOO_FEW_ARGS_TXT);
        }

        if ( m_type == ENoType) {
            m_function = EMalformed;
            printf(TYPE_INFO_NOT_CLEAR_TXT);
        }
        break;        
    }
}

#define BANNER_TXT "oStore Command Line Tool Version 1.0 | by Chris Woods 2020\n"
int main(int argc, const char* argv[]) {
    printf(BANNER_TXT);
    TParameters parameters;
    parameters.populate(argc, argv);    
    parameters.validate();
    parameters.print();
    
    //printf("Command: %s\n", COMMAND_TXT[parameters.m_function]);

    return 0;
}