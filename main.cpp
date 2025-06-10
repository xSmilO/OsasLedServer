#include "FunctionQueue.h"
#include "LedController.h"
#include "WebSocketServer.h"
#include "stdafx.h"
#include <stdio.h>
#include <winsock2.h>

const char *PORT_NAME = "\\\\.\\COM4";
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
            ledController->addEffect(new StaticColor(r, g, b));
            break;
        case '2':
            r = 0;
            g = 255;
            b = 0;
            ledController->addEffect(new StaticColor(r, g, b));
            break;
        case '3':
            r = 0;
            g = 0;
            b = 255;
            ledController->addEffect(new StaticColor(r, g, b));
            break;
        case '4':
            r = 0;
            g = 0;
            b = 0;
            ledController->addEffect(new StaticColor(r, g, b));
            break;
        case '5':
            ledController->addEffect(new ColorWheel(0.1));
            break;
        }
    }

    // ws.stop();

    delete ledController;
    return 0;
}