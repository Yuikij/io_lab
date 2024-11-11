#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>

#define MAX_EVENTS 10
#define FIFO_COUNT 2
#define BUFFER_SIZE 1024

// FIFO结构体
typedef struct {
    char *path;           // FIFO路径
    int fd;              // 文件描述符
    int writer_existed;   // 是否曾经有写入端
} FifoInfo;

void print_timestamp(const char* prefix) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", localtime(&tv.tv_sec));
    printf("%s: %s.%06ld\n", prefix, timestamp, tv.tv_usec);
}

// 设置非阻塞模式
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        exit(EXIT_FAILURE);
    }
}

// 创建和初始化FIFO
void init_fifo(FifoInfo *fifo) {
    unlink(fifo->path);
    if (mkfifo(fifo->path, 0666) == -1) {
        perror("mkfifo failed");
        exit(EXIT_FAILURE);
    }
    
    // 以非阻塞模式打开FIFO
    fifo->fd = open(fifo->path, O_RDONLY | O_NONBLOCK);
    if (fifo->fd == -1) {
        perror("open fifo failed");
        unlink(fifo->path);
        exit(EXIT_FAILURE);
    }
    set_nonblocking(fifo->fd);
    fifo->writer_existed = 0;
    
    printf("FIFO created: %s\n", fifo->path);
}

// 处理FIFO的读取事件
void handle_fifo_read(FifoInfo *fifo) {
    char buffer[BUFFER_SIZE];
    print_timestamp(fifo->path);
    
    ssize_t bytes = read(fifo->fd, buffer, sizeof(buffer) - 1);
    
    if (bytes > 0) {
        // 成功读取数据
        buffer[bytes] = '\0';
        printf("%s: Read %zd bytes: %s\n", fifo->path, bytes, buffer);
        fifo->writer_existed = 1;
    } else if (bytes == 0) {
        if (fifo->writer_existed) {
            printf("%s: All writers closed\n", fifo->path);
        } else {
            printf("%s: No writer has opened yet\n", fifo->path);
        }
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("%s: No data available\n", fifo->path);
        } else {
            perror("read failed");
        }
    }
    printf("-----------------------------------\n");
}

int main() {
    // 初始化FIFO信息
    FifoInfo fifos[FIFO_COUNT] = {
        {.path = "tmp/fifo1"},
        {.path = "tmp/fifo2"}
    };
    
    // 创建epoll实例
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }
    
    // 初始化所有FIFO并添加到epoll
    struct epoll_event ev;
    for (int i = 0; i < FIFO_COUNT; i++) {
        init_fifo(&fifos[i]);
        
        ev.events = EPOLLIN | EPOLLET;  // 边缘触发模式
        ev.data.ptr = &fifos[i];
        
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fifos[i].fd, &ev) == -1) {
            perror("epoll_ctl failed");
            exit(EXIT_FAILURE);
        }
    }
    
    printf("\nWaiting for data. To write to FIFOs, use:\n");
    printf("echo \"message\" > fifo1\n");
    printf("echo \"message\" > fifo2\n\n");
    
    // 事件循环
    struct epoll_event events[MAX_EVENTS];
    while (1) {
        // 会阻塞当前线程，任意一个文件描述符就绪都会解除阻塞，这时候就需要再次调用epoll_wait
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, 5000);  // 5秒超时
        if (nfds == -1) {
            perror("epoll_wait failed");
            break;
        } else if (nfds == 0) {
            printf("No data received in last 5 seconds...\n");
            continue;
        }
        
        // 处理所有就绪的文件描述符
        for (int n = 0; n < nfds; n++) {
            FifoInfo *fifo = (FifoInfo *)events[n].data.ptr;
            
            if (events[n].events & EPOLLIN) {
                handle_fifo_read(fifo);
            }
            
            if (events[n].events & (EPOLLHUP | EPOLLERR)) {
                printf("EPOLL error or HUP on %s\n", fifo->path);
            }
        }
    }
    
    // 清理资源
    for (int i = 0; i < FIFO_COUNT; i++) {
        close(fifos[i].fd);
        unlink(fifos[i].path);
    }
    close(epfd);
    
    return 0;
}