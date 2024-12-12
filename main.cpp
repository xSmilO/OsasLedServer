#include <stdio.h>
#include <WebSocket.h>

int main() {
    WebSocket ws;

    ws.Initialize();
    if (ws.start() == false) {
        printf("Something went wrong!");
    };
    ws.stop();
    return 0;
}