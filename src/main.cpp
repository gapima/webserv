/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: glima <glima@student.42sp.org.br>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/18 19:11:12 by glima             #+#    #+#             */
/*   Updated: 2026/02/18 22:10:30 by glima            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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

#define MAX_EVENTS 1024 



struct Client {
    int fd;

    bool operator==(int value) const {
        return fd == value;
    }
};




static int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int createListenSocket(const char* port, const char* host)
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

int main()
{
    
    struct epoll_event ev, events[MAX_EVENTS]; 
    std::vector<Client> clients;
    int server_fd = createListenSocket("8080", "127.0.0.1");
    if (server_fd == -1)
    {
        perror("listen socket");
        return 1;
    }

    //implementando epoll
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1)
    {
        perror("epoll_create");
        close(server_fd);
        return 1;
    }
    
    std::memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
    {
        perror("epoll_ctl ADD server_fd");
        close(epoll_fd);
        close(server_fd);
        return 1;
    }

    std::cout << "Connection client sucess." << std::endl;
    while (true)
    {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 10000);
        if (n == -1)
        {
            perror("epoll_wait");
            break;
        }
        for(int i = 0; i < n; i++)
        {
            struct epoll_event event = events[i];
            
            std::vector<Client>::iterator it = std::find(clients.begin(), clients.end(), event.data.fd);
            
            if (it == clients.end())
            {
                int client_fd = accept(event.data.fd, NULL, NULL);
                if (client_fd == -1)
                    continue;
                
                Client cliente;

                cliente.fd = client_fd;
                
                if (setNonBlocking(client_fd) == -1)
                {
                    close(client_fd);
                    continue;
                }

                epoll_event cev;
                std::memset(&cev, 0, sizeof(cev));
                cev.events = EPOLLIN | EPOLLET;
                cev.data.fd = client_fd;
                
                
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &cev) == -1)
                {
                    close(client_fd);
                    continue;
                }
                
                clients.push_back(cliente);
                for (size_t i = 0; i < clients.size(); i++)
                {
                    std::cout << clients[i].fd << std::endl;
                }
                            
                continue;
            }

        }
    }
    
    return 0;
}
