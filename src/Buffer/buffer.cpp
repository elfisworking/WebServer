//
// Created by yumin_zhang on 2022/4/11.
//
// please look buffer.png
#include "buffer.h"
Buffer::Buffer(int initSize): buffer(initSize), readPos(0), writePos(0){}

// can read bytes length
size_t Buffer::writableBytes() const {
    return buffer.size() - writePos;
}

size_t Buffer::readableBytes() const {
    return writePos - readPos;
}
size_t Buffer::preparableBytes() const {
    return readPos;
}

const char * Buffer::peek() const {
    // return read beginning address
    return beginPtr() + readPos;
}
// can write ?
void Buffer::ensureWritable(size_t len) {
    printf("writable bytes is %u, len is %u\n", writableBytes(), len);
    if(writableBytes() < len) {
        // if space is not enough, append space
        makeSpace(len);
    }
    
    assert(writableBytes() >= len);
}

// has written, modify write position
void Buffer::hasWritten(size_t len) {
    writePos += len;
}
// buffer read some data
void Buffer::retrieve(size_t len) {
    assert(readableBytes() >= len);
    readPos += len;
}

void Buffer::retrieveUntil(const char * end) {
    assert(peek() <= end);
    retrieve(end - peek());
}
// clear all
void Buffer::retrieveAll() {
    bzero(&buffer[0], buffer.size());
    readPos = 0;
    writePos = 0;
}

std::string Buffer::retrieveAllToStr() {
    std::string str(peek(), writableBytes());
    retrieveAll();
    return str;
}

const char * Buffer::beginWriteConst() const {
    return beginPtr() + writePos;
}

// return begin write address
char * Buffer::beginWrite() {
    return beginPtr() + writePos;
}


char * Buffer::beginPtr() {
    // iter value address
    return &*buffer.begin();
}

const char * Buffer::beginPtr() const {
    return &*buffer.begin();
}


void Buffer::append(const std::string &str) {
    append(str.data(), str.size());
}

void Buffer::append(const char *str, size_t len) {
    assert(str);
    printf("append(const %s), size_t %d\n", str, len);
    ensureWritable(len);
    std::copy(str, str + len, beginWrite());
    hasWritten(len);
}

void Buffer::append(const void *data, size_t len) {
    // static_cast
    const char * str = static_cast<const char *>(data);
    append(str, len);
}

void Buffer::append(const Buffer &buff) {
    append(buff.peek(), buff.readableBytes());
}

ssize_t Buffer::readFd(int fd, int * err) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writableLength = writableBytes();
    iov[0].iov_base = beginPtr() + writePos;
    iov[0].iov_len = writableLength;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);
    ssize_t len = readv(fd, iov, 2);
    // handle three situation
    if(len < 0) {
        *err = errno;
    } else if(static_cast<size_t>(len) <= writableLength) {
        writePos += len;
    } else {
        writePos = buffer.size();
        append(buff, len - writableLength);
    }
    return len;
}

ssize_t  Buffer::writeFd(int fd, int * err) {
    ssize_t  readableLength = readableBytes();
    ssize_t  len = write(fd, peek(), readableLength);
    if(len < 0) {
        *err = errno;
    } else {
        writePos += len;
    }
    return len;
}

void Buffer::makeSpace(size_t len) {
    if(writableBytes() + preparableBytes() < len) {
        // space is not enough, expand sapce
        buffer.resize(writePos + len + 1);
    } else {
        // space is enough, move read part to buffer beginning
        size_t readLength = readableBytes();
        std::copy(beginPtr() + readPos, beginPtr() + writePos, beginPtr());
        readPos = 0;
        writePos = readPos + readLength;
        assert(readLength == readableBytes());
    }
}