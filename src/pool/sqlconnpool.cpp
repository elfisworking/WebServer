//
// Created by yumin_zhang on 2022/4/14.
//

#include "sqlconnpool.h"
SqlConnPool::SqlConnPool() {
    useCount = 0;
    freeCount = 0;
}

SqlConnPool * SqlConnPool::instance() {
    static SqlConnPool instance;
    return &instance;
}

SqlConnPool::~SqlConnPool() {
    closePool();
}

void SqlConnPool::init(const char *host, int port, const char *user, const char *password, const char *dbName,
                       int connectionSize = 10) {
    // assert(host && port && user && password && dbName && connectionSize > 0);
    for(int i= 0; i < connectionSize; i++) {
        auto sqlConn = mysql_init(nullptr);
        if(sqlConn == nullptr) {
            LOG_ERROR("Not enough memory!")
            assert(sqlConn);
        }
        MYSQL *connect = mysql_real_connect(sqlConn, host, user, password, dbName, port, nullptr, 0);
        if(connect == nullptr) {
            LOG_ERROR("Failed to connect to databse: Error: %s", mysql_error(sqlConn));
            assert(connect);
        }
        LOG_INFO("Mysql connection Successful!");
        connectionQueue.push(connect);
    }
    maxConnection = connectionSize;
    sem_init(&semId, 0, maxConnection);
}

void SqlConnPool::closePool() {
    std::lock_guard<std::mutex> locker(mtx);
    while(!connectionQueue.empty()) {
        auto conn = connectionQueue.front();
        connectionQueue.pop();
        mysql_close(conn);
    }
    mysql_library_end();
}

int SqlConnPool::getFreeConnectionCount() {
    std::lock_guard<std::mutex> locker(mtx);
    return connectionQueue.size();
}

MYSQL *SqlConnPool::getConnection() {
//    assert(!connectionQueue.empty());
    if(connectionQueue.empty()) {
        LOG_WARN("Mysql Connection Busy!");
        // if busy, return null pointer
        return nullptr;
    }
    // C++ queue is not thread saf
    MYSQL * conn = nullptr;
    sem_wait(&semId);
    {
        std::lock_guard<std::mutex> locker(mtx);
         conn = connectionQueue.front();
        connectionQueue.pop();
    }
    return conn;
}

void SqlConnPool::freeConnection(MYSQL *connection) {
    assert(connection);
    std::lock_guard<std::mutex> locker(mtx);
    connectionQueue.push(connection);
    sem_post(&semId);
}
