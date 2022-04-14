//
// Created by yumin_zhang on 2022/4/14.
//
#include "sqlconnRAII.h"
SqlConnectionRAII::SqlConnectionRAII(MYSQL **sql, SqlConnPool *connPool) {
    assert(sql && connPool);
    *sql = connPool->getConnection();
    pool = connPool;
    connection = *sql;
}

SqlConnectionRAII::~SqlConnectionRAII() {
    if(connection != nullptr) {
        pool->freeConnection(connection);
    }
}