#include "Pixel.h"

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

    // std::cout << "R: " << (int)m_redColor << "\n";
}

void Pixel::HSVtoRGB(const float& H, const float& S, const float& V) {
    if (H > 360 || H < 0 || S>100 || S < 0 || V>100 || V < 0) {
        std::cout << "The givem HSV values are not in valid range" << std::endl;
        return;
    }
    float s = S / 100;
    float v = V / 100;
    float C = s * v;
    float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
    float m = v - C;
    float r, g, b;
    if (H >= 0 && H < 60) {
        r = C, g = X, b = 0;
    }
    else if (H >= 60 && H < 120) {
        r = X, g = C, b = 0;
    }
    else if (H >= 120 && H < 180) {
        r = 0, g = C, b = X;
    }
    else if (H >= 180 && H < 240) {
        r = 0, g = X, b = C;
    }
    else if (H >= 240 && H < 300) {
        r = X, g = 0, b = C;
    }
    else {
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

void Pixel::sendData(Pixel* pPixels, SerialPort* pArduino, const int& NUM_LEDS) {
    for (int i = 0; i < NUM_LEDS; i++) {
        Pixel::s_id = pPixels[i].m_id;
        Pixel::s_redColor = (uint8_t)pPixels[i].m_redColor;
        Pixel::s_greenColor = (uint8_t)pPixels[i].m_greenColor;
        Pixel::s_blueColor = (uint8_t)pPixels[i].m_blueColor;

        pArduino->writeSerialPort((char*)&Pixel::s_id, sizeof(uint8_t));
        pArduino->writeSerialPort((char*)&Pixel::s_redColor, sizeof(uint8_t));
        pArduino->writeSerialPort((char*)&Pixel::s_greenColor, sizeof(uint8_t));
        pArduino->writeSerialPort((char*)&Pixel::s_blueColor, sizeof(uint8_t));
    }
}