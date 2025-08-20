#pragma once
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include "LedController.h"

struct Client {
    int fd;
    bool handshakeDone = false;
    char buffer[4096];
    size_t offset = 0;
    std::string receivedMessage;
};

class WebSocketServer {
  private:
    LedController *_lc;
    sockaddr_in srvAddr;
    std::vector<Client> clients = {};
    int serverSock;
    std::thread* handleRequestsThread;
    void handleRequests();
    void performHandshake(const char* buf);

  public:
    WebSocketServer(LedController *lc) : _lc(lc) {}
    ~WebSocketServer();
    bool Initialize();
    bool Start();
    void Stop();
};
