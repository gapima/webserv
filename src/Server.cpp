#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <algorithm>
#include <Server.hpp>

int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int createListenSocket(const char* host, const char* port)
{
    addrinfo hints;
    addrinfo *res = NULL;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE;      

    int st = getaddrinfo(host, port, &hints, &res);
    if (st != 0 || !res)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(st) << std::endl;
        freeaddrinfo(res);
        return -1;
    }

    int server_fd = -1;
    for (addrinfo *p = res; p; p = p->ai_next)
    {
        server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server_fd == -1) continue;

        //int opt = 1;
        //setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(server_fd, p->ai_addr, p->ai_addrlen) == 0)
            break;

        close(server_fd);
        server_fd = -1;
    }

    freeaddrinfo(res);

    if (server_fd == -1) return -1;
    if (setNonBlocking(server_fd) == -1) { close(server_fd); return -1; }
    if (listen(server_fd, SOMAXCONN) == -1) { close(server_fd); return -1; }

    return server_fd;
}

int CreateServer(const std::char* host, const std::char* port){
    return createListenSocket(host, port);

}
