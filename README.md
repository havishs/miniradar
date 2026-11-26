# miniradar

Servo-swept ultrasonic scanner on an STM32 NUCLEO-L476RG. Bare-metal, register-level C firmware streams CRC-protected binary packets over USB serial and Bluetooth to a C++ receiver, which serves a live radar view in the browser.

https://github.com/user-attachments/assets/25f5c115-588e-472d-9fa9-61c1d2f79e9c

## Hardware
- NUCLEO-L476RG (STM32L476RG, 80 MHz)
- SG90 servo (PWM, PA6)
- HC-SR04 ultrasonic sensor (TRIG on PB5, ECHO on PB6)
- HC-05 Bluetooth module (USART1, PA9 TX to module RXD, 9600 baud)

## How it works
- **Servo:** TIM3 PWM at 50 Hz on PA6 with 1 µs ticks (80 MHz / 80). Pulse width 700-2300 µs sweeps about 180° in 100 µs steps.
- **Ranging:** TIM4 input capture on PB6 measures the echo pulse in hardware: rising edge on CC1, falling edge on CC2, pulse = CCR2 - CCR1 (16-bit wrap-safe). Distance = t x 343 m/s / 2. The 10 µs trigger pulse is timed from the same free-running timer.
- **Filtering:** each angle takes 3 pings, 50 ms apart, and reports the median, which rejects a single bad echo. A missing echo reads as 0; if two of the three pings get no echo, the reading is reported as no echo.
- **Protocol:** 8-byte packets sent on both USART2 (ST-LINK USB serial, 115200 baud) and USART1 (HC-05 Bluetooth, 9600 baud):

  | byte | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
  |---|---|---|---|---|---|---|---|---|
  | field | 0xAA | 0x55 | seq | angle | dist_lo | dist_hi | CRC-8 | spare |

  Sync bytes find the start of a packet, `seq` detects dropped packets, and CRC-8 (poly 0x07, init 0, over bytes 2-5) detects corruption. Each packet is built once and the same bytes go to both links.
- **Receiver (C++):** POSIX termios serial input, a sync/body state machine, CRC and sequence checking, plus a small HTTP server with Server-Sent Events.
- **Radar view:** `radar.html` draws polar readings on a canvas with range rings, a sweep line, and fading dots.

## Results
- **USB serial:** 2,090-packet run with 0 CRC errors and 0 dropped packets.
- **Bluetooth (HC-05):** 1,159-packet run with 0 CRC errors and 0 dropped packets.
- **Scan rate (single-ping firmware):** about 8.5 packets/s (509 packets in 60 s), about 4 s per full back-and-forth sweep (17 positions each way). The median-of-3 firmware is slower and has not been re-measured.
- **Repeatability:** standard deviation of about 5-6.5 mm at a fixed flat target at 279 mm.
- **Accuracy:** a target at a nominal 279 mm read 250-253 mm across three runs on two days, a consistent offset of about -26 to -29 mm (likely where the sensor's distance reference sits, not yet calibrated). A target at a nominal 559 mm read 576 mm, but that setup was hand-placed with about ±20 mm uncertainty.
- **Median filter:** no measurable change on a clean flat target (0 outliers with and without it). Its benefit on cluttered scenes with spurious echoes has not been measured yet.

## Build and run
Firmware: open the project in STM32CubeIDE, build, and flash to the Nucleo.

Receiver:
```
cd receiver
clang++ -std=c++17 -O2 -pthread -o radar_rx radar_rx.cpp
./radar_rx /dev/cu.usbmodemXXXX
```
Then open http://localhost:8080 in a browser. Run it from the `receiver` folder so it can find `radar.html`.

Over Bluetooth (macOS), pair the HC-05 (PIN 1234), then:
```
sudo pkill bluetoothd; sleep 5; ./radar_rx /dev/cu.HC-05 9600
```

## Repo layout
```
miniradar/
  Core/, Drivers/, miniradar.ioc   firmware (STM32CubeIDE project)
  receiver/
    radar_rx.cpp                   serial receiver + HTTP/SSE server
    radar.html                     live radar page
```

## Status / known issues
- **macOS Bluetooth quirk:** the HC-05 serial port only delivers data on the first connection after the Bluetooth service restarts; later reconnects open the port but receive nothing. Restart `bluetoothd` (command above) and make `radar_rx` the first program to open the port.
- Some runs show a sequence gap right after the receiver starts; not yet root-caused.
- Planned: calibrate the range offset against a ruler, measure the median filter in clutter, TC74 temperature compensation of the speed of sound, interrupt/DMA transmit.
