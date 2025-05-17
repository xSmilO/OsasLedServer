#include "FunctionQueue.h"
#include "LedController.h"
#include "WebSocketServer.h"
#include "stdafx.h"
#include <stdio.h>
#include <winsock2.h>

const char *PORT_NAME = "\\\\.\\COM3";
SerialPort *arduino = new SerialPort(PORT_NAME);

int main() {
    LedController *ledController = new LedController(arduino);
    WebSocketServer ws = WebSocketServer(ledController);

    // ws.Initialize();
    // if (ws.start() == false) {
    //     printf("Something went wrong!");
    // };
    char c;
    uint8_t r = 255, g = 0, b = 0;
    while(true) {
        std::cin >> c;
        switch(c) {
        case '1':
            r = 255;
            g = 0;
            b = 0;
            break;
        case '2':
            r = 0;
            g = 255;
            b = 0;
            break;
        case '3':
            r = 0;
            g = 0;
            b = 255;
            break;
        case '4':
            r = 0;
            g = 0;
            b = 0;
            break;
        }

        ledController->addEffect(new StaticColor(r, g, b));
        continue;
        // 1 - for start header
        if(GetAsyncKeyState(0x31)) {
            Pixel::sendStartHeader(arduino);
            printf("Start header\n");
        }

        // 2 - for end header
        if(GetAsyncKeyState(0x32)) {
            Pixel::sendEndHeader(arduino);
            printf("End header\n");
        }

        // 3 - sample data
        if(GetAsyncKeyState(0x33)) {
            Pixel::sendSampleData(arduino);
            printf("sample data\n");
        }

        // 4 - send 1 byte
        if(GetAsyncKeyState(0x34)) {
            arduino->writeSerialPort("", 1);
        }
    }

    // ws.stop();

    delete ledController;
    return 0;
}