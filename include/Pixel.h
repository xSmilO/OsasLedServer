#pragma once
#include "stdafx.h"

// constans values
static const char HEADER1 = 0xAA;
static const char HEADER2 = 0x55;

class Pixel {
  protected:
    float m_redColor = 0;
    float m_greenColor = 0;
    float m_blueColor = 0;
    int m_hue;
    int m_id;

    static int s_id;
    static uint8_t s_redColor;
    static uint8_t s_greenColor;
    static uint8_t s_blueColor;

  public:
    void setColor(float red, float green, float blue);
    void setId(int id);
    int getId();
    float getRedColor();
    float getBlueColor();
    float getGreenColor();
    void fadeToBlack(float amt);
    void HSVtoRGB(const float &H, const float &S, const float &V);

    static void sendData(Pixel *pPixels, SerialPort *pArduino, const int &NUM_LEDS);
};