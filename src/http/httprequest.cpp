//
// Created by yumin_zhang on 2022/4/14.
// 原项目对HTTP request 的处理并不是很好
//

#include "httprequest.h"

std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/video", "/picture", };

std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  };

HttpRequest::HttpRequest() {
    init();
}

void HttpRequest::init() {
    method = "";
    path = "";
    version = "";
    body  = "";
    // initialize state machine
    state = REQUEST_LINE;
    header.clear();
    post.clear();
}

bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.readableBytes() <= 0) {
        return false;
    }
    while(buff.readableBytes() && state != FINISH) {
        const char* lineEnd = std::search(buff.peek(), buff.beginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.peek(), lineEnd);
        // state machine
        switch(state) {
            case REQUEST_LINE:
                if(!parseRequestLine(line)) {
                    return false;
                }
                parsePath();
                break;
            case HEADERS:
                parseHeader(line);
                if(buff.readableBytes() <= 2) {
                    state = FINISH;
                }
                break;
            case BODY:
                parseBody(line);
                break;
            default:
                break;
        }
        if(lineEnd == buff.beginWrite()) break;
        buff.retrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method.c_str(), path.c_str(), version.c_str());
    return true;
}

std::string HttpRequest::getPath() const {
    return path;
}

std::string&  HttpRequest::getPath() {
    return path;
}

std::string HttpRequest::getMethod() const {
    return method;
}

std::string HttpRequest::getVersion() const {
    return version;
}

std::string HttpRequest::getPost(const char *key) {
    assert(key != nullptr);
    std::string k(key);
    if(post.count(k) > 0) {
        return post[k];
    }
    return "";
}

std::string HttpRequest::getPost(const std::string &key) {
    assert(key != "");
    if(post.count(key) > 0) {
        return post[key];
    }
    return "";
}

bool HttpRequest::parseRequestLine(const std::string &line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch match;
    if(std::regex_match(line, match, patten)) {
        method = match[1];
        path = match[2];
        version = match[3];
        state = HEADERS;
        return true;
    }
    LOG_ERROR("Request Format Error");
    return false;
}

void HttpRequest::parseHeader(const std::string &line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch match;
    if(std::regex_match(line, match, patten)) {
        header[match[1]] = match[2];
    } else {
        state = BODY;
    }
}

void HttpRequest::parseBody(const std::string &line) {
    body = line;
    parsePost();
    state = FINISH;
    LOG_DEBUG("Body: %s, len: %d", line.c_str(), line.size());
}
void HttpRequest::parsePath() {
    if(path == "/") {
        path = "/index.html";
    } else {
        for(auto &item : DEFAULT_HTML) {
            if(item == path) {
                path += ".html";
                break;
            }
        }
    }
}

void HttpRequest::parsePost() {

    if(method == "POST" && header.count("Content-Type") > 0 &&
    header["Content-Type"] == "application/x-www-form-urlencoded") {
        // handle post body
        parseFromUrlEncoded();
        if(DEFAULT_HTML_TAG.count(path)) {
            int tag = DEFAULT_HTML_TAG[path];
            LOG_DEBUG("Tag : %d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if(userVerify(post["username"], post["password"], isLogin)) {
                    path = "/welcome.html";
                } else {
                    path = "error.html";
                }
            }
        }
    }
}

void HttpRequest::parseFromUrlEncoded() {
    if(body.size() == 0) {
        return;
    }
    std::string key, value;
    int num = 0;
    int size = body.size();
    int i = 0;
    int j = 0;
    for(; i < size; i++) {
        char ch = body[i];
        switch (ch) {
            case '=':
                key = body.substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                body[i] = ' ';
                break;
             // url Encode  https://www.tutorialspoint.com/html/html_url_encoding.htm
             // 两位16进制
            case '%':
                assert( i + 2 < size);
                num = convertHex(body[i + 1]) * 16 + convertHex(body[i + 2]);
                body[i + 2] = num % 10 + '0';
                body[i + 1] = num / 10 + '0';
                i += 2;;
                break;
                // a=b&c=d situation multi key value pair
            case '&':
                value = body.substr(j, i - j);
                j += 1;
                post[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
        }
    }
    assert(j <= i);
    if(post.count(key) == 0 && j < i) {
        value = body.substr(j, i - j);
        post[key] = value;
    }
}


int HttpRequest::convertHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
}
bool HttpRequest::userVerify(const std::string &name, const std::string &pwd, bool isLogin) {
    if(name == "" || pwd == "") return false;
    LOG_INFO("verify name : %s, pwd : %s", name.c_str(), pwd.c_str());
    MYSQL * connection;
    SqlConnectionRAII(&connection, SqlConnPool::instance());
    assert(connection);
    bool flag = false;
    size_t  j = 0;
    char order[256] = {0};
    MYSQL_FIELD * fields = nullptr;
    MYSQL_RES * res = nullptr;
    if(!isLogin) flag = true;
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1"
    , name.c_str());
    LOG_DEBUG("%s", order)
    if(mysql_query(connection, order)) {
        // success 0 , fail 1
        mysql_free_result(res);
    }
    res = mysql_store_result(connection);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);
    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s, %s", row[0], row[1]);
        std::string password(row[1]);
        if(isLogin){
            if(pwd == password) {flag = true;}
            else {
                flag = false;
                LOG_DEBUG("Password Error");
            }
        } else {
            flag = false;
            LOG_DEBUG("User used");
        }
    }
    mysql_free_result(res);
    if(!isLogin && flag) {
        LOG_DEBUG("Register");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order)
        if(mysql_query(connection, order)) {
            mysql_free_result(res);
            flag = false;
        }
    }
    LOG_DEBUG("User Verify Success!");
    return flag;
}
bool HttpRequest::isKeepAlive()  {
    if(header.count("Connection")) {
        return header["Connection"] == "keep-alive" && version == "1.1";
    }
    return false;
}