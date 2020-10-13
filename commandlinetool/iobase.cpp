#include "iobase.h"
#include <string.h>
#include <assert.h>

// CIOBase.....................................................................
CIOBase::CIOBase() {
    m_parameters = NULL;
}

void CIOBase::setup(TParameters& parameters) {
    m_parameters = &parameters;
}

// CIOInputText................................................................
CIOInputText::CIOInputText() {
    m_dataread = false;
}

void CIOInputText::start() {
    m_dataread = false;
}

void CIOInputText::stop() {

}

bool CIOInputText::more() const {
    return (!m_dataread);
}

void CIOInputText::next(const uint8_t*& ptr, uint32_t& length) {
    if (!m_dataread) {
        const char* strPtr = parameters().m_string.c_str();
        const uint8_t* dataPtr = (const uint8_t*)(strPtr);
        ptr = dataPtr;
        length = parameters().m_string.size() + 1;
        m_dataread = true;
    } else {
        ptr = NULL;
        length = 0;
    }
}



// CIOInputFile................................................................
CIOInputFile::CIOInputFile() {
    memset(this, 0, sizeof(CIOInputFile));
}

CIOInputFile::~CIOInputFile() {
    stop();
}

bool CIOInputFile::more() const {
    assert(m_fp);
    bool retval = ( !feof(m_fp) && ( m_bytesRead < m_fileLength ) );
    return retval;
}

bool CIOInputFile::ok() const {
    bool retval = (m_fp != NULL);
    if ( retval ) {
        retval = more();
    }

    return retval;
}

void CIOInputFile::start() {
    stop();
    m_fp = fopen(parameters().m_filename.c_str(), "r");
    if (!m_fp) {
        printf("I can't open %s for reading.\n", parameters().m_filename.c_str());
        stop();
    } else {
        int success = fseek(m_fp, 0, SEEK_END);
        if ( success == 0 ) {
            m_fileLength = ftell(m_fp);
            success == fseek(m_fp, 0, SEEK_SET);
        }
        if ( success != 0 ) {
            printf("I can't get the length of %s.\n", parameters().m_filename.c_str());
            stop();
            return;
        }
        m_bytesRead = 0;
    }
}

void CIOInputFile::stop() {
    if ( m_fp ) {
        fclose(m_fp);
        m_fp = NULL;
        m_fileLength = 0;
        m_bytesRead = 0;
    }
}

void CIOInputFile::next(const uint8_t*& ptr, uint32_t& length) {
    memset(m_buffer,0, IO_STEAM_BUFFER_SIZE);
    length = 0;
    ptr = NULL;

    if ( m_fp ) {
        size_t bytesRemaining = m_bytesRead - m_fileLength;
        if (bytesRemaining > IO_STEAM_BUFFER_SIZE - 1) {
            bytesRemaining = IO_STEAM_BUFFER_SIZE - 1;
        }
        int read = fread(m_buffer, IO_STEAM_BUFFER_SIZE -1, 1, m_fp);
        if ( read == 1) {
            m_bytesRead += bytesRemaining;
            length = bytesRemaining;
            ptr = m_buffer;
        } 
    }
}




// CIOOutputText...............................................................
void CIOOutputText::stop() {
    printf("\n");
}

void CIOOutputText::next(const uint8_t* ptr, uint32_t length) {
    printf("%s", ptr);
}

// CIOOutputFile...............................................................
CIOOutputFile::CIOOutputFile() : CIOBaseWrite() {
    m_fp = NULL;
    stop();
}

CIOOutputFile::~CIOOutputFile() {
    stop();
}

void CIOOutputFile::start() {
    stop();
    m_fp  = fopen(parameters().m_filename.c_str(), "w");
    if ( m_fp == NULL ) {
        printf("I could not open the output file %s.\n", parameters().m_filename.c_str() );
        return;
    }
}

void CIOOutputFile::stop() {
    if ( m_fp ) {
        fclose(m_fp);
        m_fp = NULL;
    }
}

void CIOOutputFile::next(const uint8_t* ptr, uint32_t length) {
    if ( m_fp ) {
        int result = fwrite(ptr, length, 1, m_fp);
        if ( result != 1) {
            stop();
            printf("I could not write to %s.\n", parameters().m_filename.c_str());            
        }
    }
}

bool CIOOutputFile::ok() const {
    return (m_fp != NULL);
}
