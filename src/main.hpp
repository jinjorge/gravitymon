/*
MIT License

Copyright (c) 2021-22 Magnus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
#ifndef SRC_MAIN_HPP_
#define SRC_MAIN_HPP_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <stdlib.h>

enum RunMode {
  gravityMode = 0,
  configurationMode = 1,
  wifiSetupMode = 2,
  storageMode = 3
};
extern RunMode runMode;

#if defined(ESP8266)
#include <LittleFS.h>
#define ESP_RESET ESP.reset
#define PIN_SDA D3
#define PIN_SCL D4
#define PIN_DS D6
#define PIN_LED 2
// #define PIN_A0 A0
#elif  defined(ESP32C3)
#include <FS.h>
#include <LittleFS.h>
#define ESPhttpUpdate httpUpdate
#define ESP_RESET ESP.restart
#define ESP8266WebServer WebServer
#define PIN_SDA SDA
#define PIN_SCL SCL
#define PIN_DS A3
#define PIN_A0 A0
#define PIN_LED LED_BUILTIN
/*
#elif  defined(ESP32S2)
#include <FS.h>
#include <LittleFS.h>
#define ESPhttpUpdate httpUpdate
#define ESP_RESET ESP.restart
#define ESP8266WebServer WebServer
#define PIN_SDA SDA
#define PIN_SCL SCL
#define PIN_DS A8
#define PIN_A0 A2
#define PIN_LED LED_BUILTIN
*/
#else  // defined (ESP32)
#include <FS.h>
#include <LittleFS.h>
#define ESPhttpUpdate httpUpdate
#define ESP_RESET ESP.restart
#define ESP8266WebServer WebServer
#define PIN_SDA TX
#define PIN_SCL RX
#define PIN_DS MISO
#define PIN_A0 A4
#define PIN_LED LED_BUILTIN
#endif

#define PIN_LED 2

#endif  // SRC_MAIN_HPP_
