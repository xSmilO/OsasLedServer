/*
 * Author: BananaBoii600
 * Modified Library introduced in Arduino Playground which does not work
 * This works perfectly
 * LICENSE: MIT
 */

#include "SerialPort.hpp"

SerialPort::SerialPort(const char *portName) {
    this->connected = false;

    this->handler = CreateFileA(static_cast<LPCSTR>(portName), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(this->handler == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if(err == ERROR_FILE_NOT_FOUND) {
            std::cerr << "ERROR: Handle was not attached.Reason : " << portName << " not available\n";
        } else if(err == ERROR_ACCESS_DENIED) {
            std::cerr << "ERROR: Access denied!\n";
        } else {
            printf("ERROR: %ld\n", err);
            std::cerr << "ERROR!!!\n";
        }
    } else {
        DCB dcbSerialParameters = {0};

        if(!GetCommState(this->handler, &dcbSerialParameters)) {
            std::cerr << "Failed to get current serial parameters\n";
        } else {
            dcbSerialParameters.BaudRate = 250000;
            dcbSerialParameters.ByteSize = 8;
            dcbSerialParameters.StopBits = ONESTOPBIT;
            dcbSerialParameters.Parity = NOPARITY;
            dcbSerialParameters.fDtrControl = DTR_CONTROL_ENABLE;

            if(!SetCommState(handler, &dcbSerialParameters)) {
                std::cout << "ALERT: could not set serial port parameters\n";
            } else {
                this->connected = true;
                PurgeComm(this->handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
                Sleep(ARDUINO_WAIT_TIME);
            }
        }
    }
}

SerialPort::~SerialPort() {
    if(this->connected) {
        this->connected = false;
        CloseHandle(this->handler);
    }
}

// Reading bytes from serial port to buffer;
// returns read bytes count, or if error occurs, returns 0
int SerialPort::readSerialPort(const char *buffer, unsigned int buf_size) {
    DWORD bytesRead{};
    unsigned int toRead = 0;

    ClearCommError(this->handler, &this->errors, &this->status);

    if(this->status.cbInQue > 0) {
        if(this->status.cbInQue > buf_size) {
            toRead = buf_size;
        } else {
            toRead = this->status.cbInQue;
        }
    }

    memset((void *)buffer, 0, buf_size);

    if(ReadFile(this->handler, (void *)buffer, toRead, &bytesRead, NULL)) {
        return bytesRead;
    }

    return 0;
}

// Sending provided buffer to serial port;
// returns true if succeed, false if not
bool SerialPort::writeSerialPort(const char *buffer, unsigned int buf_size) {
    DWORD bytesSend;

    if(!WriteFile(this->handler, (void *)buffer, buf_size, &bytesSend, 0)) {
        ClearCommError(this->handler, &this->errors, &this->status);
        printf("ERROR: 0x%08x\n", this->errors);
        return false;
    }

    return true;
}

// Checking if serial port is connected
bool SerialPort::isConnected() {
    if(!ClearCommError(this->handler, &this->errors, &this->status)) {
        this->connected = false;
    }

    return this->connected;
}

void SerialPort::closeSerial() {
    CloseHandle(this->handler);
}
