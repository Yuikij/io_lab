#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main() {
    char buffer[1024];   // 用于存储输入的数据
    ssize_t bytesRead;

    // 将标准输入设置为非阻塞模式
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        perror("获取文件状态标志失败");
        exit(EXIT_FAILURE);
    }

    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("设置非阻塞模式失败");
        exit(EXIT_FAILURE);
    }

    printf("请输入一些内容（在 3 秒内输入，将尝试读取数据）：\n");

    // 非阻塞I/O操作：循环检查是否有数据可读
    for (int i = 0; i < 3; i++) {  // 循环尝试 3 次读取
        bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

        if (bytesRead == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("没有数据可读取，稍后再试...\n");
            } else {
                perror("读取数据出错");
                exit(EXIT_FAILURE);
            }
        } else if (bytesRead == 0) {
            printf("输入结束（EOF）\n");
            break;
        } else {
            buffer[bytesRead] = '\0';  // 确保字符串以 null 结尾
            printf("读取到的数据：%s\n", buffer);
            break;
        }

        sleep(1);  // 暂停 1 秒再尝试读取
    }

    printf("非阻塞读取结束。\n");
    return 0;
}
