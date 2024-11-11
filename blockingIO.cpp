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

int main() {
    const char* fifo_path = "tmp/test_fifo";
    
    // 创建FIFO（命名管道）
    unlink(fifo_path); // 如果已存在则删除
    if (mkfifo(fifo_path, 0666) == -1) {
        perror("mkfifo failed");
        return 1;
    }
    
    printf("FIFO created. Waiting for data...\n");
    printf("To write to the FIFO, use: echo \"your message\" > test_fifo\n\n");
    
    // 以阻塞模式打开FIFO
    int fd = open(fifo_path, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        unlink(fifo_path);
        return 1;
    }
    
    char buffer[1024];
    int read_count = 0;
    
    while (read_count < 5) { // 读取5次后退出
        print_timestamp("Waiting for data");
        printf("Attempting read #%d... (blocked until data arrives)\n", read_count + 1);
        
        // 阻塞读取
        ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
        // 不能立马执行打印，只有当读取到数据之后再回继续
        print_timestamp("Read completed");
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf("Read %zd bytes: %s\n", bytes, buffer);
        } else if (bytes == 0) {
            printf("Writer closed the FIFO\n");
            break;
        } else {
            perror("read failed");
            break;
        }
        
        read_count++;
        printf("\nWaiting for next input...\n\n");
    }
    
    close(fd);
    unlink(fifo_path);
    return 0;
}