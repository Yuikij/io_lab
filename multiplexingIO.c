#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

int main() {
    int fd1 = open("file1.txt", O_RDONLY | O_NONBLOCK);
    int fd2 = open("file2.txt", O_RDONLY | O_NONBLOCK);
    if (fd1 < 0 || fd2 < 0) {
        perror("open failed");
        return 1;
    }

    fd_set read_fds;
    char buffer[100];
    int max_fd = fd1 > fd2 ? fd1 : fd2;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(fd1, &read_fds);
        FD_SET(fd2, &read_fds);

        int result = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (result < 0) {
            perror("select failed");
            break;
        }

        if (FD_ISSET(fd1, &read_fds)) {
            ssize_t bytes = read(fd1, buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                printf("Data from file1: %s\n", buffer);
            }
        }

        if (FD_ISSET(fd2, &read_fds)) {
            ssize_t bytes = read(fd2, buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                printf("Data from file2: %s\n", buffer);
            }
        }
    }

    close(fd1);
    close(fd2);
    return 0;
}
