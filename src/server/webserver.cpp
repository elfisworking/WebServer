//
// Created by yumin_zhang on 22-4-16.
//

#include "webserver.h"
WebServer::WebServer(int port, int triggerMode, int timeout, bool optLinger, int sqlPort, const char *sqlUser,
                     const char *sqlPwd, const char *dbName, int connPoolNum, int threadNum, bool openLog, int logLevel,
                     int logQueueSize) : port_(port), timeout_(timeout), openLinger_(optLinger), isClose(false),
                     timer_(new HeapTimer), threadPool_(new ThreadPool(threadNum)), epoller_(new Epoller){
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;
    SqlConnPool::instance()->init("127.0.0.1", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);
    initEventMode(triggerMode);
    if(!initSocket()) isClose = true;
    if(openLog) {
        Log::instance()->init(logLevel, "./log", ".log", logQueueSize);
        if(isClose) {
            LOG_ERROR("======================Server Init Error=======================");
        } else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, optLinger ? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                     (listenEvent_ & EPOLLET ? "ET": "LT"),
                     (connectionEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose = true;
    free(srcDir_);
    SqlConnPool::instance()->closePool();
    Log::instance()->flush();
}

void WebServer::initEventMode(int triggerMode) {
    listenEvent_ = EPOLLRDHUP;
    connectionEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (triggerMode) {
        case 0:
            break;
        case 1:
            connectionEvent_ |= EPOLLET;
            break;
        case 2:
            listenEvent_ |= EPOLLET;
            break;
        case 3:
            listenEvent_ |= EPOLLET;
            connectionEvent_ |= EPOLLET;
            break;
        default:
            listenEvent_ |= EPOLLET;
            connectionEvent_ |= EPOLLET;
            break;
    }
    HttpConn::isET = (connectionEvent_ & EPOLLET);
}

void WebServer::start() {
    int time = -1; // 如果设置为-1，那么事件将是无阻塞的
    if(!isClose) {
        LOG_INFO("======================start=======================");
    }
    while(!isClose) {
        if(timeout_ > 0) {
            time = timer_->getNextTick();
        }
        int eventCnt = epoller_->wait(time);
        for(int i = 0; i < eventCnt; i++) {
            // deal with event
            int fd = epoller_->getEventFd(i);
            uint32_t  events = epoller_->getEvents(i);
            if(fd == listenFd_) {
                // deal with listen fd
                dealListen();
            } else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                printf("events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)");
                closeConnection(&users_[fd]);
            } else  if(events & EPOLLIN) {
                LOG_DEBUG("events epollin happen");
                assert(users_.count(fd) > 0);
                dealRead(&users_[fd]);
            } else if(events & EPOLLOUT) {
                LOG_DEBUG("events epollOut happen");
                assert(users_.count(fd) > 0);
                dealWrite(&users_[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}


void WebServer::sendError(int fd, char * info) {
    assert(users_.count(fd) > 0);
    int len = send(fd, info, strlen(info), 0);
    if(len < 0) {
        LOG_WARN("send error to clien[%d] error", fd);
    }
    close(fd);
}

void WebServer::closeConnection(HttpConn *client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->getFd());
    epoller_->delFd(client->getFd());
    client->close_();
}

void WebServer::addClient(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if(timeout_ > 0) {
        // 超时自动关闭连接
        timer_->add(fd, timeout_, std::bind(&WebServer::closeConnection, this, &users_[fd]));
    }
    epoller_->addFd(fd, EPOLLIN | connectionEvent_);
    // 如果是ET， 那么fd是非阻塞的
    setFdNonBlock(fd);
    LOG_INFO("Client[%d] in", fd);
}

void WebServer::dealListen() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd < 0) {
            return;
        } else if(HttpConn::userCount >= MAX_FD) {
            sendError(fd, "Server Busy");
            LOG_WARN("Clients is full!");
            return ;
        }
        addClient(fd, addr);
    }while(listenEvent_ & EPOLLET); // TODO 为什么这样写
}

void WebServer::dealRead(HttpConn *client) {
    assert(client);
    extentTime(client);
    threadPool_->addTask(std::bind(&WebServer::onRead, this, client));
}

void WebServer::dealWrite(HttpConn *client) {
    assert(client);
    extentTime(client);
    threadPool_->addTask(std::bind(&WebServer::onWrite, this, client));
}
void WebServer::extentTime(HttpConn *client) {
    assert(client);
    if(timeout_ > 0) {
        timer_->adjust(client->getFd(), timeout_);
    }
}

void WebServer::onRead(HttpConn *client) {
    assert(client);
    int len = -1;
    int readErrno = 0;
    // read data to Buffer
    len = client->read(&readErrno);
    if(len <= 0 && readErrno != EAGAIN) {
        printf("web server on read fail, len is %d, errno is %d", len, readErrno);
        closeConnection(client);
        return;
    }
    // change EPOLLIN EPOLLOUT state
    onProcess(client);
}
void WebServer::onProcess(HttpConn *client) {
    assert(client != nullptr);
    // judge have something to write. IF true , EPOLLOUT else EOPLLIN
    if(client->process()) {
        epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLOUT);
    } else {
        epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLIN);
    }
}

void WebServer::onWrite(HttpConn *client) {
    assert(client);
    int len = -1;
    int writeErrno = 0;
    int ret = client->write(&writeErrno);
    LOG_DEBUG("web server client->write ret is %d", ret);
    LOG_DEBUG("web server client->write toWriteBytes() is %d", client->toWriteBytes());
    // TODO 这边做了修改 会导致错误
    if(client->toWriteBytes() == 0) {
        // 传输完成
        if(client->isKeepAlive()) {
            // is a long connection
            // change event
            LOG_DEBUG("202 line after client->isKeepAlive()");
            onProcess(client);
            return;
        }
    } else if (ret < 0) {
        if(writeErrno == EAGAIN) {
            epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLOUT);
            return;
        }
    }
    closeConnection(client);
}

bool WebServer::initSocket() {
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Bad Port, port %d error!", port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    struct linger optLinger = {0};
    if(openLinger_) {
        // 优雅关闭，知道所剩数据发送完毕或者超时
        optLinger.l_linger = 1;
        optLinger.l_onoff = 1;
    }
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!");
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        LOG_ERROR("Init Linger error!");
        return false;
    }
    int optVal = 1;
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optVal, sizeof(int));
    if(ret < 0) {
        close(listenFd_);
        LOG_ERROR("Faled to set socket port reuse ");
        return false;
    }
    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        close(listenFd_);
        LOG_ERROR("Error Bind");
        return false;
    }
    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen Error");
        close(listenFd_);
        return false;
    }
    ret = epoller_->addFd(listenFd_, listenEvent_ | EPOLLIN);
    if(!ret) {
        close(listenFd_);
        LOG_ERROR("EPOLL LISTEN ERROR!");
        return false;
    }
    setFdNonBlock(listenFd_);
    LOG_INFO("Server Port: %d", port_);
    return true;
}

int WebServer::setFdNonBlock(int fd) {
    assert(fd > 0);
    int res = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    printf("set Fd non block res is %d\n", res);
    return res;
}