//
// Created by yumin_zhang on 2022/4/14.
//

#ifndef HTTPCONN_H
#define HTTPCONN_H
#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../Buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();
    void init(int sockFd, const sockaddr_in& addr);
    ssize_t read(int *saveErrno);
    ssize_t  write(int *saveErrno);
    void close_();
    int getFd() const;
    int getPort() const;
    const char * getIp() const;
    sockaddr_in getAddr() const;
    bool process();
    int toWriteBytes();
    bool isKeepAlive();
    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;
private:
    int fd;
    struct sockaddr_in addr_;
    bool isClose;
    int iovCnt;
    struct iovec iov[2];
    Buffer readBuff;
    Buffer writeBuff;
    HttpRequest request;
    HttpResponse reponse;
};


#endif //HTTPCONN_H
