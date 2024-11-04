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
#include <pthread.h>

#define NUM_THREADS 5

enum {
    ERN_SUCCESS = 0,
    ERN_SEND = 1,
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
} opt = {
    .sock = {
        .type = SOCK_DGRAM,
    },
};

struct thread_data {
    int thread_id;
    int priority_value;
};

static void __attribute__((noreturn)) cs_usage(const char *bin) {
    printf("Usage: %s [opts] <dst host> <dst port / service>\n", bin);
    printf("Options:\n"
           "\t\t-s      Silent send() failures\n"
           "");
    exit(ERN_HELP);
}

static void cs_parse_args(int argc, char *argv[]) {
    char o;

    while ((o = getopt(argc, argv, "s")) != -1) {
        switch (o) {
        case 's':
            opt.silent_send = true;
            break;
        }
    }

    if (optind != argc - 2)
        cs_usage(argv[0]);

    opt.host = argv[optind];
    opt.service = argv[optind + 1];
}

static void cs_write_cmsg(struct msghdr *msg, char *cbuf, size_t cbuf_sz, unsigned int priority) {
    struct cmsghdr *cmsg;
    size_t cmsg_len = 0;

    msg->msg_control = cbuf;

    cmsg = (struct cmsghdr *)(cbuf + cmsg_len);
    cmsg_len += CMSG_SPACE(sizeof(__u32));
    if (cbuf_sz < cmsg_len)
        error(ERN_CMSG_WR, EFAULT, "cmsg buffer too small");

    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SO_PRIORITY;
    cmsg->cmsg_len = CMSG_LEN(sizeof(__u32));
    *(__u32 *)CMSG_DATA(cmsg) = priority;

    msg->msg_controllen = cmsg_len;
}

void *send_packet(void *thread_arg) {
    struct thread_data *data = (struct thread_data *)thread_arg;
    int thread_id = data->thread_id;
    int priority_value = data->priority_value;

    struct addrinfo hints, *ai;
    struct iovec iov[1];
    struct msghdr msg;
    char cbuf[1024];
    char message[128];
    int err, fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = opt.sock.type;

    err = getaddrinfo(opt.host, opt.service, &hints, &ai);
    if (err) {
        fprintf(stderr, "Thread %d: Can't resolve address [%s]:%s: %s\n",
                thread_id, opt.host, opt.service, strerror(errno));
        pthread_exit((void *)ERN_SOCK_CREATE);
    }

    fd = socket(ai->ai_family, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        fprintf(stderr, "Thread %d: Can't open socket: %s\n", thread_id, strerror(errno));
        freeaddrinfo(ai);
        pthread_exit((void *)ERN_RESOLVE);
    }

    // Create a message with the priority value appended
    snprintf(message, sizeof(message), "Packet from thread %d with priority %d", thread_id, priority_value);
    iov[0].iov_base = message;
    iov[0].iov_len = strlen(message) + 1; // Include the null terminator

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = ai->ai_addr;
    msg.msg_namelen = ai->ai_addrlen;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    cs_write_cmsg(&msg, cbuf, sizeof(cbuf), priority_value);

    err = sendmsg(fd, &msg, 0);
    if (err < 0) {
        if (!opt.silent_send)
            fprintf(stderr, "Thread %d: send failed: %s\n", thread_id, strerror(errno));
        err = ERN_SEND;
    } else if (err != iov[0].iov_len) {
        fprintf(stderr, "Thread %d: short send\n", thread_id);
        err = ERN_SEND_SHORT;
    } else {
        err = ERN_SUCCESS;
    }

    close(fd);
    freeaddrinfo(ai);
    pthread_exit((void *)(long)err);
}

int main(int argc, char *argv[]) {
    pthread_t threads[NUM_THREADS];
    struct thread_data thread_data_array[NUM_THREADS];
    int priorities[NUM_THREADS] = {1, 2, 3, 4, 5}; // Set different priorities
    int rc;

    cs_parse_args(argc, argv);

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data_array[i].thread_id = i;
        thread_data_array[i].priority_value = priorities[i];
        rc = pthread_create(&threads[i], NULL, send_packet, (void *)&thread_data_array[i]);
        if (rc) {
            fprintf(stderr, "Error creating thread %d: %d\n", i, rc);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        void *status;
        rc = pthread_join(threads[i], &status);
        if (rc) {
            fprintf(stderr, "Error joining thread %d: %d\n", i, rc);
        }
        printf("Thread %d exited with status: %ld\n", i, (long)status);
    }

    return ERN_SUCCESS;
}

