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

// 194.400 is the amount of charracters in one full image displayed on this particular 5.83 inch Waveshare display (648x480).
// These characters consist the same recurring 5 characters, for e.g.: "0XFF,"
// Therefore, to create an array, that is readable by the Waveshare display, you have to extract those batches of 5 characters and transform them into a Hex-Array
// The Waveshare display can only receive a specific amount of bytes at a time (194.000/5 = 38.000 is too big); Therefore the whole picture has to be splitted into two
// 194.400 / 2 = 97.200 -
// 97.200 / 5 = 19.440 -> size of the resulting array of a half image
// Make a buffer, so that the ESP32 won't crash during the Hex-Array transformation; Use a high divident of 97.200
// 97.200 / 3.240 = 30 -> there need to be 30 foor loops
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

String servername = "https://ps-housetech.uni-muenster.de/api/eink";

// the following variables are unsigned longs because the time, measured in milliseconds, will quickly become a bigger number than can be stored in an int.

unsigned long lastTime = 0;

// Set timer to 5 seconds (5000)

unsigned long timerDelay = 5000;



void setup() {

  Serial.begin(115200);

  // Setup Wifi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
}

void loop() {

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

      // count for each for loop
      int count1 = 0;
      int count2 = 0;
      int count3 = 0;
      int count4 = 0;

      // count inside each for loop
      int imageDataCount1 = 0;
      int imageDataCount2 = 0;
      int imageDataCount3 = 0;
      int imageDataCount4 = 0;

      // 4 uint8_t arrays for the images; One full image is divided into an upper and lower image; Each upper and lower image has a layer 1 and 2
      uint8_t* upperImage_Layer1 = new uint8_t[19446];
      uint8_t* lowerImage_Layer1 = new uint8_t[19446];
      uint8_t* upperImage_Layer2 = new uint8_t[19446];
      uint8_t* lowerImage_Layer2 = new uint8_t[19446];

      // Getting the characters from the server and saving them into the images and layers
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
            // for debugging
            Serial.print(hex[0]);
            Serial.print(hex[1]);
            Serial.print(hex[2]);
            Serial.println(j);
            Serial.println(counter);
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
            // for debugging
            Serial.print(hex[0]);
            Serial.print(hex[1]);
            Serial.print(hex[2]);
            Serial.println(j);
            Serial.println(counter);
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
            // for debugging
            Serial.print(hex[0]);
            Serial.print(hex[1]);
            Serial.print(hex[2]);
            Serial.println(j);
            Serial.println(counter);
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
            // for debugging
            Serial.print(hex[0]);
            Serial.print(hex[1]);
            Serial.print(hex[2]);
            Serial.println(j);
            Serial.println(counter);
            */
            upperImage_Layer1[imageDataCount1] = strtol(hex, nullptr, 16);
            imageDataCount1++;
          }
          count1++;
        }
      }

      Serial.print("Amount of characters from server: ");
      Serial.println(count1 * loopSize * 5 + count2 * loopSize * 5 + count3 * loopSize * 5 + count4 * loopSize * 5);
      Serial.print("Memory After Hex: ");
      Serial.println(ESP.getFreeHeap());


      // Initialize the e-Paper display
      Epd epd;

      if (epd.Init() != 0) {

        Serial.print("ERROR: e-Paper init failed");

        //delete[] imageData;  // Free allocated memory
        return;
      }

      epd.Clear();

      // Display the image
      epd.DisplayPicture(upperImage_Layer1, upperImage_Layer2, lowerImage_Layer1, lowerImage_Layer2);

      delay(25000);
      epd.Clear();

      // Put the display to sleep
      epd.Sleep();

    }

    else {

      Serial.print("Error code: ");

      Serial.println(httpResponseCode);
    }
    // Close the connection
    http.end();
  }

  else {

    Serial.println("WiFi Disconnected");
  }

  lastTime = millis();

  // Delay before the next iteration
  delay(timerDelay);
}
