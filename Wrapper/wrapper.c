#include "wrapper.h"

int Socket(int family, int type, int protocol) {
    int n;
    if ((n = socket(family, type, protocol)) < 0) {
        perror("socket error");
        exit(1);
    }

    return n;
}

void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (bind(sockfd, addr, addrlen) < 0) {
        perror("bind error");
        exit(1);
    }
}

void Listen(int sockfd, int queueLen) {
    if (listen(sockfd, queueLen) < 0) {
        perror("listen error");
        exit(1);
    }

}


int Accept(int sockfd, struct sockaddr *clientaddr, socklen_t *addr_dim) {
    int n;

    if ((n = accept(sockfd, clientaddr, addr_dim)) < 0) {
        perror("accept error");
        exit(1);
    }

    return n;
}

void Connect(int sockfd, const struct sockaddr *servaddr, socklen_t addr_dim) {
    if (connect(sockfd, (const struct sockaddr *) servaddr, addr_dim) < 0) {
        fprintf(stderr, "connect error\n");
        exit(1);
    }
}

//ssize_t FullRead(int fd, void *buf, size_t count) {
//    size_t nleft;
//    ssize_t nread;
//    nleft = count;
//    while (nleft > 0) {             /* repeat until no left */
//        if ((nread = read(fd, buf, nleft)) < 0) {
//            if (errno == EINTR) {   /* if interrupted by system call */
//                continue;           /* repeat the loop */
//            } else {
//                exit(nread);      /* otherwise exit */
//            }
//        } else if (nread == 0) {    /* EOF */
//            break;                  /* break loop here */
//        }
//        nleft -= nread;             /* set left to read */
//        buf += nread;                /* set pointer */
//    }
//    buf = 0;
//    return (nleft);
//}

ssize_t FullRead(int fd, void *buf, size_t count) {
    size_t nleft;
    ssize_t nread;
    nleft = count;
    while (nleft > 0) {
        if ((nread = read(fd, buf, nleft)) < 0) {
            //se c'è stato errore
            if (errno = EINTR) { continue; }
            else { exit(nread); }
        } else if (nread == 0) { break; }//chiuso il canale

        nleft -= nread;
        buf += nread;
    }
    buf = 0;
    return (nleft);
}