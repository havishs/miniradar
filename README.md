# miniradar

Servo-swept ultrasonic scanner on an STM32 NUCLEO-L476RG. Bare-metal, register-level C firmware streams CRC-protected binary packets to a C++ receiver, which serves a live radar view in the browser.

<!-- add demo video / GIF link here -->
https://github.com/user-attachments/assets/25f5c115-588e-472d-9fa9-61c1d2f79e9c

## Hardware
- NUCLEO-L476RG (STM32L476RG, 80 MHz)
- SG90 servo (PWM, PA6)
- HC-SR04 ultrasonic sensor (TRIG on PB5, ECHO on PB6)
- HC-05 Bluetooth module (USART1, PA9/PA10; see status below)

## How it works
- **Servo:** TIM3 PWM at 50 Hz on PA6 with 1 µs ticks (80 MHz / 80). Pulse width 700-2300 µs sweeps about 180° in 100 µs steps.
- **Ranging:** TIM4 input capture on PB6 measures the echo pulse in hardware: rising edge on CC1, falling edge on CC2, pulse = CCR2 - CCR1 (16-bit wrap-safe). Distance = t x 343 m/s / 2. The 10 µs trigger pulse is timed from the same free-running timer.
- **Protocol:** 8-byte packets over USART2 (ST-LINK USB serial, 115200 baud):

  | byte | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
  |---|---|---|---|---|---|---|---|---|
  | field | 0xAA | 0x55 | seq | angle | dist_lo | dist_hi | CRC-8 | spare |

  Sync bytes find the start of a packet, `seq` detects dropped packets, and CRC-8 (poly 0x07, init 0, over bytes 2-5) detects corruption.
- **Receiver (C++):** POSIX termios serial input, a sync/body state machine, CRC and sequence checking, plus a small HTTP server with Server-Sent Events.
- **Radar view:** `radar.html` draws polar readings on a canvas with range rings, a sweep line, and fading dots.

## Results
- Reliability: 2,090-packet run over USB serial with 0 CRC errors and 0 dropped packets (sequence numbers continuous)
- Scan rate: about 8.5 packets/s (509 packets in 60 s), about 4 s per full back-and-forth sweep (17 positions each way)
- Repeatability: about ±5 mm at a fixed target (readings of 250-256 mm and 569-578 mm)
- Accuracy: hand-measured paper targets read 253 mm at a nominal 279 mm and 576 mm at a nominal 559 mm.

## Build and run
Firmware: open the project in STM32CubeIDE, build, and flash to the Nucleo.

Receiver:
```
cd receiver
clang++ -std=c++17 -O2 -pthread -o radar_rx radar_rx.cpp
./radar_rx /dev/cu.usbmodemXXXX
```
Then open http://localhost:8080 in a browser. Run it from the `receiver` folder so it can find `radar.html`. To read the Bluetooth port instead, pass the baud as a second argument: `./radar_rx /dev/cu.HC-05 9600`.

## Repo layout
```
miniradar/
  Core/, Drivers/, miniradar.ioc   firmware (STM32CubeIDE project)
  receiver/
    radar_rx.cpp                   serial receiver + HTTP/SSE server
    radar.html                     live radar page
```

## Status / known issues
- Firmware also transmits the same packets on USART1 for an HC-05 Bluetooth module. The module pairs and forwards data in AT-mode tests (9600 baud confirmed), but end-to-end reception on macOS is not yet verified.
- Single-ping readings show occasional outliers; a median filter is planned.
- Planned: TC74 temperature compensation of the speed of sound, interrupt/DMA transmit.
