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

enum TType {
    ENoType,
    EText,
    EFile
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
    for(int i = 0; i < mapSize; i++) {
        if ( map[i].string == NULL) {
            retval = map[i].id;
            break;
        }
    }

    // search for match, if search is NULL, we've already found it.
    if (search) {
        for(int i = 0; i < mapSize; i++) {
            if(map[i].string) {
                int match = strcmp(search, map[i].string);
                if (match == 0 ) {
                    retval = i;
                    break;
                }
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
    
    TParameters();
    void            populate(int argc, const char* argv[]);
    void            validate();

};


TParameters::TParameters()
: m_filename(), m_string()
{
    m_function = EMalformed;
    m_type = ENoType;
    m_id = 0;
}

void TParameters::populate(int argc, const char* argv[]) {
    for(int i = 1; i < argc; i++) {
        switch (i) {
            case 1:
                m_filename = argv[i];
            break;
            case 2:
                m_function  = (TFunction) processMap(FUNCTION_MAP, FUNCTION_MAP_MAX, argv[i]);
            break;

            case 3:
                
            break;
        }
    }

}

int main(int argc, const char* argv[]) {

}