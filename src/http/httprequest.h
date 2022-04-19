//
// Created by yumin_zhang on 2022/4/14.
//

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <algorithm>
#include <errno.h>
#include <mysql/mysql.h>

#include "../Buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../pool/sqlconnpool.h"
class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    HttpRequest();
    ~HttpRequest() = default;
    void init();
    bool parse(Buffer& buffer);

    std::string  getPath() const;
    std::string & getPath();
    std::string getMethod() const;
    std::string getVersion() const;
    std::string getPost(const std::string& key) ;
    std::string getPost(const char * key) ;
    bool isKeepAlive() ;
private:
    bool parseRequestLine(const std::string& line);
    void parseHeader(const std::string& line);
    void parseBody(const std::string& line);
    void parsePath();
    void parsePost();
    void parseFromUrlEncoded();
    static bool userVerify(const std::string& name, const std::string& pwd, bool isLogin);
    PARSE_STATE state;
    std::string method, path, version, body;
    std::unordered_map<std::string, std::string> header;
    std::unordered_map<std::string, std::string> post;
    static std::unordered_set<std::string> DEFAULT_HTML;
    static std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int convertHex(char ch);
};


#endif
