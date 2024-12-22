#include <winsock2.h>
#include "stdafx.h"
#include "LedController.h"
#include <stdio.h>
#include "WebSocketServer.h" 
#include "FunctionQueue.h"

const char* PORT_NAME = "\\\\.\\COM3";
SerialPort* arduino = new SerialPort(PORT_NAME);

int main() {
    LedController* ledController = new LedController(arduino);
    WebSocketServer ws = WebSocketServer(ledController);

    ws.Initialize();
    if (ws.start() == false) {
        printf("Something went wrong!");
    };
    ws.stop();


    delete ledController;
    return 0;
}