#include "WebSocketServer.h"
#include "../libs/base64.hpp"
#include "../libs/sha1.hpp"
#include <Dispatchers/EffectDispatcher.h>
#include <arpa/inet.h>
#include <cstdio>
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

#define BITVALUE(X, N) (((X) >> (N)) & 0x1)

static uint64_t reverseBytes(uint64_t value) {
    return ((value & 0x00000000000000FFULL) << 56) |
           ((value & 0x000000000000FF00ULL) << 40) |
           ((value & 0x0000000000FF0000ULL) << 24) |
           ((value & 0x00000000FF000000ULL) << 8) |
           ((value & 0x000000FF00000000ULL) >> 8) |
           ((value & 0x0000FF0000000000ULL) >> 24) |
           ((value & 0x00FF000000000000ULL) >> 40) |
           ((value & 0xFF00000000000000ULL) >> 56);
}

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

bool WebSocketServer::performHandshake(Client &client) {
    printf("[LOG]::Perfoming handshake\n");

    std::string request(client.buffer);
    std::string SecKey = "Sec-WebSocket-Accept: ";
    size_t keyPos = request.find("Sec-WebSocket-Key: ");
    if (keyPos == std::string::npos) {
        keyPos = request.find("sec-websocket-key: ");
        SecKey = "sec-websocket-accept: ";
    }
    std::string SecValue = "";

    int keyOffset = 19;

    if (keyPos != std::string::npos) {
        size_t endPos = request.find("\r\n", keyPos);

        SecValue =
            request.substr(keyPos + keyOffset, endPos - keyPos - keyOffset);

        SecValue += GUID;

        std::array<uint8_t, 20> hash = SHA1::compute(SecValue);

        std::string buff =
            base64::to_base64(std::string(hash.begin(), hash.end()));

        std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
                               "Upgrade: websocket\r\n"
                               "Connection: Upgrade\r\n"
                               "" +
                               SecKey + buff + "\r\n\r\n";

        printf("Sending handshke!\n");
        // printf("Response: \n%s", response.c_str());

        write(client.fd, response.c_str(), response.length());
        return true;
    }

    return false;
}

bool WebSocketServer::initialize() {
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

bool WebSocketServer::start() {
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
    Client *client;
    struct sockaddr_in cli_addr;
    struct epoll_event events[MAX_EVENTS];
    epfd = epoll_create(1);
    epoll_ctl_add(epfd, serverSock, EPOLLIN | EPOLLOUT | EPOLLET);

    char buf[BUF_SIZE];
    socklen = sizeof(cli_addr);

    printf("Server started at %s:%i\n", inet_ntoa(srvAddr.sin_addr),
           htons(srvAddr.sin_port));

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
                Client newClient = {};
                newClient.fd = conn_sock;
                clients.insert({conn_sock, newClient});

            } else if (events[i].events & EPOLLIN) {
                /* handle EPOLLIN event */
                for (;;) {
                    client = &clients[events[i].data.fd];
                    bzero(buf, sizeof(buf));
                    n = read(events[i].data.fd, client->buffer,
                             sizeof(client->buffer));
                    if (n <= 0) {
                        break;
                    } else {
                        if (client->handshakeDone != true) {
                            printf("[+] data from %d: %s\n", client->fd,
                                   client->buffer);
                            if (performHandshake(*client)) {
                                client->handshakeDone = true;
                            };
                        } else {
                            if (isFrameValid(client->buffer)) {
                                showFrameMetadata(client->buffer);
                                getPayloadData(client->buffer);
                            }
                        }
                    }
                }
            } else {
                printf("[+] unexpected\n");
            }
            /* check if the connection is closing */
            if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
                printf("[+] connection closed\n");
                epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                clients.erase(events[i].data.fd);
                close(events[i].data.fd);
                continue;
            }
        }
    }
}

void WebSocketServer::showFrameMetadata(char *frame) {
    uint8_t offset = 1;
    printf("[FRAME METADATA]\n");
    printf("FIN: %d\n", BITVALUE(frame[0], 7));
    printf("RSV: %d %d %d\n", BITVALUE(frame[0], 6), BITVALUE(frame[0], 5),
           BITVALUE(frame[0], 4));
    printf("OPCODE: 0x%01x\n", frame[0] & 0xF);
    printf("MASK BIT: %d\n", BITVALUE(frame[1], 7));
    printf("PL length: %ld\n", getPayloadLength(frame, offset));
    return;
}

size_t WebSocketServer::getPayloadLength(char *frame, uint8_t &offset) {
    uint8_t payloadLen = frame[offset] & 0x7F;
    offset++;

    if (payloadLen <= 125)
        return payloadLen;

    if (payloadLen == 126) {
        uint16_t length = 0;
        printf("==126\n");
        std::memcpy(&length, frame + offset, sizeof(length));
        length = ntohs(length);
        offset += 2;
        return length;
    }

    if (payloadLen == 127) {
        uint64_t length = 0;
        printf("==127\n");
        std::memcpy(&length, frame + offset, sizeof(uint64_t));
        length = reverseBytes(length);
        offset += 8;
        return length;
    }
    return 0;
}

void WebSocketServer::getPayloadData(char *frame) {
    uint8_t offset = 0;
    offset++;

    uint64_t payloadLength = getPayloadLength(frame, offset);

    unsigned char *payloadData = new unsigned char[payloadLength];
    if (BITVALUE(frame[1], 7) != 0) {
        uint32_t *maskingKey = (uint32_t *)&frame[offset];
        offset += 4;

        size_t j = 0;
        uint8_t *original = (uint8_t *)&frame[offset];
        uint8_t *masking = (uint8_t *)maskingKey;
        for (size_t i = 0; i < payloadLength; ++i) {
            j = i % 4;
            payloadData[i] = original[i] ^ masking[j];
        }
    } else {
    }

    payloadData[payloadLength] = '\0';
    // printf("[ + DATA + ] %s\n", payloadData);
    printf("[ + DATA + ]\n");
    for (uint64_t i = 0; i < payloadLength; ++i) {
        printf("0x%02x ", payloadData[i]);
    }
    printf("\n");

    processData(payloadData, payloadLength);

    delete[] payloadData;
    return;
}

bool WebSocketServer::isFrameValid(char *frame) {
    return (frame[0] & 0x70) == 0;
}

WebSocketServer::~WebSocketServer() { stop(); }

void WebSocketServer::stop() {
    if (handleRequestsThread) {
        if (handleRequestsThread->joinable()) {
            handleRequestsThread->join();
        }

        delete handleRequestsThread;
    }
    close(serverSock);
}

void WebSocketServer::processData(const uint8_t *data,
                                  const uint32_t &dataLength) {
    // Route to dispatcher
    switch (data[0]) {
    case EFFECT_HEADER:
        EffectDispatcher::dispatch(_lc, data, dataLength);
    }
}
