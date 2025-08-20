#include "WebSocketServer.h"
#include <stdio.h>

#define BITVALUE(X,N) (((X) >> (N)) & 0x1)

void printEachBitOfByte(const char& byte, size_t bits) {
    for (int i = bits - 1; i >= 0; --i) {
        printf("%d ", BITVALUE(byte, i));
    };
    printf("\n");
    return;
}

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


char* WebSocketServer::base64(const unsigned char* input, int length) {
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

void WebSocketServer::handshake(SOCKET_INFORMATION* client) {
    printf("Perfoming handshake\n");
    client->handshakeDone = true;
    std::string request(client->DataBuf.buf);
    std::string SecKey = "Sec-WebSocket-Accept: ";
    size_t keyPos = request.find("Sec-WebSocket-Key: ");
    if (keyPos == std::string::npos) {
        keyPos = request.find("sec-websocket-key: ");
        SecKey = "sec-websocket-accept: ";
    }
    std::string SecValue = "";

    int keyOffset = 19;
    // printf("Handshake: %s\n", request.c_str());
    if (keyPos != std::string::npos) {
        size_t endPos = request.find("\r\n", keyPos);

        SecValue = request.substr(keyPos + keyOffset, endPos - keyPos - keyOffset);
        SecValue += GUID;
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(SecValue.c_str()), SecValue.size(), hash);

        char* buff = base64(hash, SHA_DIGEST_LENGTH);
        std::string sBuff(buff);
        std::string response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            // "Sec-WebSocketServer-Version: 13\r\n"
            "Connection: Upgrade\r\n"
            "" + SecKey + sBuff + "\r\n\r\n";

        printf("Sending handshake!\n");
        strcpy(client->Buffer, response.c_str());
        client->DataBuf.buf = client->Buffer;
        client->DataBuf.len = response.length();
        WSASend(client->Socket, &(client->DataBuf), 1, &_SendBytes, 0, &(client->Overlapped), NULL);
    }
    return;
}

uint64_t WebSocketServer::getPayloadLength(const char* buffer, uint8_t& offset) {
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

bool WebSocketServer::isFrameComplete(SOCKET_INFORMATION* client) {
    uint8_t offset = 1;
    size_t payloadLength = getPayloadLength(client->Buffer, offset);
    size_t frameSize = 0;
    // mask bit
    if (BITVALUE(client->Buffer[1], 7) > 0) {
        // mask key is length of 4 bytes (32bit)
        frameSize += 4;
    }
    frameSize += offset;
    frameSize += payloadLength;

    return client->BytesRECV >= frameSize;
}

bool WebSocketServer::isFrameValid(SOCKET_INFORMATION* client) {
    // int off = 0;
    // printf("RSV: %d %d %d \n", BITVALUE(client->Buffer[0], ++off), BITVALUE(client->Buffer[0], ++off), BITVALUE(client->Buffer[0], ++off));
    // printf("RSV value: %d\n", client->Buffer[0] & 0x70);
    return (client->Buffer[0] & 0x70) == 0;
}

void WebSocketServer::showFrameMetadata(SOCKET_INFORMATION* client) {
    uint8_t offset = 1; // skip first byte 
    printf("****** FRAME METADATA ******\n");
    printf("FIN:  %d\n", BITVALUE(client->Buffer[0], 7));
    printf("RSV:  %d %d %d \n", BITVALUE(client->Buffer[0], 6), BITVALUE(client->Buffer[0], 5), BITVALUE(client->Buffer[0], 4));
    printf("OPCODE:  0x%01x\n", client->Buffer[0] & 0xF);
    printf("MASK: %d\n", BITVALUE(client->Buffer[1], 7));
    printf("PL length: %ld\n", getPayloadLength(client->Buffer, offset));
    return;
}

void WebSocketServer::getPayloadData(SOCKET_INFORMATION* client) {
    uint8_t offset = 0;

    offset++;
    uint64_t payloadLength = getPayloadLength(&client->Buffer[client->offset], offset);

    char* payloadData = new char[payloadLength];
    if (BITVALUE(client->Buffer[client->offset + 1], 7) != 0) {
        uint32_t* maskingKey = (uint32_t*)&client->Buffer[offset + client->offset];
        offset += 4;

        size_t j = 0;
        uint8_t* original = (uint8_t*)&client->Buffer[offset + client->offset];
        uint8_t* masking = (uint8_t*)maskingKey;
        for (size_t i = 0; i < payloadLength; ++i) {
            j = i % 4;
            payloadData[i] = original[i] ^ masking[j];
        }
        client->offset += 4;
    }
    else {
        uint8_t* original = (uint8_t*)&client->Buffer[offset + client->offset];
        for (size_t i = 0; i < payloadLength; ++i) {
            payloadData[i] = original[i];
        }
    }
    memcpy(client->receivedMessage, payloadData, payloadLength);
    client->receivedMessage[payloadLength] = '\0';
    // memmove(&client->Buffer, &client->Buffer[payloadLength + 4 + 2], DATA_BUFSIZE - payloadLength - 4 - 2);
    client->offset += payloadLength + 2;
    delete payloadData;
    return;
}

void WebSocketServer::closeConnectionWith(SOCKET_INFORMATION* client, size_t index) {
    if (closesocket(client->Socket) == SOCKET_ERROR) {
        printf("closesocket() failed with error %d\n", WSAGetLastError());
    }
    else {
        printf("closesocket() is ok!\n");
    }

    WSACloseEvent(EventArray[index - WSA_WAIT_EVENT_0]);

    for (size_t i = index - WSA_WAIT_EVENT_0; i < eventTotal; i++) {
        EventArray[i] = EventArray[i + 1];
        SocketArray[i] = SocketArray[i + 1];
    }
    free(client);
    eventTotal--;
}
void WebSocketServer::handleRequests() {
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
        SI->Buffer[SI->BytesRECV] = '\0';
        printf("BT: %d | %d\n", SI->BytesRECV, BytesTransferred);

        printf("%d | %d\n", isFrameComplete(SI), isFrameValid(SI));

        if (isFrameComplete(SI) || !isFrameValid(SI)) {
            while (isFrameValid(SI) && SI->offset < SI->BytesRECV) {
                // Sleep(LED_CALL_DELAY); // slight delay for leds because they can't handle intense traffic
                // showFrameMetadata(SI);
                getPayloadData(SI);
                printBytes((uint8_t*)SI->receivedMessage);
                printf("\n");
                controlLights((uint8_t*)SI->receivedMessage);
            }
            printf("Recv: %d\n", SI->BytesRECV);
            SI->BytesRECV = 0;
            SI->offset = 0;
        }
        Flags = 0;
        SI->DataBuf.buf = &SI->Buffer[SI->BytesRECV];
        SI->DataBuf.len = DATA_BUFSIZE - SI->BytesRECV;

        ZeroMemory(&SI->Overlapped, sizeof(WSAOVERLAPPED));
        SI->Overlapped.hEvent = EventArray[Index - WSA_WAIT_EVENT_0];
        if (WSARecv(SI->Socket, &SI->DataBuf, 1, &_RecvBytes, &Flags, &SI->Overlapped, NULL) == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAECONNRESET) {
                closeConnectionWith(SI, Index);
            }
            else if (WSAGetLastError() != WSA_IO_PENDING) {
                printf("Something went wrong with WSARecv: %d\n", WSAGetLastError());
            }
        }
    }
    return;
}


void WebSocketServer::serverSend(SOCKET_INFORMATION* client, DWORD& evtIdx, std::string& data) {
    std::vector<uint8_t> frame = {};
    createSendResponse(frame, data);
    memcpy(client->Buffer, frame.data(), frame.size());
    client->DataBuf.buf = client->Buffer;
    client->DataBuf.len = frame.size();
    // printf("idx: %d\n", evtIdx);
    ZeroMemory(&client->Overlapped, sizeof(WSAOVERLAPPED));
    client->Overlapped.hEvent = EventArray[evtIdx - WSA_WAIT_EVENT_0];
    if (WSASend(client->Socket, &client->DataBuf, 1, &_SendBytes, 0, &client->Overlapped, NULL) == SOCKET_ERROR) {
        if (WSAGetLastError() != WSA_IO_PENDING) {
            printf("Something went wrong while sending message: %ld\n", WSAGetLastError());
        }
    };
}

void WebSocketServer::createSendResponse(std::vector<uint8_t>& frame, std::string& message) {
    frame.clear();
    size_t payloadLength = message.size();
    uint8_t firstByte = 0x80 | 0x1;
    frame.push_back(firstByte);

    uint8_t secondByte = 0;

    if (payloadLength < 126) {
        secondByte |= payloadLength;
        frame.push_back(secondByte);
    }
    else if (payloadLength <= 0xFFFF) {
        secondByte |= 126;
        frame.push_back(secondByte);
        uint16_t length = payloadLength;

        printEachBitOfByte(length, 16);
        frame.push_back(static_cast<uint8_t>(length >> 8));
        frame.push_back(static_cast<uint8_t>(length & 0xFF));
    }
    else {
        secondByte |= 127;
        frame.push_back(secondByte);
        uint64_t length = payloadLength;
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<uint8_t>((length >> (i * 8)) & 0xFF));
        }
    }
    frame.insert(frame.end(), message.begin(), message.end());
    return;
}

void WebSocketServer::printBytes(uint8_t* bytes) {
    for (; *bytes != '\0'; ++bytes) {
        printf("%02x ", *bytes);
    }
}

void WebSocketServer::controlLights(uint8_t* bytes) {
    uint8_t* header = &bytes[0];

    //static color effect
    if (*header == 0x1) {
        uint8_t* colors = &bytes[1];
        // _lc->addCall(&LedController::Static, *colors, *(colors + 1), *(colors + 2));
    }
}

DWORD __stdcall WebSocketServer::listenForRequest(LPVOID lpParameter) {
    WebSocketServer* self = static_cast<WebSocketServer*>(lpParameter);
    self->handleRequests();
    return 0;
}

bool WebSocketServer::Initialize() {
    WSAData wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "Server::Initialize: WSAStartup failed.\n");
        return false;
    }


    return true;
}

bool WebSocketServer::start() {
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

    if (CreateThread(NULL, 0, WebSocketServer::listenForRequest, (LPVOID)this, 0, &listenThreadID) == NULL) {
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
        SocketArray[eventTotal]->offset = 0;

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

        //increment eventTotalCount
        eventTotal++;

        if (WSASetEvent(EventArray[0]) == false) {
            fprintf(stderr, "WSASetEvent() failde with error: %d\n", WSAGetLastError());
            return false;
        }
    }

    return true;
}

void WebSocketServer::stop() {
    if (_running) {
        _running = false;

        // close all client socktes!

        closesocket(server);
        WSACleanup();
    }
}
