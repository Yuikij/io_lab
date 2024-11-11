#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libaio.h>
#include <sys/stat.h>

#define FIFO_PATH "tmp/asynchronous_fifo"
#define BUFFER_SIZE 1024

// 异步 I/O 上下文和缓冲区
io_context_t ctx;
char buffer[BUFFER_SIZE];

// 异步 I/O 完成回调
void handle_aio_complete(struct io_event *event) {
    // 检查读取结果
    if ((long)event->res > 0) {
        buffer[event->res] = '\0';
        printf("Read %ld bytes asynchronously: %s\n", (long)event->res, buffer);
    } else {
        printf("No data read asynchronously or error occurred.\n");
    }
}

int main() {
    int ret;

    // 初始化异步 I/O 上下文
    memset(&ctx, 0, sizeof(ctx));
    ret = io_setup(10, &ctx);
    if (ret < 0) {
        perror("io_setup failed");
        exit(EXIT_FAILURE);
    }

    // 创建 FIFO
    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        perror("mkfifo failed");
        exit(EXIT_FAILURE);
    }

    // 打开 FIFO 并设置为非阻塞模式
    int fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        perror("open FIFO failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for data. To write to FIFO, use:\n");
    printf("echo \"message\" > %s\n\n", FIFO_PATH);

    while (1) {
        struct iocb cb;
        struct iocb *cbs[1] = { &cb };
        struct io_event events[1];

        // 准备异步读取操作
        io_prep_pread(&cb, fifo_fd, buffer, BUFFER_SIZE - 1, 0);

        // 提交异步读取请求
        ret = io_submit(ctx, 1, cbs);
        if (ret < 0) {
            perror("io_submit failed");
            break;
        }

        // 等待 I/O 完成事件
        ret = io_getevents(ctx, 1, 1, events, NULL);
        if (ret < 0) {
            perror("io_getevents failed");
            break;
        }

        // 调用回调处理完成事件
        handle_aio_complete(&events[0]);

        // 模拟处理其他任务
        printf("Doing other work...\n");
        sleep(5);
    }

    // 关闭 FIFO 并清理异步 I/O 上下文
    close(fifo_fd);
    io_destroy(ctx);
    unlink(FIFO_PATH);

    return 0;
}
