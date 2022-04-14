//
// Created by yumin_zhang on 2022/4/14.
//

#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H
#include "sqlconnpool.h"
class SqlConnectionRAII {
public:
    SqlConnectionRAII(MYSQL ** sql, SqlConnPool * connPool);
    ~SqlConnectionRAII();
private:
    MYSQL * connection;
    SqlConnPool * pool;
};



#endif
