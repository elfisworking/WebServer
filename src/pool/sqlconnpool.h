//
// Created by yumin_zhang on 2022/4/14.
//

#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H
#include <mysql/mysql.h>
#include <semaphore.h>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include "../log/log.h"
// singleton
class SqlConnPool {
public:
    static SqlConnPool * instance();
    MYSQL * getConnection();
    void freeConnection(MYSQL * connection);
    int getFreeConnectionCount();
    void init(const char * host, int port, const char * user, const char * password,
              const char * dbName, int connectionSize);
    void closePool();
private:
    SqlConnPool();
    ~SqlConnPool();
    int maxConnection;
    int useCount;
    int freeCount;
    std::queue<MYSQL *> connectionQueue;
    std::mutex mtx;
    sem_t semId;
};


#endif //MYWEBSERVER_SQLCONNPOOL_H
