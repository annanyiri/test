// SPDX-License-Identifier: GPL-2.0-or-later
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
#include <netinet/ip.h>
#include <arpa/inet.h>

enum {
	ERN_SUCCESS = 0,
	/* Well defined errors, callers may depend on these */
	ERN_SEND = 1,
	/* Informational, can reorder */
	ERN_HELP,
	ERN_SEND_SHORT,
	ERN_SOCK_CREATE,
	ERN_RESOLVE,
	ERN_CMSG_WR,
};

struct options {
	bool silent_send;
	const char *host;
	const char *service;
	struct {
		unsigned int type;
	} sock;
	struct {
		bool ena;
		unsigned int val;
	} priority;
} opt = {
	.sock = {
		.type	= SOCK_RAW,
	},
};

static void __attribute__((noreturn)) cs_usage(const char *bin)
{
	printf("Usage: %s [opts] <dst host> <dst port / service>\n", bin);
	printf("Options:\n"
	       "\t\t-s      Silent send() failures\n"
	       "\t\t-p val  Set SO_PRIORITY with given value\n"
	       "");
	exit(ERN_HELP);
}

static void cs_parse_args(int argc, char *argv[])
{
	char o;

	while ((o = getopt(argc, argv, "sp:")) != -1) {
		switch (o) {
		case 's':
			opt.silent_send = true;
			break;
		case 'p':
			opt.priority.ena = true;
			opt.priority.val = atoi(optarg);
			break;
		}
	}

	if (optind != argc - 2)
		cs_usage(argv[0]);

	opt.host = argv[optind];
	opt.service = argv[optind + 1];
}

static void
cs_write_cmsg(struct msghdr *msg, char *cbuf, size_t cbuf_sz)
{
	struct cmsghdr *cmsg;
	size_t cmsg_len;

	msg->msg_control = cbuf;
	cmsg_len = 0;

	if (opt.priority.ena) {
		cmsg = (struct cmsghdr *)(cbuf + cmsg_len);
		cmsg_len += CMSG_SPACE(sizeof(__u32));
		if (cbuf_sz < cmsg_len)
			error(ERN_CMSG_WR, EFAULT, "cmsg buffer too small");

		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SO_PRIORITY;
		cmsg->cmsg_len = CMSG_LEN(sizeof(__u32));
		*(__u32 *)CMSG_DATA(cmsg) = opt.priority.val;
	}

	if (cmsg_len)
		msg->msg_controllen = cmsg_len;
	else
		msg->msg_control = NULL;
}

static unsigned short checksum(void *b, int len)
{
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;
	if (len == 1)
		sum += *(unsigned char *)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

int main(int argc, char *argv[])
{
	struct iovec iov[1];
	struct msghdr msg;
	struct sockaddr_in dest_addr;
	char cbuf[1024];
	char packet[4096];
	struct iphdr *iph;
	int err;
	int fd;

	cs_parse_args(argc, argv);

	fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (fd < 0) {
		fprintf(stderr, "Can't open raw socket: %s\n", strerror(errno));
		return ERN_SOCK_CREATE;
	}

	// Manually set the destination address since getaddrinfo() isn't needed for raw sockets
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(atoi(opt.service)); // Normally, this is not used in raw sockets
	inet_pton(AF_INET, opt.host, &dest_addr.sin_addr);

	// Construct the IP header
	iph = (struct iphdr *)packet;
	iph->version = 4;
	iph->ihl = 5;
	iph->tos = 0;
	iph->tot_len = sizeof(struct iphdr) + 4;  // IP header + payload
	iph->id = htonl(54321);  // ID of this packet
	iph->frag_off = 0;
	iph->ttl = 64;
	iph->protocol = IPPROTO_RAW;
	iph->check = 0;
	iph->saddr = htonl(INADDR_ANY);  // Let the OS choose the source IP
	iph->daddr = dest_addr.sin_addr.s_addr;

	// Calculate the checksum for integrity
	iph->check = checksum((unsigned short *)packet, iph->tot_len);

	// Add payload
	strcpy(packet + sizeof(struct iphdr), "bla");

	iov[0].iov_base = packet;
	iov[0].iov_len = iph->tot_len;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	cs_write_cmsg(&msg, cbuf, sizeof(cbuf));

	err = sendmsg(fd, &msg, 0);
	if (err < 0) {
		if (!opt.silent_send)
			fprintf(stderr, "send failed: %s\n", strerror(errno));
		err = ERN_SEND;
	} else if (err != iph->tot_len) {
		fprintf(stderr, "short send\n");
		err = ERN_SEND_SHORT;
	} else {
		err = ERN_SUCCESS;
	}

	close(fd);
	return err;
}

