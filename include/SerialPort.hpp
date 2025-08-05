#pragma once
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

class SerialPort {
private:
  int fd;
  bool is_open;

public:
  SerialPort(const char *portName, size_t baudRate);
  ~SerialPort();

  int readSerialPort(const char *buffer, unsigned int buf_size);
  bool writeSerialPort(const char *buffer, size_t buf_size);
  bool isConnected();
  void closeSerial();
  void clearBuffer();
};
