#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include <string>

enum TFunction {
    EMalformed = 0,
    ENoFunction = 1,
    ECreate = 2,
    EList = 3,
    EExtract = 4,
    EInsert = 5 
};

enum TType {
    ENoType = 0,
    EText = 1,
    EFile = 2
};

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

#endif //PARAMETERS_H_