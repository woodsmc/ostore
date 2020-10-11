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