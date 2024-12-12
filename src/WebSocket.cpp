#include <WebSocket.h>
#include <stdio.h>

char* WebSocket::base64(const unsigned char* input, int length) {
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

void WebSocket::handshake(SOCKET_INFORMATION* client) {
    printf("Perfoming handshake\n");
    client->handshakeDone = true;
    std::string request(client->DataBuf.buf);
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

        // printf("%s\n", response.c_str());
        client->DataBuf.buf = (char*)response.c_str();
        client->DataBuf.len = response.length();
        WSASend(client->Socket, &(client->DataBuf), 1, &_SendBytes, 0, &(client->Overlapped), NULL);
    }
    return;
}

DWORD __stdcall WebSocket::listenForRequest(LPVOID lpParameter) {
    WebSocket* self = static_cast<WebSocket*>(lpParameter);
    self->handleRequests();
    return 0;
}

bool WebSocket::Initialize() {
    WSAData wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "Server::Initialize: WSAStartup failed.\n");
        return false;
    }


    return true;
}

bool WebSocket::start() {
    if ((server = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == SOCKET_ERROR) {
        fprintf(stderr, "Server::Start: WSASocket failed. %d\n", WSAGetLastError());
        return false;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(server, (PSOCKADDR)&serverAddr, sizeof(sockaddr_in)) == SOCKET_ERROR) {
        fprintf(stderr, "Server::Start: bind failed. %d\n", WSAGetLastError());
        return false;
    }

    if (listen(server, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "Server::Start: listen failed. %d\n", WSAGetLastError());
        return false;
    }

    if ((acceptSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
        fprintf(stderr, "Server::Start: accept socket creation failed. %d\n", WSAGetLastError());
        return false;
    }

    if ((EventArray[0] = WSACreateEvent()) == WSA_INVALID_EVENT) {
        fprintf(stderr, "WSACreateEvent() failed with code: %d\n", WSAGetLastError());
        return false;
    }

    if (CreateThread(NULL, 0, WebSocket::listenForRequest, (LPVOID)this, 0, &listenThreadID) == NULL) {
        fprintf(stderr, "Server::Start: thread creation failed. %d\n", GetLastError());
        return false;
    }

    eventTotal = 1;

    while (true) {
        if ((acceptSocket = accept(server, NULL, NULL)) == INVALID_SOCKET) {
            fprintf(stderr, "Server::Start: accept() failed: %d\n", WSAGetLastError());
            return false;
        }
        SocketArray[eventTotal] = new SOCKET_INFORMATION;
        SocketArray[eventTotal]->Socket = acceptSocket;
        ZeroMemory(&(SocketArray[eventTotal]->Overlapped), sizeof(OVERLAPPED));
        SocketArray[eventTotal]->BytesSEND = 0;
        SocketArray[eventTotal]->BytesRECV = 0;
        SocketArray[eventTotal]->DataBuf.buf = SocketArray[eventTotal]->Buffer;
        SocketArray[eventTotal]->DataBuf.len = DATA_BUFSIZE;
        SocketArray[eventTotal]->handshakeDone = false;

        if ((SocketArray[eventTotal]->Overlapped.hEvent = EventArray[eventTotal] = WSACreateEvent()) == WSA_INVALID_EVENT) {
            fprintf(stderr, "WSACreateEvent() failed with error %d\n", WSAGetLastError());
            return false;
        }

        flags = 0;
        if (WSARecv(SocketArray[eventTotal]->Socket, &(SocketArray[eventTotal]->DataBuf), 1, &_RecvBytes, &flags, &(SocketArray[eventTotal]->Overlapped), NULL) == SOCKET_ERROR) {
            if (WSAGetLastError() != ERROR_IO_PENDING) {
                fprintf(stderr, "WSARecv() failed. %d\n", WSAGetLastError());
                return false;
            }
        }

        eventTotal++;

        if (WSASetEvent(EventArray[0]) == false) {
            fprintf(stderr, "WSASetEvent() failde with error: %d\n", WSAGetLastError());
            return false;
        }
    }

    return true;
}

void WebSocket::stop() {
    if (_running) {
        _running = false;

        // close all client socktes!

        closesocket(server);
        WSACleanup();
    }
}

void WebSocket::handleRequests() {
    printf("Handle request!\n");
    DWORD Index;
    DWORD Flags;
    SOCKET_INFORMATION* SI;
    DWORD BytesTransferred;
    DWORD i;
    while (true) {
        if ((Index = WSAWaitForMultipleEvents(eventTotal, EventArray, FALSE, WSA_INFINITE, false)) == WSA_WAIT_FAILED) {
            fprintf(stderr, "WSAWaitForMultipleEvents() failed %d\n", WSAGetLastError());
            return;
        }
        else {
            // printf("WSAWaitForMultipleEvents() is OK!\n");
        }


        if ((Index - WSA_WAIT_EVENT_0) == 0) {
            WSAResetEvent(EventArray[0]);
            // printf("FirstEvent\n");
            continue;
        }

        SI = SocketArray[Index - WSA_WAIT_EVENT_0];
        WSAResetEvent(EventArray[Index - WSA_WAIT_EVENT_0]);

        if (WSAGetOverlappedResult(SI->Socket, &(SI->Overlapped), &BytesTransferred, FALSE, &Flags) == false || BytesTransferred == 0) {
            printf("Closing socket %d\n", SI->Socket);
            if (closesocket(SI->Socket) == SOCKET_ERROR) {
                printf("closesocket() failed with error %d\n", WSAGetLastError());
            }
            else {
                printf("closesocket() is ok!\n");
            }

            free(SI);

            WSACloseEvent(EventArray[Index - WSA_WAIT_EVENT_0]);

            if ((Index - WSA_WAIT_EVENT_0) + 1 != eventTotal) {
                for (i = Index - WSA_WAIT_EVENT_0; i < eventTotal; i++) {
                    EventArray[i] = EventArray[i + 1];
                    SocketArray[i] = SocketArray[i + 1];
                }
            }

            eventTotal--;
            continue;
        }

        //first received message
        if (SI->BytesRECV == 0) {

            SI->BytesRECV = BytesTransferred;
            SI->BytesSEND = 0;

            //handshake
            if (SI->handshakeDone == false)
                handshake(SI);
        }
        else {
            SI->BytesSEND += BytesTransferred;
        }

        if (SI->BytesRECV > SI->BytesSEND) {
            ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));
            printf("Recv: %d | Send: %d\n", SI->BytesRECV, SI->BytesSEND);
            SI->Overlapped.hEvent = EventArray[Index - WSA_WAIT_EVENT_0];

            SI->DataBuf.buf = SI->Buffer + SI->BytesSEND;
            SI->DataBuf.len = SI->BytesRECV - SI->BytesSEND;

            printf("Sending bytes: %s\n", SI->DataBuf.buf);
            if (WSASend(SI->Socket, &(SI->DataBuf), 1, &_SendBytes, 0, &(SI->Overlapped), NULL) == SOCKET_ERROR) {
                if (WSAGetLastError() != ERROR_IO_PENDING) {
                    fprintf(stderr, "WSASend() failed with error %d\n", WSAGetLastError());
                    return;
                }
            }
            else {
                printf("WSASend() is ok!\n");
            }
        }
        else {
            SI->BytesRECV = 0;

            Flags = 0;
            ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));
            SI->Overlapped.hEvent = EventArray[Index - WSA_WAIT_EVENT_0];

            SI->DataBuf.len = DATA_BUFSIZE;
            SI->DataBuf.buf = SI->Buffer;
            printf("simea\n");

            if (WSARecv(SI->Socket, &(SI->DataBuf), 1, &_RecvBytes, &Flags, &(SI->Overlapped), NULL) == SOCKET_ERROR) {
                if (WSAGetLastError() != ERROR_IO_PENDING) {
                    fprintf(stderr, "WSARecv() failed with error %d\n", WSAGetLastError());
                    return;
                }
            }
            else {
                printf("WSARecv() is OK!\n");
                printf("Buf: %s\n", SI->DataBuf.buf);
                // check what is the request and do handshake
                // handshake(SI, std::string(SI->DataBuf.buf));
                ZeroMemory(&SI->DataBuf, sizeof(WSABUF));
            }
        }
    }
    return;
}