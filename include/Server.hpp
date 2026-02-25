#include <iostream>
#include <string>

class CreateServer {
public:
    CreateServer(const std::char* host, const std::char* port) : host(host), port(port) {}
    ~CreateServer();
private:
    std::string host;
    std::string port;
};
    