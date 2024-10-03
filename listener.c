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

int main() {
    int fd;
    struct sockaddr_in addr;
    struct msghdr msg;
    struct iovec iov[1];
    char buf[BUF_SIZE];
    char cbuf[1024];
    struct cmsghdr *cmsg;
    __u32 mark = 0;
    int err;

    // Create a UDP socket
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    // Set up the address for 1.1.1.2:12345
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    inet_pton(AF_INET, "1.1.1.2", &addr.sin_addr);

    // Bind the socket to the address
    err = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0) {
        perror("bind");
        close(fd);
        return 1;
    }

    // Prepare for receiving the message
    iov[0].iov_base = buf;
    iov[0].iov_len = BUF_SIZE;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cbuf;
    msg.msg_controllen = sizeof(cbuf);

    // Wait for the message
    printf("Waiting for a message on 1.1.1.2:12345...\n");
    err = recvmsg(fd, &msg, 0);
    if (err < 0) {
        perror("recvmsg");
        close(fd);
        return 1;
    }

    // Process control messages (ancillary data)
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_MARK) {
            mark = *(__u32 *)CMSG_DATA(cmsg);
            printf("Received SO_MARK: %u\n", mark);
        }
    }

    // Print the received message
    printf("Received message: %s\n", buf);

    close(fd);
    return 0;
}

