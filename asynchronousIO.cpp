#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <libaio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>  // Include this header for localtime and strftime

#define BUFFER_SIZE 4096  // 必须是块大小的整数倍
#define MAX_EVENTS 5

// 全局变量，用于信号处理
volatile sig_atomic_t io_ready = 0;
io_context_t aio_ctx = 0;

void print_timestamp(const char* prefix) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    char timestamp[64];
    struct tm *tm_info;

    tm_info = localtime(&tv.tv_sec);  // This is where the error occurred
    if (tm_info == NULL) {
        perror("localtime failed");
        return;
    }

    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);
    printf("%s: %s.%06ld\n", prefix, timestamp, tv.tv_usec);
}

// 信号处理函数
void sigio_handler(int signo) {
    io_ready = 1;
}

// 设置对齐的内存缓冲区
void* allocate_aligned_buffer(size_t size) {
    void* buf = NULL;
    if (posix_memalign(&buf, getpagesize(), size) != 0) {
        return NULL;
    }
    return buf;
}

int main() {
    struct sigaction sa;
    const char* fifo_path = "test_fifo";
    struct iocb *iocbs[MAX_EVENTS];
    struct io_event events[MAX_EVENTS];
    char **buffers;
    int submitted = 0;
    int completed = 0;
    
    // 初始化信号处理
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigio_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGIO, &sa, NULL) < 0) {
        perror("sigaction failed");
        return 1;
    }
    
    // 创建并打开FIFO
    unlink(fifo_path);
    if (mkfifo(fifo_path, 0666) == -1) {
        perror("mkfifo failed");
        return 1;
    }
    
    printf("FIFO created. Demo of AIO with signal notification...\n");
    printf("To write to the FIFO, use: echo \"your message\" > test_fifo\n\n");

    // 打开FIFO，使用O_DIRECT标志
    int fd = open(fifo_path, O_RDONLY | O_DIRECT);
    if (fd == -1) {
        perror("open failed");
        unlink(fifo_path);
        return 1;
    }

    // 设置文件描述符为异步模式
    if (fcntl(fd, F_SETOWN, getpid()) < 0) {
        perror("fcntl F_SETOWN failed");
        goto cleanup;
    }
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_ASYNC) < 0) {
        perror("fcntl F_SETFL O_ASYNC failed");
        goto cleanup;
    }

    // 初始化AIO上下文
    if (io_setup(MAX_EVENTS, &aio_ctx) != 0) {
        perror("io_setup failed");
        goto cleanup;
    }

    // 分配对齐的缓冲区
    buffers = (char **)calloc(MAX_EVENTS, sizeof(char*));
    for (int i = 0; i < MAX_EVENTS; i++) {
        buffers[i] = (char *)allocate_aligned_buffer(BUFFER_SIZE);
        if (!buffers[i]) {
            perror("buffer allocation failed");
            goto cleanup;
        }
        
        // 创建并初始化iocb
        iocbs[i] = (iocb  *)malloc(sizeof(struct iocb));
        if (!iocbs[i]) {
            perror("iocb allocation failed");
            goto cleanup;
        }
        
        // 设置异步读取请求
        io_prep_pread(iocbs[i], fd, buffers[i], BUFFER_SIZE, 0);
        io_set_eventfd(iocbs[i], fd);  // 关联文件描述符用于信号通知
    }

    printf("Starting asynchronous read operations...\n");

    // 提交初始读取请求
    while (submitted < MAX_EVENTS && completed < MAX_EVENTS) {
        print_timestamp("Submitting read request");
        
        int ret = io_submit(aio_ctx, 1, &iocbs[submitted]);
        if (ret != 1) {
            if (ret < 0) {
                fprintf(stderr, "io_submit failed: %s\n", strerror(-ret));
            } else {
                fprintf(stderr, "io_submit failed: submitted %d instead of 1\n", ret);
            }
            break;
        }
        
        printf("Submitted async read request #%d\n", submitted + 1);
        submitted++;

        // 等待信号通知
        while (!io_ready) {
            pause();  // 暂停直到收到信号
        }
        io_ready = 0;  // 重置信号标志

        // 检查完成的事件
        int num_events = io_getevents(aio_ctx, 0, submitted, events, NULL);
        for (int i = 0; i < num_events; i++) {
            struct io_event *event = &events[i];
            if (event->res > 0) {
                print_timestamp("Data received");
                printf("Completed read request: %ld bytes\n", event->res);
                buffers[completed][event->res] = '\0';
                printf("Data: %s\n", buffers[completed]);
                completed++;
            } else if (event->res == 0) {
                printf("End of file reached\n");
                completed++;
            } else {
                fprintf(stderr, "Read error: %s\n", strerror(-event->res));
                completed++;
            }
        }
        
        printf("-----------------------------------\n");
    }

    printf("All async operations completed.\n");

cleanup:
    // 清理资源
    if (aio_ctx) {
        io_destroy(aio_ctx);
    }
    if (buffers) {
        for (int i = 0; i < MAX_EVENTS; i++) {
            free(buffers[i]);
            free(iocbs[i]);
        }
        free(buffers);
    }
    close(fd);
    unlink(fifo_path);
    
    return 0;
}
