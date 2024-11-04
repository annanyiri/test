#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/socket.h>

#define BUFFER_SIZE 1024

int client_socket;

int main(int argc, char *argv[])
{
    int ret;
    struct sockaddr_ll server_addr;
    char buffer[BUFFER_SIZE];
    struct msghdr msg;
    struct iovec iov;
    struct cmsghdr *cmsg;
    char cmsgbuf[CMSG_SPACE(sizeof(int))];  // Buffer for control message
    int priority = atoi(argv[1]);

    // Create a socket
    client_socket = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
    if (client_socket < 0) {
        perror("socket");
        return -1;
    }

    // Prepare the destination address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sll_family = AF_PACKET;
    server_addr.sll_protocol = htons(ETH_P_ALL);
    server_addr.sll_ifindex = if_nametoindex("red0");

    sprintf(buffer, "%s", "hello server!");

    // Set up the message header (msghdr) for sending with control message
    memset(&msg, 0, sizeof(msg));
    iov.iov_base = buffer;
    iov.iov_len = strlen(buffer);
    msg.msg_name = &server_addr;
    msg.msg_namelen = sizeof(server_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    
    // Set up the control message (cmsg) to send SO_PRIORITY
    memset(cmsgbuf, 0, sizeof(cmsgbuf));
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SO_PRIORITY;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    *(__u32 *)CMSG_DATA(cmsg) = priority;

    // Send the message with control message
    ret = sendmsg(client_socket, &msg, 0);
    if (ret == -1) {
        perror("sendmsg");
        close(client_socket);
        return -2;
    }

    printf("Sent buffer with priority %d: %s\n", priority, buffer);

    // Close the socket
    close(client_socket);

    return 0;
}

