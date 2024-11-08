// 1. 阻塞I/O模型示例
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// 阻塞I/O服务器
void blocking_io_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Blocking I/O server listening on port 8080...\n");

    while (1) {
        int client_socket = accept(server_fd, NULL, NULL); // 阻塞直到有连接进来
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        char buffer[1024] = {0};
        read(client_socket, buffer, 1024); // 阻塞读取
        printf("Received: %s\n", buffer);

        char* response = "Hello from server";
        write(client_socket, response, strlen(response));
        close(client_socket);
    }
}

// 2. 非阻塞I/O模型示例
#include <fcntl.h>
#include <errno.h>

void nonblocking_io_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // 设置非阻塞
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8081);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    printf("Non-blocking I/O server listening on port 8081...\n");

    while (1) {
        int client_socket = accept(server_fd, NULL, NULL);
        if (client_socket < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // 没有连接请求，继续循环
                continue;
            }
            perror("Accept failed");
            continue;
        }

        // 设置客户端socket为非阻塞
        flags = fcntl(client_socket, F_GETFL, 0);
        fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

        char buffer[1024] = {0};
        int bytes_read = read(client_socket, buffer, 1024);
        if (bytes_read < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // 数据未准备好，稍后再试
                continue;
            }
            perror("Read failed");
        } else if (bytes_read > 0) {
            printf("Received: %s\n", buffer);
            char* response = "Hello from non-blocking server";
            write(client_socket, response, strlen(response));
        }
        close(client_socket);
    }
}

// 3. I/O多路复用模型示例 (使用epoll)
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

int main() {
    // 选择其中一个服务器运行
    // blocking_io_server();
    // nonblocking_io_server();
    epoll_io_server();
    return 0;
}