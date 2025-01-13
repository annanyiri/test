#include <errno.h>
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

#define SO_RCVPRIORITY 82

int main() {
    int send_fd;
    int recv_fd;
    struct sockaddr_in send_addr;
    struct sockaddr_in recv_addr;
    struct msghdr msg;
    struct iovec iov[1];
    char recv_buf[1024];
    char cbuf[1024];
    struct cmsghdr *cmsg;
    __u32 recv_priority = 0;
    int err;

    recv_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (recv_fd < 0) {
        perror("Can't open recv socket");
        return -errno;
    }

    int enable = 1;
    err = setsockopt(recv_fd, SOL_SOCKET, SO_RCVPRIORITY, &enable, sizeof(enable));
    
    if (err < 0) {
        perror("Recv setsockopt error");
        close(recv_fd);
        return -errno;
    }

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(12345);
    recv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    err = bind(recv_fd, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
    if (err < 0) {
        perror("Recv bind error");
        close(recv_fd);
        return -errno;
    }

    send_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (send_fd < 0) {
        perror("Can't open send socket");
        return -errno;
    }

    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(12345);
    send_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);


    __u32 priority = 5;
    err = setsockopt(send_fd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));
    if (err < 0) {
        perror("Send setsockopt error");
        close(send_fd);
        return -errno;
    }


    err = sendto(send_fd, NULL, 0, 0, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
    if (err < 0) {
        perror("Packet send error");
        close(recv_fd);
        close(send_fd);
        return -errno;
    }

    printf("Message sent with SO_PRIORITY: %u\n", priority);

    iov[0].iov_base = recv_buf;
    iov[0].iov_len = 1024;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cbuf;
    msg.msg_controllen = sizeof(cbuf);

    err = recvmsg(recv_fd, &msg, 0);
    if (err < 0) {
        perror("Message receive error");
        close(recv_fd);
        close(send_fd);
        return -errno;
    }

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_PRIORITY) {
            recv_priority = *(__u32 *)CMSG_DATA(cmsg);
            printf("Received SO_PRIORITY: %u\n", recv_priority);
        }
    }

    close(recv_fd);
    close(send_fd);
    return 0;
}
