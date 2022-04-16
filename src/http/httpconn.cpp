//
// Created by yumin_zhang on 2022/4/14.
//

#include "httpconn.h"
bool HttpConn::isET;
const char* HttpCoon::srcDir;
std::atomic<int> HttpConn::userCount;
HttpConn::HttpConn() {
    fd = -1;
    addr_ = {0};
    isClose = false;
}
HttpConn::~HttpConn() {
    close();
}
void HttpConn::init(int sockFd, const sockaddr_in &addr) {
    assert(sockFd > 0);
    userCount++;;
    addr_ = addr;
    fd = sockFd;
    writeBuff.retrieveAll();
    readBuff.retrieveAll();
    isClose = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::close() {
    reponse.unmapFile();
    if(isClose == false) {
        isClose = true;
        userCount--;
        close(fd);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}
int HttpConn::getFd() const {
    returnf fd;
}
int HttpConn::getPort() const {
    return addr_.sin_port;
}

int HttpConn::getIp() const {
    return inet_ntoa(addr_.sin_addr)
}

sockaddr_in HttpConn::getAddr() const {
    return addr_;
}


ssize_t  HttpConn::read(int* saveErrno) {
    ssize_t  len = -1;
    do {
        readBuff.readFd(fd, saveErrno);
        if(len <= 0) break;
    } while(isET);
    return len;
}

ssize_t HttpConn::write(int *saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd, iov, iovCnt);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov[0].iov_len + iov[1].iov_len == 0) break;
        else if(static_cast<size_t>(len) > iov[1].iov_len) {
            iov[1].iov_base = (uint8_t*)iov[1].iov_base + (len - iov[0].iov_len);
            iov[1].iov_len -= (len - iov[0].iov_len);
            // iov[0] write has finished
            if(iov[0].iov_len) {
                writeBuff.retrieveAll();
                iov[0].iov_len = 0;
            }

        } else {
            iov[0].iov_base = (uint8_t*)iov[0].iov_base + len;
            iov[0].iov_len -= len;
            writeBuff.retrieve(len);
        }
    } while(isET || toWriteBytes() > 10240);
    return len;
}

int HttpConn::toWriteBytes() {
    return iov[0].iov_len + iov[1].iov_len;
}

bool HttpConn::isKeepAlive() {
    return request.isKeepAlive();
}

bool HttpConn::process() {
    request.init();
    if(readBuff.readableBytes() <=0) {
        return false;
    }  else if(request.parse(readBuff)) {
        LOG_DEBUG("Http Connection request %s", request.getPath().c_str());
        reponse.init(srcDir, request.getPath(), request.isKeepAlive(), 200);
    } else {
        reponse.init(srcDir, request.getPath(), false, 400);
    }
    reponse.makeResponse(writeBuff);
    // reponse headfer
    iov[0].iov_base = const_cast<char*>(writeBuff.peek());
    iov[0].iov_len = writeBuff.readableBytes();
    // file
    if(reponse.file() && reponse.fileLen() > 0) {
        iov[1].iov_base = reponse.file();
        iov[1].iov_len = reponse.fileLen();
        iovCnt = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;
}
