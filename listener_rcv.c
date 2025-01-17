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

struct options {
    __u32 val;
    int name;
    int rcvname;
    const char *host;
    const char *service;
} opt;

static void __attribute__((noreturn)) cs_usage(const char *bin)
{
	printf("Usage: %s [opts] <dst host> <dst port / service>\n", bin);
	printf("Options:\n"
	       "\t\t-M val  Test SO_RCVMARK\n"
	       "\t\t-P val  Test SO_RCVPRIORITY\n"
	       "");
	exit(EXIT_FAILURE);
}

static void cs_parse_args(int argc, char *argv[])
{
	int o;

	while ((o = getopt(argc, argv, "M:P:")) != -1) {
		switch (o) {
		case 'M':
			opt.val = atoi(optarg);
            opt.name = SO_MARK;
            opt.rcvname = SO_RCVMARK;
			break;
		case 'P':
			opt.val = atoi(optarg);
            opt.name = SO_PRIORITY;
            opt.rcvname = SO_RCVPRIORITY;
			break;
        default:
            cs_usage(argv[0]);
			break;
		}
	}

	if (optind != argc - 2)
		cs_usage(argv[0]);

	opt.host = argv[optind];
	opt.service = argv[optind + 1];
}


int main(int argc, char *argv[]) {
    int recv_fd;
    struct sockaddr_in recv_addr;
    struct msghdr msg;
    struct iovec iov[1];
    char recv_buf[1024];
    char cbuf[1024];
    struct cmsghdr *cmsg;
    __u32 recv_val;
    int err;

    cs_parse_args(argc, argv);

    recv_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (recv_fd < 0) {
        perror("Can't open recv socket");
        return -errno;
    }

    err = setsockopt(recv_fd, SOL_SOCKET, opt.rcvname, &opt.val, sizeof(opt.val));
    
    if (err < 0) {
        perror("Recv setsockopt error");
        close(recv_fd);
        return -errno;
    }

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(atoi(opt.service));

    if (inet_pton(AF_INET, opt.host, &recv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(recv_fd);
        return -errno;
    }

    err = bind(recv_fd, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
    if (err < 0) {
        perror("Recv bind error");
        close(recv_fd);
        return -errno;
    }

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
        return -errno;
    }

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == opt.name) {
            recv_val = *(__u32 *)CMSG_DATA(cmsg);
            printf("Received value: %u\n", recv_val);

            if (recv_val != opt.val) {
                fprintf(stderr, "Error: expeced value: %u, got: %u\n", opt.val, recv_val);
                close(recv_fd);
                return -EINVAL;
            }
        }
    }

    close(recv_fd);
    return 0;
}
