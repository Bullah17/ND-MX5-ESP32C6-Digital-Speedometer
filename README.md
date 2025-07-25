# ND-MX5-ESP32C6-Digital-Speedometer (WIP)
This is an ESP-32 based digital speedometer for the ND MX5. Designed to be inserted directly in-between the top of the gauge cluster binnacle and the tachnometer. I made it to be semi-unobtrusive, while also remaining in your peripheral view when driving. 

## Features
Uses ESP-NOW to wirelessly obtain the speed and rpm values from another esp-32 based CAN bus reader (you'll need to source that yourself).
* Powered either by USB or through the headerpins behind (for a cleaner setup)
* Toggle on/off shift lights
* Adjustable display brightness (can also switch between dim and bright depending on headlights)

## Parts
You can use the step files to 3D print some of the parts (with exception of the assembly)
* Filament: Use **TPU** 95A, I do **not** reccommend anything other than TPU as its supposed to 'squish' a little into your gauge cluster
  - Only the sheath **with USB** requires supports
* An ESP-32 based CAN bus reader. This solution requires ESP-NOW to communicate the speed and rpm values from the CAN bus to the speed display. You can DIY your own thing using any ESP-32 Dev board, CAN breakout module, and a OBD2 Male connector. Otherwise you can checkout integrated solutions like the [Mr DIY ESP32 CAN Bus Shield](https://store.mrdiy.ca/p/esp32-can-bus-shield/ "Mr DIY ESP32 CAN Bus Shield") or [RejsaCAN-ESP32](https://github.com/MagnusThome/RejsaCAN-ESP32 "RejsaCAN-ESP32")
* Off-the-shelf Parts:
  - [Waveshare ESP32-C6 with 1.47 Touch LCD](https://www.waveshare.com/wiki/ESP32-C6-Touch-LCD-1.47#Dimensions "Waveshare ESP32-C6 with 1.47 Touch LCD")
  - 4 x M2x8mm Screws for fastening the display to the 3d printed parts


