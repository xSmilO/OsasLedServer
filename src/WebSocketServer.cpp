#include "WebSocketServer.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_PORT 27015
#define MAX_CONN 16
#define MAX_EVENTS 32
#define BUF_SIZE 512
#define MAX_LINE 256

static int setNonBlocking(int sockfd) {
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

static void setSockaddr(struct sockaddr_in *addr) {
    bzero((char *)addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(DEFAULT_PORT);
}

static void epoll_ctl_add(int epfd, int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl()\n");
        exit(1);
    }
}

WebSocketServer::~WebSocketServer() {
    Stop();
}

bool WebSocketServer::Initialize() {
    serverSock = socket(AF_INET, SOCK_STREAM, 0);

    setSockaddr(&srvAddr);
    if (bind(serverSock, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0) {
        perror("bind");
        return false;
    }

    setNonBlocking(serverSock);
    listen(serverSock, MAX_CONN);

    return true;
}

bool WebSocketServer::Start() {
    handleRequestsThread =
        new std::thread(&WebSocketServer::handleRequests, this);

    return true;
}

void WebSocketServer::handleRequests() {
    int i, n;
    int epfd;
    int conn_sock;
    int socklen;
    int nfds;
    struct sockaddr_in cli_addr;
    struct epoll_event events[MAX_EVENTS];
    epfd = epoll_create(1);
    epoll_ctl_add(epfd, serverSock, EPOLLIN | EPOLLOUT | EPOLLET);

    char buf[BUF_SIZE];
    socklen = sizeof(cli_addr);

    printf("Server started at %s:%i\n", inet_ntoa(srvAddr.sin_addr), htons(srvAddr.sin_port));

    for (;;) {
        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (i = 0; i < nfds; i++) {
            if (events[i].data.fd == serverSock) {
                /* handle new connection */
                conn_sock = accept(serverSock, (sockaddr *)&cli_addr,
                                   (socklen_t *)&socklen);

                inet_ntop(AF_INET, (char *)&(cli_addr.sin_addr), buf,
                          sizeof(cli_addr));
                printf("[+] connected with %s:%d\n", buf,
                       ntohs(cli_addr.sin_port));

                setNonBlocking(conn_sock);
                epoll_ctl_add(epfd, conn_sock,
                              EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP);
            } else if (events[i].events & EPOLLIN) {
                /* handle EPOLLIN event */
                for (;;) {
                    bzero(buf, sizeof(buf));
                    n = read(events[i].data.fd, buf, sizeof(buf));
                    if (n <= 0 /* || errno == EAGAIN */) {
                        break;
                    } else {
                        printf("[+] data: %s\n", buf);
                        write(events[i].data.fd, buf, strlen(buf));
                    }
                }
            } else {
                printf("[+] unexpected\n");
            }
            /* check if the connection is closing */
            if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
                printf("[+] connection closed\n");
                epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                close(events[i].data.fd);
                continue;
            }
        }
    }
}
void WebSocketServer::Stop() {
    if (handleRequestsThread) {
        if (handleRequestsThread->joinable()) {
            handleRequestsThread->join();
        }

        delete handleRequestsThread;
    }
    close(serverSock);
}
