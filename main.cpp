#include "Effects/AmbientLight.h"
#include "Effects/ColorWheel.h"
#include "Effects/StaticColor.h"
#include "LedController.h"
#include "WebSocketServer.h"
#include <pipewire/pipewire.h>
#include <stdafx.h>
#include <termios.h>
// #include <winsock2.h>
//
// BOARD NAME = ESP32 ESP-WROOM CH350 WiFi Bluethooth 4.2

// const char *PORT_NAME = "\\\\.\\COM4"; //windows
const char *PORT_NAME = "/dev/ttyUSB0";
SerialPort *arduino = new SerialPort(PORT_NAME, B500000);

int main() {
    LedController *ledController = new LedController(arduino);
    WebSocketServer ws = WebSocketServer(ledController);

    if (ws.Initialize()) {
        printf("Server successfully initialzed!\n");
        ws.Start();
    }
    // if (ws.start() == false) {
    //     printf("Something went wrong!");
    // };
    // sleep(2);
    if (arduino->isConnected()) {
        // arduino->clearBuffer();
        printf("Device is ready\n");
    }
    char c;
    uint8_t r = 255, g = 0, b = 0;
    while (true) {
        std::cin >> c;
        switch (c) {
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
            ledController->addEffect(new ColorWheel(0.05));
            break;
        case '6':
            ledController->addEffect(new AmbientLight(TOP_LEDS, LEFT_LEDS, RIGHT_LEDS));
            break;
        }
    }

    // ws.stop();

    delete ledController;
    return 0;
}
