#ifndef IO_BASE_H_
#define IO_BASE_H_
#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include "parameters.h"

// CIOBase.....................................................................
class CIOBase {
    public:
    CIOBase();
    virtual ~CIOBase() {};
    void setup(TParameters& parameters);
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool ok() const = 0;
    const TParameters& parameters() const { return *m_parameters; }
    TParameters& parameters() { return *m_parameters; }

    private:
    TParameters* m_parameters;
};

class CIOBaseRead : public CIOBase {
    public:
    CIOBaseRead() : CIOBase() {}
    virtual ~CIOBaseRead() {}
    virtual bool more() const = 0;
    virtual void next(const uint8_t*& ptr, uint32_t& length) = 0;
};

class CIOBaseWrite : public CIOBase {
    public:
    CIOBaseWrite() : CIOBase() {}
    virtual ~CIOBaseWrite() {}
    virtual void next(const uint8_t* ptr, uint32_t length) = 0;
};

// CIOInputText................................................................
class CIOInputText : public CIOBaseRead {
    public:
    CIOInputText();
    virtual ~CIOInputText() {}
    virtual bool more() const;
    virtual void start();
    virtual void stop();
    virtual bool ok() const { return true; }
    virtual void next(const uint8_t*& ptr, uint32_t& length);

    private:
    bool    m_dataread;
};

// CIOInputFile................................................................
#define IO_STEAM_BUFFER_SIZE (1024 * 4)
class CIOInputFile : public CIOBaseRead {
    public:
    CIOInputFile();
    virtual ~CIOInputFile();
    virtual bool more() const;
    virtual void start();
    virtual void stop();
    virtual bool ok() const;
    virtual void next(const uint8_t*& ptr, uint32_t& length);

    private:
    uint8_t         m_buffer[IO_STEAM_BUFFER_SIZE];
    size_t          m_fileLength;
    size_t          m_bytesRead;
    FILE*           m_fp;
};


// CIOOutputText...............................................................
class CIOOutputText : public CIOBaseWrite {
    public:
    CIOOutputText() : CIOBaseWrite() {}
    virtual ~CIOOutputText() {}
    virtual void start() {}
    virtual void stop();
    virtual bool ok() {return true; }
    virtual void next(const uint8_t* ptr, uint32_t length);
};


// CIOOutputFile...............................................................
class CIOOutputFile : public CIOBaseWrite {
    public:
    CIOOutputFile();
    virtual ~CIOOutputFile();
    virtual void start();
    virtual void stop();
    virtual bool ok() const;
    virtual void next(const uint8_t* ptr, uint32_t length);

    private:
    FILE*       m_fp;
};


#endif // IO_BASE_H_
