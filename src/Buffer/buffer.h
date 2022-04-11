//
// Created by yumin_zhang on 2022/4/11.
//

#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <unistd.h>
#include <atomic>
#include <vector>
#include <assert.h>
#include <iostream>
#include <sys/uio.h> // readv or writev

class Buffer {
public:
    Buffer(int initSize = 1024);
    ~Buffer() = default;
    // only read op
    size_t writableBytes() const;
    size_t readableBytes() const;
    size_t preparableBytes() const;
    // only read op and return const value
    const char * peek() const;
    void ensureWritable(size_t len);
    void hasWritten(size_t len);

    void retrieve(size_t len);
    void retrieveUntil(char * end);

    void retrieveAll();
    std::string retrieveAllToStr();

    const char * beginWriteConst() const;
    char * beginWrite();

    void append(const std::string& str);
    void append(const char * str, size_t len);
    void append(const void * data, size_t len);
    void append(const Buffer& buff);

    ssize_t readFd(int fd, int * err);
    ssize_t  writeFd(int fd, int * err);


private:
    char * beginPtr();
    const char * beginPtr() const;
    void makeSpace(size_t len);
    std::vector<char> buffer;
    // atomic value,  no data conflict
    std::atomic<std::size_t> readPos;
    std::atomic<std::size_t> writePos;
};


#endif
