#include <winsock2.h>
#include "stdafx.h"
#include "LedController.h"
#include <stdio.h>
#include <WebSocket.h>
// #include "SerialPort.hpp"


const char* PORT_NAME = "\\\\.\\COM3";
SerialPort* arduino = new SerialPort(PORT_NAME);

int main() {
    LedController* ledController = new LedController(arduino);
    WebSocket ws = WebSocket(ledController);

    ws.Initialize();
    if (ws.start() == false) {
        printf("Something went wrong!");
    };
    ws.stop();


    delete ledController;
    return 0;
}