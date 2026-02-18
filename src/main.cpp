/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: glima <glima@student.42sp.org.br>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/18 19:11:12 by glima             #+#    #+#             */
/*   Updated: 2026/02/18 19:36:52 by glima            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <map>
#include <string>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>

static int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int createListenSocket(const char* port)
{
    addrinfo hints;
    addrinfo *res = NULL;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE;      

    int st = getaddrinfo(NULL, port, &hints, &res);
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

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

int main()
{
    int server_fd = createListenSocket("8080");
    if (server_fd == -1)
    {
        perror("listen socket");
        return 1;
    }

    int epfd = epoll_create(1);
    if (epfd == -1)
    {
        perror("epoll_create");
        close(server_fd);
        return 1;
    }

    epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;      
    ev.data.fd = server_fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
    {
        perror("epoll_ctl ADD server_fd");
        close(epfd);
        close(server_fd);
        return 1;
    }

    std::map<int, std::string> inBuf;
    std::map<int, std::string> outBuf;

    epoll_event events[64];

    std::cout << "Listening on 8080 (epoll)..." << std::endl;

    while (true)
    {
        int n = epoll_wait(epfd, events, 64, -1);
        if (n == -1)
        {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;

            if (fd == server_fd)
            {
                sockaddr_storage client_addr;
                socklen_t client_len = sizeof(client_addr);

                int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
                if (client_fd == -1)
                    continue; 

                if (setNonBlocking(client_fd) == -1)
                {
                    close(client_fd);
                    continue;
                }

                epoll_event cev;
                std::memset(&cev, 0, sizeof(cev));
                cev.events = EPOLLIN;
                cev.data.fd = client_fd;

                if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev) == -1)
                {
                    close(client_fd);
                    continue;
                }

                inBuf[client_fd] = "";
                outBuf[client_fd] = "";

                std::cout << "Client connected fd=" << client_fd << std::endl;
            }
            else
            {
                if (events[i].events & (EPOLLHUP | EPOLLERR))
                {
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    inBuf.erase(fd);
                    outBuf.erase(fd);
                    continue;
                }

                if (events[i].events & EPOLLIN)
                {
                    char buf[1024];
                    int r = recv(fd, buf, sizeof(buf), 0);
                    if (r <= 0)
                    {
                        close(fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                        inBuf.erase(fd);
                        outBuf.erase(fd);
                        continue;
                    }
                    inBuf[fd].append(buf, r);

                    std::cout << "fd=" << fd << " received " << r << " bytes" << std::endl;
                }
            }
        }
    }

    close(epfd);
    close(server_fd);
    return 0;
}
