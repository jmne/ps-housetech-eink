/**
    @filename   :   epd5in83b_V2-demo.ino
    @brief      :   5.83inch e-paper B V2 display demo
    @author     :   Yehui from Waveshare

    Copyright (C) Waveshare     July 4 2020

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documnetation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to  whom the Software is
   furished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include <SPI.h>

#include "epd5in83b_V2.h"

#include "WiFi.h"

#include <HTTPClient.h>

#include <ESP.h>

#include <Arduino.h>

#include "time.h"

// 194.400 is the amount of charracters in one full image displayed on this particular 5.83 inch Waveshare display (648x480).
// These characters contain the same recurring 5 characters, for e.g.: "0XFF,"
// Therefore, to create an array, that is readable by the Waveshare display, you have to extract those batches of 5 characters and transform them into a Hex-Array
// The Waveshare display can only receive a specific amount of bytes at a time (194.000/5 = 38.000 is too big); Therefore the whole picture has to be splitted into two
// 194.400 / 2 = 97.200
// 97.200 / 5 = 19.440 -> size of the resulting array of a half image
// Make a buffer, so that the ESP32 won't crash during the Hex-Array transformation; Use a high divident of 97.200
// 97.200 / 3.240 = 30 -> there need to be 30 for loops
// 3.240 / 5 = 648 -> each for loop needs to have 648 iterations
int chars = 97200;

int responseLength = 3240;

int forLoops = chars / responseLength;

int loopSize = responseLength / 5;

// Response length for the buffer; +1 because the buffer needs it that way
const int MAX_RESPONSE_LENGTH = responseLength + 1;

// WIFI and server configuration
const char* ssid = "sum";
const char* password = "PSK-22050-wl-xr";

const char* ntpServer = "pool.ntp.org";
// Timeszone
const long gmtOffset_sec = 2;
// Hour off because of energy saving reasons
const int daylightOffset_sec = 3600;

String room = "008";

String servername = "https://ps-housetech.uni-muenster.de:444/api/eink/" + room;

// Converion factor into minutes, ULL to force bigger values than 2147483647 in esp_sleep_enable_timer_wakeup()
#define uS_TO_S_FACTOR 60000000ULL
// Converion factor into seconds
//#define uS_TO_S_FACTOR 1000000ULL

int sleepTime;
int minutesOff;

RTC_DATA_ATTR int bootCount = 0;

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wake_up_source;

  wake_up_source = esp_sleep_get_wakeup_cause();

  switch (wake_up_source) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wake-up from external signal with RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wake-up from external signal with RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wake up caused by a timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wake up caused by a touchpad"); break;
    default: Serial.printf("Wake up not caused by Deep Sleep: %d\n", wake_up_source); break;
  }
}

void doRestart() {
  //Timer Configuration
  esp_sleep_enable_timer_wakeup(1000000);
  Serial.println("ESP32 wake-up in 1 second");

  Serial.println("WiFi Disconnected");
  Serial.println("Goes into Deep Sleep mode");
  Serial.println("----------------------");
  delay(100);
  esp_deep_sleep_start();
}

// Wieso das hier? Timer
void calcDifferenceTime() {
  Serial.println("--- calcDifferenceTime ---");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  // Calculate the sleepTime
  minutesOff = timeinfo.tm_min;
  // Its 20:00
  // between 18:00-00:00
  if (timeinfo.tm_hour >= 18) {
    sleepTime = 24 - timeinfo.tm_hour;
  }

  else if (timeinfo.tm_hour < 6) {
    sleepTime = 24 - timeinfo.tm_hour;
  } else {
    //Its for whatever reason after 06:00
    // calculate hours between time and 18:00
    sleepTime = 18 - timeinfo.tm_hour;
  }
  Serial.println("SleepTime, minutesOff");
  Serial.println(sleepTime);
  Serial.println(minutesOff);
}

void setup() {
  Serial.begin(115200);
  int wifiCount = 0;
  // Setup Wifi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    // Try to connect to the WiFi ten times, if it doesn't work, restart the ESP32
    if (wifiCount < 10) {
      delay(500);
      Serial.println("Connecting to WiFi..");
    } else {
      doRestart();
    }
    wifiCount++;
  }
  Serial.println("Connected to the WiFi network");

  // Timer  holt Date und Time vom Timeserver
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  calcDifferenceTime();
}

void loop() {
  ++bootCount;
  Serial.println("----------------------");
  Serial.println(String(bootCount) + "th Boot ");

  // Displays the reason for the wake up
  print_wakeup_reason();

  int adjustedSleepTime;

  adjustedSleepTime = (sleepTime * 60) - minutesOff;

  //Timer Configuration
  esp_sleep_enable_timer_wakeup(adjustedSleepTime * uS_TO_S_FACTOR);
  Serial.println("ESP32 wake-up in " + String(adjustedSleepTime) + " minutes");

  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    // Connect to the server
    http.begin(servername.c_str());

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);

      // Create a buffer to read the response in chunks
      char responseBuffer[MAX_RESPONSE_LENGTH];
      int bytesRead;

      // Get the HTTP response stream
      WiFiClient* stream = http.getStreamPtr();

      // Count for each for loop
      int count1 = 0;
      int count2 = 0;
      int count3 = 0;
      int count4 = 0;

      // Count inside each for loop
      int imageDataCount1 = 0;
      int imageDataCount2 = 0;
      int imageDataCount3 = 0;
      int imageDataCount4 = 0;

      // 4 uint8_t arrays for the images; One full image is divided into an upper and lower image; Each upper and lower image has a layer 1 and 2
      uint8_t* upperImage_Layer1 = new uint8_t[19446];
      uint8_t* lowerImage_Layer1 = new uint8_t[19446];
      uint8_t* upperImage_Layer2 = new uint8_t[19446];
      uint8_t* lowerImage_Layer2 = new uint8_t[19446];

      // Get the characters from the server and save them into the images and layers
      while (stream->available()) {
        bytesRead = stream->readBytes(responseBuffer, sizeof(responseBuffer) - 1);
        responseBuffer[bytesRead] = '\0';  // Null-terminate the buffer

        if (count3 >= forLoops) {
          for (size_t j = 0; j < loopSize; j++) {
            char hex[3];
            hex[0] = responseBuffer[j * 5 + 2];
            hex[1] = responseBuffer[j * 5 + 3];
            hex[2] = '\0';
            /*
            // Debug
            Serial.print(hex[0]);
            Serial.print(hex[1]);
            Serial.print(hex[2]);
            Serial.println(j);
            Serial.println(count4);
            */
            lowerImage_Layer2[imageDataCount4] = strtol(hex, nullptr, 16);
            imageDataCount4++;
          }
          count4++;
        }

        else if (count2 >= forLoops) {
          for (size_t j = 0; j < loopSize; j++) {
            char hex[3];
            hex[0] = responseBuffer[j * 5 + 2];
            hex[1] = responseBuffer[j * 5 + 3];
            hex[2] = '\0';
            /*
            // Debug
            Serial.print(hex[0]);
            Serial.print(hex[1]);
            Serial.print(hex[2]);
            Serial.println(j);
            Serial.println(count3);
            */
            upperImage_Layer2[imageDataCount3] = strtol(hex, nullptr, 16);
            imageDataCount3++;
          }
          count3++;
        }

        else if (count1 >= forLoops) {
          for (size_t j = 0; j < loopSize; j++) {
            char hex[3];
            hex[0] = responseBuffer[j * 5 + 2];
            hex[1] = responseBuffer[j * 5 + 3];
            hex[2] = '\0';
            /*
            // Debug
            Serial.print(hex[0]);
            Serial.print(hex[1]);
            Serial.print(hex[2]);
            Serial.println(j);
            Serial.println(count2);
            */
            lowerImage_Layer1[imageDataCount2] = strtol(hex, nullptr, 16);
            imageDataCount2++;
          }
          count2++;

        } else {
          for (size_t j = 0; j < loopSize; j++) {
            char hex[3];
            hex[0] = responseBuffer[j * 5 + 2];
            hex[1] = responseBuffer[j * 5 + 3];
            hex[2] = '\0';
            /*
            // Debug
            Serial.print(hex[0]);
            Serial.print(hex[1]);
            Serial.print(hex[2]);
            Serial.println(j);
            Serial.println(count1);
            */
            upperImage_Layer1[imageDataCount1] = strtol(hex, nullptr, 16);
            imageDataCount1++;
          }
          count1++;
        }
      }
      /*
      // Debug
      Serial.print("Amount of characters from server: ");
      Serial.println(count1 * loopSize * 5 + count2 * loopSize * 5 + count3 * loopSize * 5 + count4 * loopSize * 5);
      Serial.print("Memory After Hex: ");
      Serial.println(ESP.getFreeHeap());
      */
      // Initialize the e-Paper display
      Epd epd;

      if (epd.Init() != 0) {

        Serial.print("ERROR: e-Paper init failed");
        return;
      }

      // Refresh display
      epd.Clear();

      // Display the image
      epd.DisplayPicture(upperImage_Layer1, upperImage_Layer2, lowerImage_Layer1, lowerImage_Layer2);
      // Close the connection
      http.end();

      Serial.println("Goes into Deep Sleep mode");
      Serial.println("----------------------");
      delay(100);
      esp_deep_sleep_start();
    }
  } else {
    doRestart();
  }
  doRestart();
}
