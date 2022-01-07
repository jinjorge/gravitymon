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
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <WiFiManager.h>
#include <incbin.h>

#include <calc.hpp>
#include <config.hpp>
#include <gyro.hpp>
#include <helper.hpp>
#include <tempsensor.hpp>
#include <wifi.hpp>

Wifi myWifi;

const char *userSSID = USER_SSID;
const char *userPWD = USER_SSID_PWD;

//
// Connect to last known access point or create one if connection is not
// working.
//
bool Wifi::connect(bool showPortal) {
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA);

  if (!strlen(myConfig.getWifiSSID())) {
    Log.info(
        F("WIFI: No SSID seams to be stored, forcing portal to start." CR));
    showPortal = true;
  } else {
    // Log.info(F("WIFI: Using SSID=%s and %s." CR), myConfig.getWifiSSID(),
    // myConfig.getWifiPass()); Log.info(F("WIFI: Using SSID=%s and %s." CR),
    // myConfig.getWifiSSID(), "*****");
  }

  if (strlen(userSSID) == 0 && showPortal) {
#if LOG_LEVEL == 6
    Log.verbose(
        F("WIFI: Connecting to WIFI via connection manager (portal=%s)." CR),
        showPortal ? "true" : "false");
#endif
    WiFiManager myWifiManager;
    Log.notice(F("WIFI: Starting wifi portal." CR));
    myWifiManager.setDebugOutput(true);
    myWifiManager.setClass("invert");
    myWifiManager.setConfigPortalTimeout(120);  // Keep it open for 120 seconds
    bool f =
        myWifiManager.startConfigPortal(WIFI_DEFAULT_SSID, WIFI_DEFAULT_PWD);
    if (f) {
      // Log.notice(F("WIFI: Success got values from WIFI portal=%s,%s." CR),
      // myWifiManager.getWiFiSSID(), myWifiManager.getWiFiPass() );
      Log.notice(F("WIFI: Success got values from WIFI portal=%s,%s." CR),
                 myWifiManager.getWiFiSSID(), "*****");
      myConfig.setWifiSSID(myWifiManager.getWiFiSSID());
      myConfig.setWifiPass(myWifiManager.getWiFiPass());
      myConfig.saveFile();
    } else {
      Log.notice(F("WIFI: Failure from WIFI portal, rebooting." CR));
      delay(200);
      ESP.reset();
    }
  }

  // Connect to wifi
  int i = 0;

  // Log.notice(F("WIFI: Connecting to WIFI, mode=%d,persistent=%d,fhy=%d ."
  // CR), WiFi.getMode(), WiFi.getPersistent(), WiFi.getPhyMode() );
  WiFi.mode(WIFI_STA);
  if (strlen(userSSID)) {
    Log.notice(F("WIFI: Connecting to wifi using hardcoded settings %s." CR),
               userSSID);
    WiFi.begin(userSSID, userPWD);
  } else {
    Log.notice(F("WIFI: Connecting to wifi using stored settings %s." CR),
               myConfig.getWifiSSID());
    WiFi.begin(myConfig.getWifiSSID(), myConfig.getWifiPass());
  }

  // WiFi.printDiag(Serial);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");

    if (i++ > 100) {  // Try for 20 seconds.
      Log.error(F("WIFI: Failed to connect to wifi %d, aborting %s." CR),
                WiFi.status(), getIPAddress().c_str());
      WiFi.disconnect();
      return connectedFlag;  // Return to main that we have failed to connect.
    }
  }
  Serial.print(CR);
  connectedFlag = true;
  Log.notice(F("WIFI: Connected to wifi ip=%s." CR), getIPAddress().c_str());
  Log.notice(F("WIFI: Using mDNS name %s." CR), myConfig.getMDNS());
  return connectedFlag;
}

//
// This will erase the stored credentials and forcing the WIFI manager to AP
// mode.
//
bool Wifi::disconnect() {
  Log.notice(F("WIFI: Erasing stored WIFI credentials." CR));
  // Erase WIFI credentials
  return WiFi.disconnect(true);
}

#if defined(ACTIVATE_OTA)

//
//
//
bool Wifi::updateFirmware() {
  if (!newFirmware) {
    Log.notice(F("WIFI: No newer version exist, skipping update." CR));
    return false;
  }
#if LOG_LEVEL == 6
  Log.verbose(F("WIFI: Updating firmware." CR));
#endif

  WiFiClient client;
  String serverPath = myConfig.getOtaURL();
  serverPath += "firmware.bin";

  HTTPUpdateResult ret = ESPhttpUpdate.update(client, serverPath);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Log.error(F("WIFI: OTA update failed %d, %s." CR),
                ESPhttpUpdate.getLastError(),
                ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      break;
    case HTTP_UPDATE_OK:
      Log.notice("WIFI: OTA Update sucesfull, rebooting.");
      delay(100);
      ESP.reset();
      break;
  }
  return false;
}

//
// Download and save file
//
void Wifi::downloadFile(const char *fname) {
#if LOG_LEVEL == 6
  Log.verbose(F("WIFI: Download file %s." CR), fname);
#endif
  WiFiClient client;
  HTTPClient http;
  String serverPath = myConfig.getOtaURL();
  serverPath += fname;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverPath);
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    File f = LittleFS.open(fname, "w");
    http.writeToStream(&f);
    f.close();
    Log.notice(F("WIFI: Downloaded file %s." CR), fname);
  } else {
    Log.error(F("WIFI: Failed to download file, respone=%d" CR),
              httpResponseCode);
  }
  http.end();
}

//
// Check what firmware version is available over OTA
//
bool Wifi::checkFirmwareVersion() {
#if LOG_LEVEL == 6
  Log.verbose(F("WIFI: Checking if new version exist." CR));
#endif
  WiFiClient client;
  HTTPClient http;
  String serverPath = myConfig.getOtaURL();
  serverPath += "version.json";

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverPath);

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    Log.notice(F("WIFI: Found version.json, response=%d" CR), httpResponseCode);

    String payload = http.getString();
#if LOG_LEVEL == 6
    Log.verbose(F("WIFI: Payload %s." CR), payload.c_str());
#endif
    DynamicJsonDocument ver(300);
    DeserializationError err = deserializeJson(ver, payload);
    if (err) {
      Log.error(F("WIFI: Failed to parse version.json, %s" CR), err);
    } else {
#if LOG_LEVEL == 6
      Log.verbose(F("WIFI: Project %s version %s." CR),
                  (const char *)ver["project"], (const char *)ver["version"]);
#endif
      int newVer[3];
      int curVer[3];

      if (parseFirmwareVersionString(newVer, (const char *)ver["version"])) {
        if (parseFirmwareVersionString(curVer, CFG_APPVER)) {
#if LOG_LEVEL == 6
          Log.verbose(F("WIFI: OTA checking new=%d.%d.%d cur=%d.%d.%d" CR),
                      newVer[0], newVer[1], newVer[2], curVer[0], curVer[1],
                      curVer[2]);
#endif
          // Compare major version
          if (newVer[0] > curVer[0]) newFirmware = true;
          // Compare minor version
          if (newVer[0] == curVer[0] && newVer[1] > curVer[1])
            newFirmware = true;
          // Compare patch version
          if (newVer[0] == curVer[0] && newVer[1] == curVer[1] &&
              newVer[2] > curVer[2])
            newFirmware = true;
        }
      }

      // Download new html files to filesystem if they are present.
      if (!ver["html"].isNull() && newFirmware) {
        Log.notice(F("WIFI: OTA downloading new html files." CR));
        JsonArray htmlFiles = ver["html"].as<JsonArray>();
        for (JsonVariant v : htmlFiles) {
          String s = v;
#if LOG_LEVEL == 6
          Log.verbose(F("WIFI: OTA listed html file %s" CR), s.c_str());
#endif
          downloadFile(s.c_str());
        }
      }
    }
  } else {
    Log.error(F("WIFI: OTA error checking version.json, response=%d" CR),
              httpResponseCode);
  }
  http.end();
#if LOG_LEVEL == 6
  Log.verbose(F("WIFI: OTA found new version %s." CR),
              newFirmware ? "true" : "false");
#endif
  return newFirmware;
}

//
// Parse a version string in the format M.m.p (eg. 1.2.10)
//
bool Wifi::parseFirmwareVersionString(int (&num)[3], const char *version) {
#if LOG_LEVEL == 6
  Log.verbose(F("WIFI: Parsing version number string %s." CR), version);
#endif
  char temp[80];
  char *s;
  char *p = &temp[0];
  int i = 0;

  // strcpy(&temp[0], version);
  snprintf(&temp[0], sizeof(temp), "%s", version);

  while ((s = strtok_r(p, ".", &p)) != NULL) {
    num[i++] = atoi(s);
  }

  return true;
}

#endif  // ACTIVATE_OTA

// EOF
