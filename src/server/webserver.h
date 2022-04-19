//
// Created by yumin_zhang on 22-4-16.
//

#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "epoller.h"
#include "../log/log.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer {
public:
    WebServer(int port, int triggerMode, int timeout, bool OptLinger, int SqlPort, const char * sqlUser,
               const char * sqlPwd, const char * dbName, int connPoolNum, int thredNum, bool openLog, int LogLevel,
               int logQueueSize);
    ~WebServer();
    void start();
private:
    bool initSocket();
    void initEventMode(int triggerMode);
    void addClient(int fd, sockaddr_in addr);
    void dealListen();
    void dealWrite(HttpConn * client);
    void dealRead(HttpConn * client);
    void sendError(int fd, char * info);
    void extentTime(HttpConn * client);
    void closeConnection(HttpConn * client);
    void onRead(HttpConn * client);
    void onWrite(HttpConn * client);
    void onProcess(HttpConn * client);
    static const int MAX_FD = 65535;
    static  int setFdNonBlock(int fd);
    int port_;
    bool openLinger_;
    int timeout_;
    bool isClose;
    int listenFd_;
    char * srcDir_;
    uint32_t  listenEvent_;
    uint32_t  connectionEvent_;
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;


};


#endif //MYWEBSERVER_WEBSERVER_H
