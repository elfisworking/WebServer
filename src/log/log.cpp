//
// Created by yumin_zhang on 2022/4/11.
//

#include "log.h"

Log::Log():lineCount(0), isAsync(false), writeThread(nullptr), toDay(0), fp(nullptr), deque(nullptr){}

Log::~Log() {
    if(writeThread && writeThread->joinable()) {
        while(!deque->empty()) {
            deque->flush();
        }
        deque->close();
        // wait write thread die
        writeThread->join();
    }
    if(fp) {
        std::lock_guard<std::mutex> locker(mtx);
        flush();
        fclose(fp);
    }
}

void Log::appendLogLevelTitle(int level) {
    switch(level) {
        case  0:
            buffer.append("[DEBUG]: ", 9);
            break;
        case 1:
            buffer.append("[INFO] : ", 9);
            break;
        case 2:
            buffer.append("[WARN] : ", 9);
            break;
        case 3:
            buffer.append("[ERROR]: ", 9);
            break;
        default:
            buffer.append("[INFO] : ", 9);
    }

}


void Log::init(int lev = 1, const char *filePath, const char *fileSuffix, int maxQueueCap) {
    openState = true;
    level = lev;
    if(maxQueueCap > 0) {
        isAsync = true;
        if(deque == nullptr) {
            deque = std::make_unique<BlockQueue<std::string>>();
            writeThread = std::make_unique<std::thread>(flushLogThread);
            // make_unique C++14 make_shared C++11
        }
    } else {
        isAsync = false;
    }
    lineCount = 0;
    time_t timer = time(nullptr);
    tm * t = localtime(&timer);
    this->path = filePath;
    this->suffix = fileSuffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN, "%s/%04d_%02d_%02d%s",
             path, t->tm_year + 1900 , t->tm_mon + 1, t->tm_mday, this->suffix);
    toDay = t->tm_mday;
    {
        std::lock_guard<std::mutex> locker(mtx);
        buffer.retrieveAll();
        if(fp) {
            flush();
            fclose(fp);
        }
        fp = fopen(fileName, "a");
        if(fp == nullptr) {
            mkdir(this->path, 0777);
            fp = fopen(fileName, "a");
        }
        assert(fp != nullptr);
    }


}

Log *Log::instance() {
    static Log log;
    return &log;
}

void Log::asyncWrite() {
    std::string str = "";
    while(deque->pop(str)) {
        std::lock_guard<std::mutex> locker(mtx);
        fputs(str.c_str(), fp);
    }
}

void Log::flushLogThread() {
    Log::instance()->asyncWrite();
}

int Log::getLevel() {
    std::lock_guard<std::mutex> locker(mtx);
    return level;
}

void Log::setLevel(int lev) {
    std::lock_guard<std::mutex> locker(mtx);
    level = lev;
}

bool Log::isOpen() {
    std::lock_guard<std::mutex> locker(mtx);
    return openState;
}


void Log::write(int level, const char *format, ...) {
    // get time
    timeval valTime = {0, 0};
    gettimeofday(&valTime, nullptr);
    __time_t sec = valTime.tv_sec;
    tm *locTime = localtime(&sec);
    va_list vaList;
    if(toDay != locTime->tm_mday || (lineCount  && (lineCount % MAX_LINES == 0))) {
        std::unique_lock<std::mutex> locker(mtx);
        locker.unlock();
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%05d_%02d_%02d", locTime->tm_year + 1900,
                 locTime->tm_mon + 1, locTime->tm_mday);
        if(toDay != locTime->tm_mday) {
            // a new day that create a new log file
            // why minus 72
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path, tail, suffix);
            // update toDay
            toDay = locTime->tm_mday;
            lineCount = 0;
        } else {
            // in one day, log number over MAX_LINES
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path, tail, (lineCount / MAX_LINES),
                     suffix);
        }
        locker.lock();
        flush();
        if(fp != nullptr) {
            fclose(fp);
        }
        fp = fopen(newFile, "a");
        assert(fp != nullptr);
    }
    // log pattern: time + level + content
    {
        std::unique_lock<std::mutex> locker(mtx);
        lineCount++;
        int len = snprintf(buffer.beginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                 locTime->tm_year + 1900, locTime->tm_mon + 1, locTime->tm_mday,
                 locTime->tm_hour, locTime->tm_min, locTime->tm_sec, valTime.tv_usec);

        buffer.hasWritten(len);
        appendLogLevelTitle(level);
        va_start(vaList, format);
        int m = vsnprintf(buffer.beginWrite(), buffer.writableBytes(), format, vaList);
        va_end(vaList);
        buffer.hasWritten(m);
        buffer.append("\n\0", 2);
        if(isAsync && deque && !deque->full()) {
            deque->push_back(buffer.retrieveAllToStr());
        } else {
            fputs(buffer.peek(), fp);
        }
        // clear buffer
        buffer.retrieveAll();
    }
}

void Log::flush() {
    if(isAsync) {
        deque->flush();
    }
    fflush(fp);
}

