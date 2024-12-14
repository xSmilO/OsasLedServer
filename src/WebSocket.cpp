#include <WebSocket.h>
#include <stdio.h>

#define BITVALUE(X,N) (((X) >> (N)) & 0x1)

uint64_t reverseBytes(uint64_t value) {
    return ((value & 0x00000000000000FFULL) << 56) |
        ((value & 0x000000000000FF00ULL) << 40) |
        ((value & 0x0000000000FF0000ULL) << 24) |
        ((value & 0x00000000FF000000ULL) << 8) |
        ((value & 0x000000FF00000000ULL) >> 8) |
        ((value & 0x0000FF0000000000ULL) >> 24) |
        ((value & 0x00FF000000000000ULL) >> 40) |
        ((value & 0xFF00000000000000ULL) >> 56);
}


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
            // "Sec-WebSocket-Version: 13\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: " + sBuff + "\r\n\r\n";

        printf("Sending handshake!\n");
        strcpy(client->Buffer, response.c_str());
        client->DataBuf.buf = client->Buffer;
        client->DataBuf.len = response.length();
        WSASend(client->Socket, &(client->DataBuf), 1, &_SendBytes, 0, &(client->Overlapped), NULL);
        // WSAResetEvent(client->Overlapped.hEvent);
    }
    return;
}

void printEachBitOfByte(const char& byte, size_t bits) {
    for (int i = 0; i < bits; ++i) {
        printf("%d ", BITVALUE(byte, i));
    };
    printf("\n");
    return;
}

void WebSocket::interpretData(SOCKET_INFORMATION* client) {
    // printf("data = %s\n", client->Buffer);
    size_t offset = 0;
    if (BITVALUE(client->Buffer[0], offset) == 0) return;
    printf("=============BYTE 1==============\n");
    printEachBitOfByte(client->Buffer[0], sizeof(char) * 8);
    //FIN
    printf("FIN bit: %d\n", BITVALUE(client->Buffer[0], offset));

    // RSV1,RSV2,RSV3
    printf("RSV: %d %d %d \n", BITVALUE(client->Buffer[0], ++offset), BITVALUE(client->Buffer[0], ++offset), BITVALUE(client->Buffer[0], ++offset));
    //Opcode 
    int opcodeMask = 0x0F;

    printf("OPCODE: 0x%04x\n", client->Buffer[0] & opcodeMask);

    offset = 0;
    printf("=============BYTE 2==============\n");
    printEachBitOfByte(client->Buffer[1], sizeof(char) * 8);
    // MASK bit
    printf("Mask bit: %d\n", BITVALUE(client->Buffer[1], offset));

    // Payload length
    offset = 1;
    uint64_t payloadLength = getPayloadLength(client->Buffer, offset);
    printf("Payload length: %ld\n", payloadLength);
    printf("Offset: %d\n", offset);

    // masking key

    int32_t* maskingKey = (int32_t*)&client->Buffer[offset];
    printEachBitOfByte(client->Buffer[offset], 32);
    offset += 4;

    // printf("%d | %d | %d | %d\n", payloadLength, payloadLength / 8, (payloadLength / 8) * 8, payloadLength - ((payloadLength / 8) * 8));
    // size_t octets = payloadLength / 8;
    // if (payloadLength - ((payloadLength / 8) * 8) > 0)
    //     octets++;
    // printf("%d\n", octets);
    size_t j = 0;
    int8_t* original = (int8_t*)&client->Buffer[offset];
    int8_t* masking = (int8_t*)maskingKey;
    char* payloadData = new char[payloadLength + 1];
    payloadData[payloadLength] = '\0';
    for (size_t i = 0; i < payloadLength; ++i) {
        j = i % 4;
        payloadData[i] = original[i] ^ masking[j];
    }

    printf("text: %s\n", payloadData);
    return;
}

uint64_t WebSocket::getPayloadLength(const char* buffer, size_t& offset) {
    uint8_t firstByte = buffer[offset];
    uint8_t payloadLen = firstByte & 0x7F;
    offset++;

    if (payloadLen <= 125) {
        return payloadLen;
    }
    else if (payloadLen == 126) {
        uint16_t length = 0;
        std::memcpy(&length, buffer + offset, sizeof(length));
        length = ntohs(length);
        offset += 2;
        return length;
    }
    else if (payloadLen == 127) {
        uint64_t length;
        std::memcpy(&length, buffer + offset, sizeof(length));
        length = reverseBytes(length);
        offset += 8;
        return length;
    }
    return 0;
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
    DWORD Index, Flags, i;
    SOCKET_INFORMATION* SI;
    DWORD BytesTransferred;

    while (true) {
        Index = WSAWaitForMultipleEvents(eventTotal, EventArray, FALSE, WSA_INFINITE, FALSE);
        if (Index == WSA_WAIT_FAILED) {
            fprintf(stderr, "WSAWaitForMultipleEvents() failed %d\n", WSAGetLastError());
            continue;
        }
        if ((Index - WSA_WAIT_EVENT_0) == 0) {
            printf("User connected!\n");
            WSAResetEvent(EventArray[0]);
            continue;
        }

        SI = SocketArray[Index - WSA_WAIT_EVENT_0];
        WSAResetEvent(EventArray[Index - WSA_WAIT_EVENT_0]);

        if (WSAGetOverlappedResult(SI->Socket, &SI->Overlapped, &BytesTransferred, FALSE, &Flags) == false || BytesTransferred == 0) {
            printf("Closing socket %d\n", SI->Socket);
            if (closesocket(SI->Socket) == SOCKET_ERROR) {
                printf("closesocket() failed with error %d\n", WSAGetLastError());
            }
            else {
                printf("closesocket() is ok!\n");
            }

            WSACloseEvent(EventArray[Index - WSA_WAIT_EVENT_0]);

            for (i = Index - WSA_WAIT_EVENT_0; i < eventTotal; i++) {
                EventArray[i] = EventArray[i + 1];
                SocketArray[i] = SocketArray[i + 1];
            }
            free(SI);
            eventTotal--;
            continue;
        }

        if (!SI->handshakeDone) {
            handshake(SI);
            continue;
        }

        SI->BytesRECV += BytesTransferred;
        // printf("%d\n", EventArray[Index - WSA_WAIT_EVENT_0]);

        SI->Buffer[SI->BytesRECV] = '\0';
        // printf("BT = %d\n", BytesTransferred);
        // printf("Recv: %s\n", SI->DataBuf.buf);


        interpretData(SI);
        ZeroMemory(&SI->Overlapped, sizeof(WSAOVERLAPPED));
        SI->Overlapped.hEvent = EventArray[Index - WSA_WAIT_EVENT_0];
        Flags = 0;
        SI->BytesRECV = 0;
        SI->DataBuf.buf = SI->Buffer;
        SI->DataBuf.len = DATA_BUFSIZE;

        if (WSARecv(SI->Socket, &SI->DataBuf, 1, &_RecvBytes, &Flags, &SI->Overlapped, NULL) == SOCKET_ERROR) {
            if (WSAGetLastError() != WSA_IO_PENDING) {
                printf("Something went wrong with WSARecv: %d\n", WSAGetLastError());
            }
        }
    }
    return;
}