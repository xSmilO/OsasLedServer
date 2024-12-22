#pragma once

#include <WinSock2.h>
// #include <windows.h>
#include <inttypes.h>
#include <math.h>
#include <chrono>
#include "SerialPort.hpp"

template<typename T>
bool find(T key, T* arr, const int& size) {
    for (int i = 0; i < size; i++) {
        if (key == arr[i])
            return true;
    }

    return false;
}

template<typename T>
void printArr(T* arr, const int& size) {
    for (int i = 0; i < size; i++) {
        std::cout << arr[i] << ",";
    }

    std::cout << "\n";
}