#pragma once
// #include "stdafx.h"
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "LedController.h"

#define DATA_BUFSIZE 512000 // 0.5KB
#define LED_CALL_DELAY 5

struct SOCKET_INFORMATION {
    char Buffer[DATA_BUFSIZE];
    char receivedMessage[DATA_BUFSIZE];
    WSABUF DataBuf;
    SOCKET Socket;
    WSAOVERLAPPED Overlapped;
    DWORD BytesSEND;
    DWORD BytesRECV;
    bool handshakeDone;
    size_t offset;
};

class WebSocketServer {
private:
    LedController* _lc;

    const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    const int PORT = 27015;

    bool _running;
    SOCKET server;
    SOCKET acceptSocket;
    SOCKET_INFORMATION* SocketArray[WSA_MAXIMUM_WAIT_EVENTS];
    DWORD eventTotal = 0;
    WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];
    DWORD _RecvBytes;
    DWORD _SendBytes = 0;
    DWORD flags;
    DWORD listenThreadID;

    static DWORD WINAPI listenForRequest(LPVOID lpParameter);
    static char* base64(const unsigned char* input, int length);
    static uint64_t getPayloadLength(const char* buffer, uint8_t& offset);
    bool isFrameComplete(SOCKET_INFORMATION* client);
    bool isFrameValid(SOCKET_INFORMATION* client);
    void getPayloadData(SOCKET_INFORMATION* client);
    void showFrameMetadata(SOCKET_INFORMATION* client);
    void handshake(SOCKET_INFORMATION* client);
    void closeConnectionWith(SOCKET_INFORMATION* client, size_t index);
    void createSendResponse(std::vector<uint8_t>& frame, std::string& message);
    void printBytes(uint8_t* bytes);
    void controlLights(uint8_t* bytes);
public:
    WebSocketServer(LedController* lc) : _lc(lc), _running(false), server(INVALID_SOCKET), acceptSocket(INVALID_SOCKET) {};
    bool Initialize();
    bool start();
    void stop();
    void handleRequests();
    void serverSend(SOCKET_INFORMATION* client, DWORD& evtIdx, std::string& data);

};
