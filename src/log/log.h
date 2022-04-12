//
// Created by yumin_zhang on 2022/4/11.
//

#ifndef LOG_H
#define LOG_H
#include <mutex>
#include <string>
#include <sys/time.h>
#include <string.h>
#include <sys/stat.h>
#include "blockqueue.h"
#include "../Buffer/buffer.h"
#include <thread> // thread C++ 11
#include <stdio.h>
#include <stdarg.h>.

// one instance
class Log {
public:
    void init(int lev, const char* pa = "./log",
              const char* fileSuffix = ".log",
              int maxQueueCap = 1024);
    static Log* instance();
    static void flushLogThread();
    void write(int level, const char * format, ...);
    void flush();
    int getLevel();

    void setLevel(int lev);
    bool isOpen();
private:
    Log();
    virtual ~Log();
    void appendLogLevelTitle(int level);
    void asyncWrite();
private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;
    const char * path;
    const char* suffix;
    // int MAX_LINES;
    int lineCount;
    int toDay;
    bool openState;
    Buffer buffer;
    int level;
    bool isAsync;
    FILE* fp;
    std::unique_ptr<BlockQueue<std::string>> deque;
    std::unique_ptr<std::thread> writeThread;
    std::mutex mtx;
};

#define LOG_BASE(level, format, ...) \
    do {\
         Log * log  = Log::instance(); \
         if(log->isOpen() && log->getLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS_);     \
            log->flush();\
         }\
    } while(0)

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...)  do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);
#endif
