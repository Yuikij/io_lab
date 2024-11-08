
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10

void epoll_io_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8082);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    // 创建epoll实例
    int epoll_fd = epoll_create1(0);
    struct epoll_event event, events[MAX_EVENTS];
    
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    printf("Epoll I/O server listening on port 8082...\n");

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        for (int n = 0; n < nfds; n++) {
            if (events[n].data.fd == server_fd) {
                // 新连接
                int client_socket = accept(server_fd, NULL, NULL);
                event.events = EPOLLIN;
                event.data.fd = client_socket;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
            } else {
                // 已存在的连接有数据可读
                char buffer[1024] = {0};
                int bytes_read = read(events[n].data.fd, buffer, 1024);
                
                if (bytes_read <= 0) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, NULL);
                    close(events[n].data.fd);
                    continue;
                }
                
                printf("Received: %s\n", buffer);
                char* response = "Hello from epoll server";
                write(events[n].data.fd, response, strlen(response));
            }
        }
    }
}