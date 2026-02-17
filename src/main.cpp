/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: glima <glima@student.42sp.org.br>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 15:59:49 by glima             #+#    #+#             */
/*   Updated: 2026/02/17 17:31:39 by glima            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/socket.h>
#include <iostream>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

int main()
{

    struct addrinfo hints;
    struct addrinfo *res = NULL;
    
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    int status = getaddrinfo("127.0.0.1", "8080", &hints, &res);

    if (status != 0 || res == NULL)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        freeaddrinfo(res);
        exit(1);
    }
    
    int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(server_fd == -1)
    {
        std::cerr << "socket: " << gai_strerror(server_fd) << std::endl;
        freeaddrinfo(res);
        exit(1);
    }
    
    int bi = bind(server_fd, res->ai_addr, res->ai_addrlen);
    if (bi != 0)
    {
        std::cerr << "bind: " << gai_strerror(bi) << std::endl;
        freeaddrinfo(res);
        close(server_fd);
        exit(1);
    }
    
    
    //std::cout << "Valor do bind: " << bi << std::endl;

    
    if(listen(server_fd, SOMAXCONN))
    {
        perror("listen");
        close(server_fd);
        exit(1);
    }
    
    while (1)
    {
        int accept_fd = accept(server_fd, res->ai_addr, &res->ai_addrlen);
        char buffer[1024];
        recv(accept_fd, buffer, sizeof(buffer), 0);
        std::cout << "buffer: " << buffer << std::endl;
        std::cout << "Cliente Online: " << accept_fd << std::endl;
    }
    freeaddrinfo(res);
    

    return 0;
}