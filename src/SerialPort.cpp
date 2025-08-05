#include "SerialPort.hpp"
#include <asm-generic/ioctls.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

SerialPort::SerialPort(const char *portName, size_t baudRate) {
  fd = open("/dev/ttyUSB0", O_RDWR);
  // clear buffer after opening
  if (fd < 0) {
    printf("Error %i from open: %s\n", errno, strerror(errno));
    is_open = false;
  }
  sleep(2);

  tcflush(fd, TCIOFLUSH);
  is_open = true;

  termios tty;

  if (tcgetattr(fd, &tty) != 0) {
    printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
  }

  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag |= CS8;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag |= CREAD | CLOCAL;

  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO;
  tty.c_lflag &= ~ECHOE;
  tty.c_lflag &= ~ECHONL;
  tty.c_lflag &= ~ISIG;

  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

  tty.c_oflag &= ~OPOST;
  tty.c_oflag &= ~ONLCR;

  tty.c_cc[VTIME] = 2;
  tty.c_cc[VMIN] = 0;

  cfsetspeed(&tty, baudRate);

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    is_open = false;
    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
  }
}

SerialPort::~SerialPort() { close(fd); }
void SerialPort::closeSerial() { close(fd); }
bool SerialPort::isConnected() { return is_open; }

int SerialPort::readSerialPort(const char *buffer, unsigned int buf_size) {
  size_t toRead = 0;
  size_t cbInQue = 0;

  ioctl(fd, FIONREAD, &cbInQue);

  if (cbInQue > buf_size) {
    toRead = buf_size;
  } else {
    toRead = cbInQue;
  }

  memset((void *)buffer, 0, buf_size);
  return read(fd, (void *)buffer, toRead);
}

bool SerialPort::writeSerialPort(const char *buffer, size_t buf_size) {
  write(fd, (void *)buffer, buf_size);
  return true;
}

void SerialPort::clearBuffer() {
  uint8_t byte = 0;
  while(readSerialPort((const char*)&byte, 1) > 0) {
    printf("buf ready\n");
  }
}
