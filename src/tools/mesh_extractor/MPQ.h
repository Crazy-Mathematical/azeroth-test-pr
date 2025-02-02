/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#ifndef MPQ_H
#define MPQ_H

#include "libmpq/mpq.h"
#include "Define.h"
#include "Errors.h"
#include <string>
#include <ctype.h>
#include <vector>
#include <iostream>
#include <deque>

class MPQArchive
{

public:
    mpq_archive_s* mpq_a;

    std::vector<std::string> Files;

    MPQArchive(const char* filename);
    void close();

    void GetFileListTo(std::vector<std::string>& filelist)
    {
        uint32_t filenum;
        if (libmpq__file_number(mpq_a, "(listfile)", &filenum)) return;
        libmpq__off_t size, transferred;
        libmpq__file_unpacked_size(mpq_a, filenum, &size);

        char* buffer = new char[size + 1];
        buffer[size] = '\0';

        libmpq__file_read(mpq_a, filenum, (unsigned char*)buffer, size, &transferred);

        char seps[] = "\n";
        char* token;

        token = strtok( buffer, seps );
        uint32 counter = 0;
        while ((token != nullptr) && (counter < size))
        {
            //cout << token << endl;
            token[strlen(token) - 1] = 0;
            std::string s = token;
            filelist.push_back(s);
            counter += strlen(token) + 2;
            token = strtok(nullptr, seps);
        }

        delete[] buffer;
    }
};

class MPQFile
{
    //MPQHANDLE handle;
    bool eof;
    char* buffer;
    libmpq__off_t pointer, size;

    // disable copying
    MPQFile(const MPQFile& /*f*/) {}
    void operator=(const MPQFile& /*f*/) {}

public:
    MPQFile(const char* filename);    // filenames are not case sensitive
    ~MPQFile() { close(); }
    size_t Read(void* dest, size_t bytes);
    FILE* GetFileStream();
    size_t getSize() { return size; }
    size_t getPos() { return pointer; }
    char* getBuffer() { return buffer; }
    char* getPointer() { return buffer + pointer; }
    bool isEof() { return eof; }
    void seek(int offset);
    void seekRelative(int offset);
    void close();
};

inline void flipcc(char* fcc)
{
    char t;
    t = fcc[0];
    fcc[0] = fcc[3];
    fcc[3] = t;
    t = fcc[1];
    fcc[1] = fcc[2];
    fcc[2] = t;
}

#endif
