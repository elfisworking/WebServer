//
// Created by yumin_zhang on 2022/4/14.
//

#include "httpconn.h"
bool HttpConn::isET;
const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
HttpConn::HttpConn() {
    fd = -1;
    addr_ = {0};
    isClose = false;
}
HttpConn::~HttpConn() {
    close_();
}
void HttpConn::init(int sockFd, const sockaddr_in &addr) {
    assert(sockFd > 0);
    userCount++;;
    addr_ = addr;
    fd = sockFd;
    writeBuff.retrieveAll();
    readBuff.retrieveAll();
    isClose = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd, getIp(), getPort(), (int)userCount);
}

void HttpConn::close_() {
    reponse.unmapFile();
    if(isClose == false) {
        isClose = true;
        userCount--;
        close(fd);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd, getIp(), getPort(), (int)userCount);
    }
}
int HttpConn::getFd() const {
    return fd;
}
int HttpConn::getPort() const {
    return addr_.sin_port;
}

const char * HttpConn::getIp() const {
    return inet_ntoa(addr_.sin_addr);
}

sockaddr_in HttpConn::getAddr() const {
    return addr_;
}


ssize_t  HttpConn::read(int* saveErrno) {
    LOG_DEBUG("Httpconn.cpp 54 line read() function is doing! start ");
    ssize_t  len = -1;
    do {
        len = readBuff.readFd(fd, saveErrno);
        
        if(len <= 0) break;
    } while(isET);
    LOG_DEBUG("Httpconn.cpp 54 line read() function is doing! end ");
    return len;
}

ssize_t HttpConn::write(int *saveErrno) {
    ssize_t len = -1;
    LOG_DEBUG("Http connection ET mode is %d", isET);
    do {
        len = writev(fd, iov, iovCnt);
        LOG_DEBUG("write length is %d", len);
        if(len <= 0) {
            LOG_DEBUG("Http connection write end!");
            *saveErrno = errno;
            break;
        }
        if(iov[0].iov_len + iov[1].iov_len == 0) break;
        else if(static_cast<size_t>(len) > iov[0].iov_len) {
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
    LOG_DEBUG("httpconn.cpp 103 line request init");
    if(readBuff.readableBytes() <=0) {
        return false;
    }  else if(request.parse(readBuff)) {
        LOG_DEBUG("Http Connection request %s", request.getPath().c_str());
        reponse.init(srcDir, request.getPath(), request.isKeepAlive(), 200);
    } else {
        reponse.init(srcDir, request.getPath(), false, 400);
    }
    reponse.makeResponse(writeBuff);
    // response header
    iov[0].iov_base = const_cast<char*>(writeBuff.peek());
    iov[0].iov_len = writeBuff.readableBytes();
    iovCnt = 1;
    // file mmap
    if(reponse.file() && reponse.fileLen() > 0) {
        iov[1].iov_base = reponse.file();
        iov[1].iov_len = reponse.fileLen();
        iovCnt = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", reponse.fileLen(), iovCnt, toWriteBytes());
    return true;
}
