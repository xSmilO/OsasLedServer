#pragma once
#include "LedController.h"
#include <arpa/inet.h>
#include <strings.h>
#include <thread>
#include <unordered_map>
#include <string>

#define SHA_DIGEST_LENGTH 20

struct Client {
    bool handshakeDone = false;
    char buffer[4096];
    size_t offset = 0;
    int fd = -1;
};

class WebSocketServer {
  private:
    const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    LedController *_lc;
    sockaddr_in srvAddr;
    std::unordered_map<int, Client> clients = {};
    int serverSock;
    std::thread *handleRequestsThread;
    void handleRequests();
    void showFrameMetadata(char *frame);
    void getPayloadData(char* frame);
    size_t getPayloadLength(char *frame, uint8_t &offset);
    bool isFrameValid(char *frame);
    bool performHandshake(Client &client);

  public:
    WebSocketServer(LedController *lc) : _lc(lc) {}
    ~WebSocketServer();
    bool Initialize();
    bool Start();
    void Stop();
};
