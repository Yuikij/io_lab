#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    char buffer[1024];  // 创建一个缓冲区来存储输入的数据

    printf("请输入一些内容（按Ctrl+D结束输入）：\n");

    // 阻塞I/O操作：使用fgets读取用户输入
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        // 将读取的数据输出到控制台
        printf("您输入的内容是：%s", buffer);
    }

    if (feof(stdin)) {
        printf("\nEOF received. 退出程序。\n");
    } else {
        perror("fgets 出错");
        exit(EXIT_FAILURE);
    }

    return 0;
}
