#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>


#define FIFO_PATH "tmp/signal_fifo"
#define BUFFER_SIZE 1024

// 定义文件描述符变量
int fifo_fd;

// 信号处理函数
void handle_io_signal(int signum) {
    if (signum == SIGIO) {
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;

        // 从FIFO读取数据
        while ((bytes_read = read(fifo_fd, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            printf("Received %zd bytes: %s\n", bytes_read, buffer);
        }

        // 检查是否没有数据可读
        if (bytes_read == -1 && errno != EAGAIN) {
            perror("read failed");
            exit(EXIT_FAILURE);
        }
    }
}

int main() {
    // 创建FIFO
    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        perror("mkfifo failed");
        exit(EXIT_FAILURE);
    }

    // 打开FIFO并设置非阻塞模式
    fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        perror("open fifo failed");
        exit(EXIT_FAILURE);
    }

    // 设置信号处理程序
    struct sigaction sa;
    sa.sa_handler = handle_io_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGIO, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }

    // 配置文件描述符接收SIGIO信号
    if (fcntl(fifo_fd, F_SETOWN, getpid()) == -1) {
        perror("fcntl F_SETOWN failed");
        exit(EXIT_FAILURE);
    }

    // 设置文件描述符的信号驱动I/O和非阻塞标志
    int flags = fcntl(fifo_fd, F_GETFL);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fifo_fd, F_SETFL, flags | O_ASYNC | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for data. To write to FIFO, use:\n");
    printf("echo \"message\" > %s\n\n", FIFO_PATH);

    // 主事件循环
    while (1) {
        printf("Doing other work...\n");
        sleep(5);  // 模拟其它任务
    }

    // 清理资源
    close(fifo_fd);
    unlink(FIFO_PATH);
    return 0;
}
