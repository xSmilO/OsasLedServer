#pragma once

#include <inttypes.h>
#include <math.h>
#include <iostream>

#define TOP_LEDS 34
#define LEFT_LEDS 14
#define RIGHT_LEDS 14

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
