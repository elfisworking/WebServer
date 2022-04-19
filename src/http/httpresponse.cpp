//
// Created by yumin_zhang on 2022/4/14.
//

#include "httpresponse.h"
std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
        {".html", "text/html"},
        {",xml", "text/xml"},
        {"xhtml", "application/xhtml+xml"},
        {".txt", "text/plain"},
        {".rtf", "application/rtf"},
        {".pdf", "application/pdf"},
        {".word", "application/nsword"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".jpg", "image/jpeg"},
        {".au", "image/basic"},
        {".mpeg", "video/mpeg"},
        {".mpg", "video/mpeg"},
        {".avi", "video/x-msvideo"},
        {".gz", "application/x-gzip"},
        {".tar", "application/x-tar"},
        {".css", "text/css"},
        {".js", "text/javascript"},
};

std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
        {200, "OK"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"},
};
// html related to error code
std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
        {400, "/400.html"},
        {403, "403.html"},
        {404, "404.html"},
};

HttpResponse::HttpResponse() {
    code = -1;
    isKeepAlive = false;
    path = srcDir = "";
    mmFile = nullptr;
    mmFileStat = {0};
}
HttpResponse::~HttpResponse() {
    unmapFile();
}


void HttpResponse::init(const std::string& srcDir_, std::string &path_, bool isKeepAlive_, int initCode) {
    assert(srcDir_ != "");
    assert(path_ != "");
    if(mmFile) unmapFile();
    code = initCode;
    isKeepAlive = isKeepAlive_;
    path = path_;
    srcDir = srcDir_;
    mmFile = nullptr;
    mmFileStat = {0};
}


void HttpResponse::makeResponse(Buffer &buff) {
    //TODO make response
    if(stat((srcDir + path).data(), &mmFileStat) < 0 || S_ISDIR(mmFileStat.st_mode)) {
        code = 404;
    } else if(!(mmFileStat.st_mode & S_IROTH)) {
        code = 403;
    }
    else if(code == -1) {
        code = 200;
    }
    errorHTML();
    addStateLine(buff);
    addHeader(buff);
    addContent(buff);
}

char *HttpResponse::file() {
    return mmFile;
}

size_t HttpResponse::fileLen() const {
    return mmFileStat.st_size;
}

void HttpResponse::addStateLine(Buffer &buff) {
    std::string status;
    if(CODE_STATUS.count(code)) {
        status = CODE_STATUS[code];
    } else {
        code = 400;
        status = CODE_STATUS[code];
    }
    buff.append("HTTP/1.1" + std::to_string(code) + " " + status + "\r\n");
}
// construct response header
void HttpResponse::addHeader(Buffer &buff) {
    buff.append("Connection: ");
    if(isKeepAlive) {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + getFileType() + "\r\n");
}

void HttpResponse::addContent(Buffer &buff) {
    int srcFd = open((srcDir + path).data(), O_RDONLY);
    if(srcFd < 0) {
        errorContent(buff, "File NotFound");
        return;
    }
    LOG_DEBUG("HTTP Response Add Content: file path is %s", (srcDir + path).c_str());
    // struct utsname kernelInfo;
    // int success = uname(&kernelInfo);
    // if(success != 0) {
    //     LOG_WARN("Judge kernel Version failed");
    // }
    // LOG_DEBUG("Current kernel version is %s", kernelInfo.version);
    // // 并不是严谨的判断 sendfile kernel version >= 2.4
    // if(success == 0 && kernelInfo.version[0] >= '2' && kernelInfo.version[2] >= '4') {
    //     // using sendfile function
    //     // TODO sendfile 考虑后续要不要加
    // } else {
        int *mmRet = (int *) mmap(0, mmFileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
        if(*mmRet == -1) {
            errorContent(buff, "File NotFound");
            return;
        }
        mmFile = (char *)mmRet;
        close(srcFd);
        buff.append("Content-length: " + std::to_string(mmFileStat.st_size) + "\r\n\r\n");
    // }

}
void HttpResponse::errorHTML() {
    if(CODE_PATH.count(code)) {
        path = CODE_PATH[code];
        stat((srcDir + path).data(), &mmFileStat);
    }
}

std::string HttpResponse::getFileType() {
    std::string::size_type  idx = path.find_last_of('.');
    if(idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path.substr(idx);
    if(SUFFIX_TYPE.count(suffix)) {
        return SUFFIX_TYPE[suffix];
    }
    return "text/plain";
}

int HttpResponse::getCode() const {
    return code;
}

void HttpResponse::unmapFile() {
    if(mmFile) {
        munmap(mmFile, mmFileStat.st_size);
        mmFile = nullptr;
    }
}

void HttpResponse::errorContent(Buffer &buff, std::string message) {
    std::string body;
    std::string  status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code) == 1) {
        status = CODE_STATUS[code];
    } else {
        status = "Bad Request";
    }
    body += std::to_string(code) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}