#pragma once
#include "stdafx.h"

// constans values
static const char *START_HEADER = "OSAS_START";
static const char *END_HEADER = "OSAS_END";
static const uint8_t START_HEADER_LEN = 10;
static const uint8_t END_HEADER_LEN = 8;

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
    static void sendStartHeader(SerialPort *pArduino);
    static void sendEndHeader(SerialPort *pArduino);
    static void sendSampleData(SerialPort *pArduino);
};