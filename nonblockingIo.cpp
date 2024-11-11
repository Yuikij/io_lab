#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

void print_timestamp(const char* prefix) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", localtime(&tv.tv_sec));
    printf("%s: %s.%06ld\n", prefix, timestamp, tv.tv_usec);
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        exit(1);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        exit(1);
    }
}

int main() {
    const char* fifo_path = "tmp/test_fifo";
    
    // 创建FIFO
    unlink(fifo_path);
    if (mkfifo(fifo_path, 0666) == -1) {
        perror("mkfifo failed");
        return 1;
    }
    
    printf("FIFO created. Demo of non-blocking I/O...\n");
    printf("To write to the FIFO, use: echo \"your message\" > test_fifo\n\n");
    
    // 以非阻塞模式打开FIFO
    int fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("open failed");
        unlink(fifo_path);
        return 1;
    }
    
    char buffer[1024];
    int read_count = 0;
    
    // 持续运行，直到读取到5次数据
    while (read_count < 5) {
        print_timestamp("Attempting read");
        printf("Waiting for data... (read count: %d/5)\n", read_count);
        
        // 非阻塞读取
        ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
        
        if (bytes > 0) {
            // 成功读取数据
            buffer[bytes] = '\0';
            print_timestamp("Data received");
            printf("Read %zd bytes: %s\n", bytes, buffer);
            read_count++;
            printf("\nWaiting for next input...\n\n");
        } else if (bytes == 0) {
            // 写入端关闭
            read_count++;
            printf("Writer closed the FIFO\n");
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 没有数据可读
            printf("No data available right now\n");
            printf("Process can do other things here instead of blocking...\n");
        } else {
            // 其他错误
            perror("read failed");
            break;
        }
        
        printf("-----------------------------------\n");
        sleep(2);  // 等待2秒再次尝试
    }
    
    printf("Successfully read 5 messages. Exiting...\n");
    close(fd);
    unlink(fifo_path);
    return 0;
}