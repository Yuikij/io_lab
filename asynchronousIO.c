
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <aio.h>
#include <string.h>
#include <signal.h>

void aio_completion_handler(sigval_t sigval) {
    struct aiocb *req = (struct aiocb *)sigval.sival_ptr;
    if (aio_error(req) == 0) {
        ssize_t bytes = aio_return(req);
        printf("Async I/O complete: %s\n", (char *)req->aio_buf);
    } else {
        perror("Async I/O failed");
    }
}

int main() {
    int fd = open("test.txt", O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return 1;
    }

    char *buffer = (char *)malloc(100);
    struct aiocb req;
    memset(&req, 0, sizeof(req));
    req.aio_fildes = fd;
    req.aio_buf = buffer;
    req.aio_nbytes = 100;
    req.aio_sigevent.sigev_notify = SIGEV_THREAD;
    req.aio_sigevent.sigev_notify_function = aio_completion_handler;
    req.aio_sigevent.sigev_value.sival_ptr = &req;

    // 发起异步I/O请求
    if (aio_read(&req) == -1) {
        perror("aio_read failed");
        free(buffer);
        close(fd);
        return 1;
    }

    printf("Async I/O in progress...\n");

    // 模拟执行其他任务
    sleep(3);
    printf("Doing other work while I/O completes...\n");

    sleep(5);  // 等待异步I/O完成
    free(buffer);
    close(fd);
    return 0;
}
