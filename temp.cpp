
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <stdio.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <WebSocket.h>
// #pragma comment(lib, "Ws2_32.lib")


#define DEFAULT_PORT "27015"

char* base64(const unsigned char* input, int length) {
    BIO* bmem, * b64;
    BUF_MEM* bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    char* buff = (char*)malloc(bptr->length);
    memcpy(buff, bptr->data, bptr->length - 1);
    buff[bptr->length - 1] = 0;

    BIO_free_all(b64);

    return buff;
}

void handshake(SOCKET client, std::string request) {
    size_t keyPos = request.find("Sec-WebSocket-Key: ");
    std::string SecKey = "";
    int keyOffset = 19;
    if (keyPos != std::string::npos) {
        size_t endPos = request.find("\r\n", keyPos);

        SecKey = request.substr(keyPos + keyOffset, endPos - keyPos - keyOffset);
        SecKey += GUID;
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(SecKey.c_str()), SecKey.size(), hash);

        char* buff = base64(hash, SHA_DIGEST_LENGTH);
        std::string sBuff(buff);
        std::string response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: " + sBuff + "\r\n\r\n";

        printf("%s\n", response.c_str());
        send(client, response.c_str(), response.size(), 0);
    }
    return;
}

int main() {
    WSADATA wsaData;

    int iResult;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup fialde: %d\n", iResult);
        return 1;
    }

    struct addrinfo* result = NULL, * ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ListenSocket = INVALID_SOCKET;

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult != 0) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    SOCKET ClientSocket;
    ClientSocket = INVALID_SOCKET;

    // Accept a client socket
    const int BUFFER_SIZE = 30720;
    int bytes = 0;

    printf("Server is listening on localhost:27015\n");

    while (ClientSocket == SOCKET_ERROR) {
        ClientSocket = accept(ListenSocket, NULL, NULL);
    }
    char buff[BUFFER_SIZE] = { 0 };

    bytes = recv(ClientSocket, buff, BUFFER_SIZE, 0);
    printf("-----------------------\n");
    // printf("req: %s\n", buff);
    printf("-----------------------\n");
    std::string request(buff);
    handshake(ClientSocket, request);
    printf("User connected!\n");
    bytes = 0;
    do {
        memset(buff, 0, BUFFER_SIZE);
        // printf("bytes: %d\n", bytes);
        bytes = recv(ClientSocket, buff, BUFFER_SIZE, 0);



        // printf("%s", buff);
        // printf("Bytes: \n", bytes);
    } while (bytes > 0);

    printf("\n\n");

    if (shutdown(ClientSocket, SD_SEND) != 0)
        printf("Something went wrong while closing connection: %ld\n", WSAGetLastError());

    else
        printf("Connection shutteddown\n");


    if (closesocket(ListenSocket) != 0)
        printf("Something went wrong while closing socket: %ld\n", WSAGetLastError());
    else
        printf("Socket closed!\n");

    if (WSACleanup() != 0)
        printf("Server: WSACleanup() failed!Error code : % ld\n, WSAGetLastError()");

    else
        printf("Server: WSACleanup() is OK...\n");
    return 0;
}