#pragma once
#include <winsock2.h>
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#define DATA_BUFSIZE 8192

struct SOCKET_INFORMATION {
    char Buffer[DATA_BUFSIZE];
    WSABUF DataBuf;
    SOCKET Socket;
    WSAOVERLAPPED Overlapped;
    DWORD BytesSEND;
    DWORD BytesRECV;
    bool handshakeDone;
}; 

class WebSocket {
private:
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
    void handshake(SOCKET_INFORMATION *client);
    void interpretData(SOCKET_INFORMATION* client);
    static uint64_t getPayloadLength(const char* buffer, size_t &offset); 
public:
    WebSocket() : _running(false), server(INVALID_SOCKET), acceptSocket(INVALID_SOCKET) {};
    bool Initialize();
    bool start();
    void stop();
    void handleRequests();
}; 