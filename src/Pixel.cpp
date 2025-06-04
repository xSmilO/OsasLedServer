#include "Pixel.h"
#include <chrono>

#define FROM_NOW_MILLIS(a)                                                                                             \
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - a).count()

int Pixel::s_id = 0;
uint8_t Pixel::s_redColor = 0;
uint8_t Pixel::s_greenColor = 0;
uint8_t Pixel::s_blueColor = 0;

void Pixel::setColor(float red, float green, float blue) {
    m_redColor = red;
    m_greenColor = green;
    m_blueColor = blue;
}

void Pixel::setId(int id) {
    m_id = id;
}

void Pixel::fadeToBlack(float amt) {
    m_redColor -= m_redColor * (amt / 255);
    m_greenColor -= m_greenColor * (amt / 255);
    m_blueColor -= m_blueColor * (amt / 255);
}

void Pixel::HSVtoRGB(const float &H, const float &S, const float &V) {
    if(H > 360 || H < 0 || S > 100 || S < 0 || V > 100 || V < 0) {
        std::cout << "The givem HSV values are not in valid range" << std::endl;
        return;
    }
    float s = S / 100;
    float v = V / 100;
    float C = s * v;
    float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
    float m = v - C;
    float r, g, b;
    if(H >= 0 && H < 60) {
        r = C, g = X, b = 0;
    } else if(H >= 60 && H < 120) {
        r = X, g = C, b = 0;
    } else if(H >= 120 && H < 180) {
        r = 0, g = C, b = X;
    } else if(H >= 180 && H < 240) {
        r = 0, g = X, b = C;
    } else if(H >= 240 && H < 300) {
        r = X, g = 0, b = C;
    } else {
        r = C, g = 0, b = X;
    }
    m_redColor = (r + m) * 255;
    m_greenColor = (g + m) * 255;
    m_blueColor = (b + m) * 255;
}

int Pixel::getId() {
    return m_id;
}

float Pixel::getRedColor() {
    return m_redColor;
}

float Pixel::getGreenColor() {
    return m_greenColor;
}

float Pixel::getBlueColor() {
    return m_blueColor;
}

void Pixel::sendData(Pixel *pPixels, SerialPort *pArduino, const int &NUM_LEDS) {
    if(!pArduino->isConnected())
        return;
    char *frameData = new char[NUM_LEDS * 4];
    const uint8_t CHUNK_SIZE = 64;
    const uint16_t TOTAL_SIZE = NUM_LEDS * 4;
    char ack = '0';

    // fill up frameData
    size_t idx = 0;
    for(size_t i = 0; i < NUM_LEDS; ++i) {
        frameData[idx++] = pPixels[i].m_id;
        frameData[idx++] = pPixels[i].m_redColor;
        frameData[idx++] = pPixels[i].m_greenColor;
        frameData[idx++] = pPixels[i].m_blueColor;
    }

    // send start header 1
    ack = '0';
    do {
        // printf("chuj 1\n");
        while(!pArduino->writeSerialPort(&HEADER1, 1))
            ;
        Sleep(10);
    } while(pArduino->readSerialPort(&ack, 1) <= 0 || ack != '1');

    while(!pArduino->writeSerialPort(&HEADER2, 1))
        ;
    ack = '0';

    uint8_t debugByte;
    // clearing buffer
    while(!pArduino->readSerialPort((char *)&debugByte, 1) != 0) {
        if((char)debugByte != '1')
            break;
    }

    uint8_t lastChunkIdx = 0;
    bool shouldResent = false;

    printf("========================\n");
    for(size_t i = 0; i < TOTAL_SIZE; i += CHUNK_SIZE) {
        uint16_t remaining = TOTAL_SIZE - i;
        uint16_t sizeToSend = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;

        while(!pArduino->writeSerialPort(frameData + i, sizeToSend))
            ;

        printf("|| SENDING : %d \n", sizeToSend);
        // need to check in what bytes arduino stuck and resend them!
        auto begin = std::chrono::high_resolution_clock::now();
        lastChunkIdx = 0;
        shouldResent = true;
        while(pArduino->readSerialPort((char *)&debugByte, 1) != 0 || FROM_NOW_MILLIS(begin) < 300) {
            if(debugByte == 69) {
                printf("RESET CHUNK IDX\n");
            } else {
                printf("CHUNK IDX: %d | ", debugByte);
            }

            if(debugByte == 0) {
                printf("ELAPSED: %ld", FROM_NOW_MILLIS(begin));
                printf(" LAST BYTE: %d", lastChunkIdx);

            } else
                lastChunkIdx = debugByte;

            printf("\n");
            if(debugByte == sizeToSend) {
                printf("SENDING NEXT PACKET\n");
                shouldResent = false;
                break;
            }
        }

        // resend remaining non kurwa processed chunk do chuja bo arduino to gowno jeabne
        if(shouldResent) {
            printf("RESENDING %d Bytes\n", CHUNK_SIZE - lastChunkIdx);
            while(!pArduino->writeSerialPort(frameData + i + lastChunkIdx, CHUNK_SIZE - lastChunkIdx))
                ;
        }

        printf("TIMES OUT\n");
    }
    printf("========================\n");

    printf("udalo sie\n");

    delete[] frameData;
}