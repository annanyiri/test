#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024



int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Usage: %s <IP Address> <Port>\n", argv[0]);
        return 1;
    }

    char *ip_address = argv[1];
    int port = atoi(argv[2]);

    int fd;
    struct sockaddr_in addr;
    struct msghdr msg;
    struct iovec iov[1];
    char buf[BUF_SIZE];
    char cbuf[1024];
    struct cmsghdr *cmsg;
    __u32 mark = 0;
    int err;

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip_address, &addr.sin_addr);

    err = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0) {
        perror("bind");
        close(fd);
        return 1;
    }

    iov[0].iov_base = buf;
    iov[0].iov_len = BUF_SIZE;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cbuf;
    msg.msg_controllen = sizeof(cbuf);

    printf("Waiting for a message on %s:%d...\n", ip_address, port);
    err = recvmsg(fd, &msg, 0);
    if (err < 0) {
        perror("recvmsg");
        close(fd);
        return 1;
    }

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_MARK) {
            mark = *(__u32 *)CMSG_DATA(cmsg);
            printf("Received SO_MARK: %u\n", mark);
        }
    }

    printf("Received message: %s\n", buf);

    close(fd);
    return 0;
}

