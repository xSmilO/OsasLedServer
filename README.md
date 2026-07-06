# 💡 Serial LED Streamer & Controller

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![Arduino](https://img.shields.io/badge/Platform-Arduino%20%2F%20ESP-00979D.svg)
![FastLED](https://img.shields.io/badge/Library-FastLED-orange.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

> A high-performance, multithreaded LED controller system for streaming dynamic lighting effects from a PC to a microcontroller (ESP/Arduino) at a smooth 60 FPS.

[//]: # (Dodaj tutaj GIF lub zdjęcie działających LEDów!)
![Demo](link_to_your_gif_or_image_here)

## 🚀 Overview

This project consists of two parts: a **C++ Server/PC Application** that generates mathematical lighting effects and an **Embedded Microcontroller Firmware** that drives WS2812B LED strips. 

Instead of generating basic effects on the limited microcontroller, the PC handles all the heavy lifting and streams the pixel data via a custom, high-speed serial protocol (500,000 baud).

### ✨ Key Features
* **60 FPS Real-Time Streaming:** Smooth, lag-free data transmission from PC to hardware.
* **Custom Serial Protocol:** A lightweight, packet-based protocol with a non-blocking State Machine.
* **Hardware Interpolation (LERP):** The microcontroller smoothly interpolates between frames, ensuring buttery-smooth transitions even if a packet is dropped.
* **Multithreaded PC Backend:** Dedicated worker threads handle the effect queue and strictly paced frame dispatching (`std::chrono`) to prevent serial buffer overflow.
* **Bidirectional Communication:** Supports `SET` (changing brightness, animation delays) and `GET` (fetching current hardware configurations) commands.

---

## 🌐 3-Tier Architecture & Network Bridge

To make the system truly platform-agnostic, the C++ application operates as a **Bridge Server** rather than a standalone UI. This decouples the hardware interface from the user interface, allowing any client device on the local network to control the hardware seamlessly.

### 🏗️ System Flow

1. **Client Layer (Any Platform):** A Mobile App, Web Dashboard, or Desktop Client sends lightweight commands (e.g., JSON payloads) over the network (TCP/WebSockets).
2. **Server Layer (C++ Bridge):** The multithreaded C++ server listens for network commands, processes the mathematical lighting effects, and strictly paces the frame rate.
3. **Hardware Layer (ESP/Arduino):** Receives the high-speed binary stream via USB/Serial and updates the WS2812B LEDs.

    [ Mobile App / Web UI ] 
              │ 
              │ (Network: JSON over TCP/WebSockets)
              ▼
    [ C++ Bridge Server ] ──► Worker Thread 1 (Network Listener)
              │           ──► Worker Thread 2 (60 FPS Serial Pacing)
              │
              │ (Serial: Custom Binary Protocol @ 500k baud)
              ▼
    [ ESP / Arduino ] ──────► LERP Interpolation & WS2812B Output

### 🔒 Concurrency & Thread Safety
Because the server listens to network requests asynchronously while streaming data to the microcontroller at a fixed 60 FPS, strict thread safety is enforced. 

Incoming network commands (like adding a new effect to the queue or changing brightness) are secured using `std::mutex` and `std::lock_guard`. This guarantees that the network thread can inject new data into the `effectsQueue` without crashing the high-speed Serial Worker thread.

### 📱 Client Agnostic
Because the server exposes a standard network interface, client applications can be built in **any language or framework**:
* **Mobile:** React Native, Flutter, Swift, or Kotlin.
* **Web:** React, Vue, or vanilla JavaScript dashboards.
* **Desktop:** C#, Python, or Electron.
## 🛠️ Tech Stack & Hardware

**Software:**
* **C++ (PC Backend):** Multithreading (`std::thread`), precise timing (`std::chrono`), OOP (Effect Queue system).
* **C++ / Arduino (Embedded):** Non-blocking loops, Memory management (struct padding), FastLED library.

**Hardware:**
* Microcontroller: ESP8266 / ESP32 / Arduino (tested on [Insert Your Board])
* LED Strip: WS2812B (Currently configured for 62 LEDs on Pin 4)
* Power Supply: 5V (Adequate for max LED current)

---

## 🧠 Architecture & Challenges Solved

### 1. The Bottleneck: Serial Communication
Sending 62 LEDs × 3 bytes per frame at 60 FPS requires robust handling. If the PC sends data too fast, the OS buffer overflows, causing massive lag. 
**Solution:** Implemented a frame-pacing algorithm using `std::chrono` on the PC side to cap transmission precisely at the hardware's refresh rate (16.66ms per frame).

### 2. Embedded State Machine
Using standard `Serial.readBytes()` blocks the microcontroller, causing LEDs to freeze. 
**Solution:** Designed a custom, non-blocking State Machine. The ESP reads bytes one by one on every loop cycle. It parses headers (`0xAA`), identifies command types (`FRAME_DATA`, `SET_CONFIG`, `GET_CONFIG`), and processes variable-length payloads without halting the LED refresh logic.

### 3. Little Endian & Struct Serialization
Sending multi-byte variables (like `uint16_t` for animation delays or `float` for LERP speed) across serial requires careful bit-shifting. The project implements precise byte reconstruction and struct memory mapping to pass configuration data back and forth seamlessly.

---

## 🔌 The Custom Protocol

The communication is built on a custom packet structure. All packets start with a `0xAA` header.

| Command Type | Byte | Payload Format | Description |
| :--- | :---: | :--- | :--- |
| **FRAME DATA** | `0x10` | `[ID] [R] [G] [B]` ... | Streams raw color data for the LEDs. |
| **SET CONFIG** | `0x20` | `[Sub-ID] [Value...]` | Sets parameters. Variable length (1-2 bytes). |
| **GET CONFIG** | `0x30` | *None* | Requests the hardware to send back its state. |

**Example (Setting Animation Delay to 500ms):**
`0xAA` (Header) -> `0x20` (SET) -> `0x03` (ID_DELAY) -> `0xF4` (Low Byte) -> `0x01` (High Byte).

---

## 🚀 Getting Started

### Embedded Setup
1. Open the `.ino` file in the Arduino IDE or PlatformIO.
2. Install the **FastLED** library.
3. Configure `#define NUM_LEDS` and `#define DATA_PIN` to match your hardware.
4. Flash the code to your microcontroller.

### PC Server Setup
1. Clone the repository.
2. [Add instructions here on how to build the C++ project - e.g., CMake, Visual Studio, or Make]
3. Run the executable, ensuring the correct COM port is targeted.

---

## 📂 Code Structure Highlights

* `LedController.cpp` *(PC)*: Manages the effect queue and the multithreaded serial dispatch loop.
* `Effect.h` *(PC)*: Base class for creating custom math-based lighting effects.
* `Firmware.ino` *(Embedded)*: The hardware State Machine, LERP calculations, and FastLED implementation.

---

*This is still in development and the versions may vary*
