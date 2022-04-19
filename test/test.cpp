#include "../src/log/log.h"
#include "../src/pool/threadpool.h"
#include <features.h>

#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30:
#include <sys/syscall.h>
#define gettid() syscall(SYS_gitted)
#endif

void TestLog() {
    int cnt = 0;
    int level = 0;
    Log::instance()->init(level, "./testlog", ".log", 0);
    // 同步
    for(level = 3; level >= 0; level--) {
        Log::instance()->setLevel(level);
        for(int j = 0; j < 100000; j++) {
            for(int i = 0; i < 4; i++) {
                LOG_BASE(level, "Test ======= %d =======", cnt++);
            }
        }
    }
    cnt = 0;
    Log::instance()->init(level, "./testlog2", ".log", 0);
    for(level = 0; level < 4; level++) {
        Log::instance()->setLevel(level);
        for(int j = 0; j < 10000; j++) {
            for(int i = 0; i < 4; i++) {
                LOG_BASE(level, "Test2 ======== %d ========", cnt++);
            }
        }
    }
}


void ThreadLogTask(int i, int cnt) {
    for(int j = 0; j < 10000; j++) {
        LOG_BASE(i, "PID[%04d] ========= %05d =======",cnt++);
    }
}

void TestThreadPool() {
    Log::instance()->init(0, "./testThreadPool", ".log", 5000);
    ThreadPool threadpool(6);
    for(int i = 0; i < 18; i++) {
        threadpool.addTask(std::bind(ThreadLogTask, i % 4, i * 10000));
    }
}

int main() {
    TestLog();
    TestThreadPool();
    return 0;
}