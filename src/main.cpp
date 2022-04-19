#include <unistd.h>
#include "server/webserver.h"

int main() {
    WebServer server(1316, 3, 60000, false, 
                    3306, "root", "123456", "webserver", 
                    2, 1, true, 0, 0);
    server.start();
    return 0;
}