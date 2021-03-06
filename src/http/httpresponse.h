//
// Created by yumin_zhang on 2022/4/14.
//

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "../Buffer/buffer.h"
#include "../log/log.h"
#include <sys/utsname.h> // use for judge linux kernel version
#include <sys/sendfile.h> // sendfile function for zero copy

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();
    void init(const std::string& srcDir_, std::string& path_, bool isKeepAlive_ = false, int initCode = -1);
    void makeResponse(Buffer& buff);
    void unmapFile();
    char * file();
    size_t fileLen() const;
    void errorContent(Buffer& buff, std::string message);
    int getCode() const;
private:
    void addStateLine(Buffer& buff);
    void addHeader(Buffer& buff);
    void addContent(Buffer& buff);
    void errorHTML();
    std::string getFileType();
    int code;
    bool isKeepAlive;
    std::string path;
    std::string srcDir;
    // zero copy
    char * mmFile;
    struct stat mmFileStat;

    static std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static std::unordered_map<int, std::string> CODE_STATUS;
    static std::unordered_map<int, std::string> CODE_PATH;
};


#endif
