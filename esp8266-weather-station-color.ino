/**The MIT License (MIT)
 
 Copyright (c) 2018 by ThingPulse Ltd., https://thingpulse.com
 
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

/*****************************
 * Important: see settings.h to configure your settings!!!
 * ***************************/
#include "settings.h"

#include <Arduino.h>
#include "time.h"
#include <SPI.h>
#include <ESP8266WiFi.h>

#include <XPT2046_Touchscreen.h>
#include "TouchControllerWS.h"
#include "SunMoonCalc.h"


/***
   Install the following libraries through Arduino Library Manager
   - Mini Grafx by Daniel Eichhorn
   - ESP8266 WeatherStation by Daniel Eichhorn
   - Json Streaming Parser by Daniel Eichhorn
 ***/

#include <JsonListener.h>
#include <OpenWeatherMapCurrent.h>
#include <OpenWeatherMapForecast.h>
#include <MiniGrafx.h>
#include <Carousel.h>
#include <ILI9341_SPI.h>

#include "ArialRounded.h"
#include "moonphases.h"
#include "weathericons.h"

#define DISPLAY_DEBUG 0
#define DISABLE_WIFI_DEBUG 0

#define DISPLAY_WIDTH 239
#define DISPLAY_HEIGHT 319
#define DISPLAY_SPACER_MARGIN 15

#define MINI_BLACK 0
#define MINI_WHITE 1
#define MINI_YELLOW 2
#define MINI_BLUE 3

#define MAX_FORECASTS 12

#define NORMAL_TEXT_MARGIN 4
#define LONG_TEXT_MARGIN NORMAL_TEXT_MARGIN * 3

#define MAX_WEATHER_DESCRIPTION 18

// Generated with Analogous tool from: https://www.canva.com/colors/color-wheel/
// Converted to hex colors with: http://www.rinkydinkelectronics.com/calc_rgb565.php

// defines the colors usable in the paletted 16 color frame buffer
uint16_t palette[] = {
  ILI9341_BLACK, // 0 - Black
  0xFFBE, // 1 - White
  0xFEE8, // 2 - Yellow
  0xFC08  //3 - Blue
};

// Limited to 4 colors due to memory constraints
int BITS_PER_PIXEL = 2; // 2^2 =  4 colors


ADC_MODE(ADC_VCC);


ILI9341_SPI tft = ILI9341_SPI(TFT_CS, TFT_DC);
MiniGrafx gfx = MiniGrafx(&tft, BITS_PER_PIXEL, palette);

XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
TouchControllerWS touchController(&ts);

void calibrationCallback(int16_t x, int16_t y);
CalibrationCallback calibration = &calibrationCallback;

OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
OpenWeatherMapForecastData updated_forecasts[MAX_FORECASTS];

SunMoonCalc::Moon moonData;

void updateData();
void drawProgress(uint8_t percentage, String text);
void drawTime();
void drawWifiQuality();
void drawCurrentWeather();
void drawForecast();
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex);
void drawAstronomy();
void drawForecastTable(uint8_t start);
void drawSeparator(uint16_t y);
String getTime(time_t *timestamp);
const char* getMeteoconIconFromProgmem(String iconText);
const char* getMiniMeteoconIconFromProgmem(String iconText);
void loadPropertiesFromSpiffs();

// FrameCallback frames[] = { drawForecast1, drawForecast2 };
// int frameCount = 2;

// how many different screens do we have?
int screenCount = 5;
long lastDownloadUpdate = millis();
long lastScreenUpdate = millis();

uint8_t screen = 0;
// divide screen into 4 quadrants "< top", "> bottom", " < middle "," > middle "
uint16_t dividerTop, dividerBottom, dividerMiddle;
uint8_t changeScreen(TS_Point p, uint8_t screen);

long timerPress;
bool canBtnPress;
char* make12_24(int hour);

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  //Manual Wifi
  Serial.printf("Connecting to WiFi %s/%s", WIFI_SSID.c_str(), WIFI_PASS.c_str());
  // WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (i > 80) i = 0;
    drawProgress(i, "Connecting to WiFi '" + String(WIFI_SSID.c_str()) + "'");
    i += 10;
    Serial.print(".");
  }
  drawProgress(100, "Connected to WiFi '" + String(WIFI_SSID.c_str()) + "'");
  Serial.println("connected.");
  Serial.printf("Connected, IP address: %s/%s\n", WiFi.localIP().toString().c_str(), WiFi.subnetMask().toString().c_str()); //Get ip and subnet mask
  Serial.printf("Connected, MAC address: %s\n", WiFi.macAddress().c_str());  //Get the local mac address
}

void initTime() {
  Serial.println("Initializing time...");
  time_t now;

  gfx.fillBuffer(MINI_BLACK);
  gfx.setFont(ArialRoundedMTBold_14);

  Serial.printf("Configuring time for timezone %s\n", TIMEZONE.c_str());
  configTime(TIMEZONE.c_str(), NTP_SERVERS);
  int i = 1;
  while ((now = time(nullptr)) < NTP_MIN_VALID_EPOCH) {
    if (i > 80) i = 0;
    drawProgress(i, "Updating time...");
    Serial.println("initTime: " + String(now));
    delay(300);
    yield();
    i += 10;
  }
  drawProgress(100, "Time synchronized");
  Serial.println();

  printf("Local time: %s", asctime(localtime(&now))); // print formated local time, same as ctime(&now)
  printf("UTC time:   %s", asctime(gmtime(&now)));    // print formated GMT/UTC time
}

void setup() {
  Serial.begin(115200);

  loadPropertiesFromSpiffs();

  // The LED pin needs to set HIGH
  // Use this pin to save energy
  // Turn on the background LED
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);    // HIGH to Turn on;

  gfx.init();
  gfx.fillBuffer(MINI_BLACK);
  gfx.commit();

  // Serial.println("Initializing touch screen...");
  // ts.begin();

  Serial.println("Mounting file system...");
  bool isFSMounted = SPIFFS.begin();
  if (!isFSMounted) {
    Serial.println("Formatting file system...");
    drawProgress(50, "Formatting file system");
    SPIFFS.format();
  }
  drawProgress(100, "Formatting done");

  /* Allow user to force a screen re-calibration  */
  gfx.fillBuffer(MINI_BLACK);
  gfx.commit();

//   gfx.drawString(120, 160, F("Press and hold\nto initiate touch screen\ncalibration"));
//   gfx.commit();
//   delay(3000);
//   yield();
//   boolean isCalibrationAvailable = touchController.loadCalibration();
//   if(ts.touched()) {
//     Serial.println("first");
//     isCalibrationAvailable = false;
//     gfx.fillBuffer(MINI_YELLOW);
//     gfx.drawString(120, 160, F("Calibration initiated\nnow release screen"));
//     gfx.commit();

//     // Wait for release otherwise touch becomes first calibration point
//     while(ts.touched()) { 
//       Serial.println("loop");
//       delay(10);
//       yield();
//     }

//     Serial.println("second");

//     delay(100); // debounce
//     touchController.getPoint(); // throw away last point
//   }

// Serial.println("third");

//   if (!isCalibrationAvailable) {
//     Serial.println("Calibration data not available or force calibration initiated");
//     touchController.startCalibration(&calibration);
//     while (!touchController.isCalibrationFinished()) {
//       gfx.fillBuffer(0);
//       gfx.setColor(MINI_YELLOW);
//       gfx.setTextAlignment(TEXT_ALIGN_CENTER);
//       gfx.drawString(120, 160, "Please calibrate\ntouch screen by\ntouch point");
//       touchController.continueCalibration();
//       gfx.commit();
//       yield();
//     }
//     touchController.saveCalibration();
//   }

  dividerTop = 64;
  dividerBottom = gfx.getHeight() - dividerTop;
  dividerMiddle = gfx.getWidth() / 2;
  
  if (DISABLE_WIFI_DEBUG != 1) {
    connectWifi();
    // update the weather information
    updateData();
  }

  timerPress = millis();
  canBtnPress = true;
}

long lastDrew = 0;
bool btnClick;
uint8_t MAX_TOUCHPOINTS = 10;
TS_Point points[10];
uint8_t currentTouchPoint = 0;

void loop() {
  static bool asleep = false;	//  asleep used to stop screen change after touch for wake-up
  
  /* Break up the screen into 4 sections a touch in section:
   * - Top changes the time format
   * - Left back one page
   * - Right forward one page
   * - Bottom jump to page 0
   */
  // if (touchController.isTouched(500)) {
  //   TS_Point p = touchController.getPoint();
  //   timerPress = millis();
  //   if (!asleep) { 				// no need to update or change screens;
  //   	screen = changeScreen(p, screen);
  //   }
  // } // isTouched()

  if (millis() - lastScreenUpdate > 1000 * SCREEN_UPDATE_INTERVAL_SECS) {
    updateScreen();
    lastScreenUpdate = millis();
  }

  // Check if we should update weather information
  if (millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS) {
    updateData();
    lastDownloadUpdate = millis();
  }

  delay(1000);
  yield();
}

void updateScreen() {
  Serial.println("Updating screen.");
  if (screen == 0) {
    gfx.fillBuffer(MINI_BLACK);

    drawDisplayDebug();
    drawCurrentDate();
    // drawTime();
    // drawWifiQuality();
    // int remainingTimeBudget = carousel.update();
    // if (remainingTimeBudget > 0) {
    //   // You can do some work here
    //   // Don't do stuff if you are below your
    //   // time budget.
    //   delay(remainingTimeBudget);
    // }
    drawCurrentWeather();
    drawForecast();
    // drawAstronomy();

  }
  gfx.commit();
}

void printWiFiStatus() {
  Serial.print("WiFi Status: [" + String(WiFi.status()) + "]");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" connected ");
  } else {
    Serial.print(" non connected status ");
  }

  Serial.println();
}

// Update the internet based information and update screen
void updateData() {
  Serial.println("Updating data...");
  printWiFiStatus();

  initTime();
  time_t now = time(nullptr);

  gfx.fillBuffer(MINI_BLACK);
  gfx.setFont(ArialRoundedMTBold_14);

  drawProgress(50, "Updating conditions...");
  OpenWeatherMapCurrent *currentWeatherClient = new OpenWeatherMapCurrent();
  currentWeatherClient->setMetric(IS_METRIC);
  currentWeatherClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  currentWeatherClient->updateCurrentById(&currentWeather, OPEN_WEATHER_MAP_API_KEY, OPEN_WEATHER_MAP_LOCATION_ID);
  delete currentWeatherClient;
  currentWeatherClient = nullptr;

  trimWeatherDescription();

  drawProgress(70, "Updating forecasts...");
  OpenWeatherMapForecast *forecastClient = new OpenWeatherMapForecast();
  forecastClient->setMetric(IS_METRIC);
  forecastClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {12, 0};
  forecastClient->setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient->updateForecastsById(updated_forecasts, OPEN_WEATHER_MAP_API_KEY, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);
  delete forecastClient;
  forecastClient = nullptr;

  drawProgress(80, F("Filtering forecasts..."));
  Serial.println(F("Filtering forecasts..."));
  filterForecasts();

  drawProgress(90, "Updating astronomy...");
  // 'now' has to be epoch instant, lat/lng in degrees not radians
  SunMoonCalc *smCalc = new SunMoonCalc(now, currentWeather.lat, currentWeather.lon);
  moonData = smCalc->calculateSunAndMoonData().moon;
  delete smCalc;
  smCalc = nullptr;
  Serial.printf("Free mem: %d\n",  ESP.getFreeHeap());

  delay(1000);
}

void filterForecasts() {
  OpenWeatherMapForecastData localForecasts[MAX_FORECASTS];
  uint8_t localCounter = 0;
  int selectedFilterHour = -1;

  time_t time1 = time(nullptr);
  int currentDate = getDate(&time1);

  for (uint8_t counter = 0;counter < MAX_FORECASTS;counter++) {

    time_t time = updated_forecasts[counter].observationTime;
    struct tm * timeinfo = localtime(&time);

    // Serial.print("Filtering forecast for index: ");
    // Serial.println(counter);
    // Serial.println(timeinfo->tm_hour);
    // Serial.println(String(timeinfo->tm_hour));
    
    // Filter out current date
    if (timeinfo->tm_mday == currentDate) {
      continue;
    }

    if (selectedFilterHour == -1 && timeinfo->tm_hour >= 13 && timeinfo->tm_hour <= 15) {
      selectedFilterHour = timeinfo->tm_hour;
      Serial.print("Selected filter hour: " + selectedFilterHour);
    }

    if (timeinfo->tm_hour == selectedFilterHour) {

      Serial.print("Adding forecast day: ");  
      Serial.print(WDAY_NAMES[timeinfo->tm_wday]);
      Serial.print(" ");
      Serial.println(timeinfo->tm_hour);

      localForecasts[localCounter] = updated_forecasts[counter];
      localCounter++;
    }
  }

  memcpy(forecasts, localForecasts, sizeof(forecasts));
}

void trimWeatherDescription() {
  if (currentWeather.description.length() <= MAX_WEATHER_DESCRIPTION) {
    return;
  }

  byte descriptionLength = MAX_WEATHER_DESCRIPTION - 3;
  String newDescription = currentWeather.description.substring(0, descriptionLength) + "...";
  currentWeather.description = newDescription;
}

// Progress bar helper
void drawProgress(uint8_t percentage, String text) {
  gfx.fillBuffer(MINI_BLACK);
  // gfx.drawPalettedBitmapFromPgm(20, 5, ThingPulseLogo);
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  // gfx.setColor(MINI_WHITE);
  // gfx.drawString(120, 90, "https://thingpulse.com");
  gfx.setColor(MINI_YELLOW);

  gfx.drawString(120, 146, text);
  gfx.setColor(MINI_WHITE);
  gfx.drawRect(10, 168, 240 - 20, 15);
  gfx.setColor(MINI_BLUE);
  gfx.fillRect(12, 170, 216 * percentage / 100, 11);

  gfx.commit();
}

// draws the clock
void drawTime() {
  char time_str[11];
  time_t now = time(nullptr);
  struct tm * timeinfo = localtime(&now);
  uint16_t previousTextPixelsLength = 0;

  gfx.setFont(ArialRoundedMTBold_14);

  if (IS_STYLE_12HR) {                                                              //12:00
    int hour = (timeinfo->tm_hour + 11) % 12 + 1; // take care of noon and midnight
    if (IS_STYLE_HHMM) {
      sprintf(time_str, "%2d:%02d\n", hour, timeinfo->tm_min);                //hh:mm
    } else {
      sprintf(time_str, "%2d:%02d:%02d\n", hour, timeinfo->tm_min, timeinfo->tm_sec); //hh:mm:ss
    }
  } else {                                                                            //24:00
    if (IS_STYLE_HHMM) {
        sprintf(time_str, "%02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min); //hh:mm
    } else {
        sprintf(time_str, "%02d:%02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec); //hh:mm:ss
    }
  }

  uint16_t xcoord = 5;

  gfx.setColor(MINI_WHITE);
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  previousTextPixelsLength = gfx.drawString(xcoord, 0, time_str);
  
  xcoord += previousTextPixelsLength + LONG_TEXT_MARGIN;

  gfx.setColor(MINI_BLUE);
  previousTextPixelsLength = gfx.drawString(xcoord, 0, WDAY_NAMES[timeinfo->tm_wday]);

  xcoord += previousTextPixelsLength + NORMAL_TEXT_MARGIN;
  gfx.setColor(MINI_WHITE);
  previousTextPixelsLength = gfx.drawString(xcoord, 0, String(timeinfo->tm_mday));

  xcoord += previousTextPixelsLength + LONG_TEXT_MARGIN;
  gfx.setColor(MINI_YELLOW);
  previousTextPixelsLength = gfx.drawString(xcoord, 0, MONTH_NAMES[timeinfo->tm_mon]);

  xcoord += previousTextPixelsLength + NORMAL_TEXT_MARGIN;
  gfx.setColor(MINI_WHITE);
  previousTextPixelsLength = gfx.drawString(xcoord, 0, String(1900 + timeinfo->tm_year));
}

// draws current date information
void drawCurrentDate() {
  
  time_t now = time(nullptr);
  struct tm * timeinfo = localtime(&now);

  uint8_t ycoord = 6;
  uint8_t xcoord = DISPLAY_WIDTH - DISPLAY_SPACER_MARGIN + 1;

  gfx.setFont(ArialRoundedMTBold_36);
  gfx.setColor(MINI_WHITE);
  gfx.setTextAlignment(TEXT_ALIGN_RIGHT);

  uint16_t previousTextPixelsLength = gfx.drawString(xcoord, ycoord, MONTH_NAMES[timeinfo->tm_mon]);

  xcoord -= previousTextPixelsLength;
  gfx.setColor(MINI_BLUE);
  previousTextPixelsLength = gfx.drawString(xcoord, ycoord, String(timeinfo->tm_mday) + " ");

  xcoord -= previousTextPixelsLength;
  gfx.setColor(MINI_WHITE);
  previousTextPixelsLength = gfx.drawString(xcoord, ycoord, WDAY_NAMES[timeinfo->tm_wday] + ", ");
}

// draws current weather information
void drawCurrentWeather() {
  uint8_t ycoord = 48;
  uint8_t xcoord = 225;

  gfx.setTransparentColor(MINI_BLACK);
  gfx.drawPalettedBitmapFromPgm(10, ycoord, getMeteoconIconFromProgmem(currentWeather.icon));
  // Weather Text

  ycoord += 7;
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setColor(MINI_YELLOW);
  gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
  gfx.drawString(xcoord, ycoord, DISPLAYED_LOCATION_NAME);

  gfx.setFont(ArialRoundedMTBold_36);
  gfx.setColor(MINI_WHITE);
  gfx.setTextAlignment(TEXT_ALIGN_RIGHT);

  ycoord += 16;
  gfx.drawString(xcoord, ycoord, String(currentWeather.temp, 0) + (IS_METRIC ? "°C" : "°F"));

  ycoord += 42;
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setColor(MINI_BLUE);
  gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
  gfx.drawString(xcoord, ycoord, currentWeather.description);
}

void drawForecast() {
  uint8_t xcoord = 13;
  uint8_t ycoord = 140;
  uint8_t distance = 57;

  drawForecastDetail(xcoord, ycoord, 0);
  
  ycoord += distance;
  drawForecastDetail(xcoord, ycoord, 1);

  ycoord += distance;
  drawForecastDetail(xcoord, ycoord, 2);
}

// helper for the forecast columns
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex) {
  gfx.setColor(MINI_YELLOW);
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);

  time_t time = forecasts[dayIndex].observationTime;
  struct tm * timeinfo = localtime(&time);
  uint8_t xcoord_distance = 54;

  uint8_t xcoord_date_distance = xcoord_distance;
  uint16_t previousTextPixelsLength2 = previousTextPixelsLength2 = gfx.drawString(x + xcoord_date_distance, y + 5, WDAY_FULL_NAMES[timeinfo->tm_wday] + ", ");

  xcoord_date_distance += previousTextPixelsLength2;
  gfx.setColor(MINI_WHITE);
  previousTextPixelsLength2 = gfx.drawString(x + xcoord_date_distance, y + 5, String(timeinfo->tm_mday) + " ");

  xcoord_date_distance += previousTextPixelsLength2;
  gfx.setColor(MINI_YELLOW);
  previousTextPixelsLength2 = gfx.drawString(x + xcoord_date_distance, y + 5, MONTH_FULL_NAMES[timeinfo->tm_mon]);

  // xcoord_distance -= 4;

  gfx.setColor(MINI_WHITE);
  uint16_t previousTextPixelsLength = gfx.drawString(x + xcoord_distance, y + 25, String(forecasts[dayIndex].temp, 0) + (IS_METRIC ? "°C" : "°F") + "  /  ");

  gfx.drawPalettedBitmapFromPgm(x, y, getMiniMeteoconIconFromProgmem(forecasts[dayIndex].icon));
  gfx.setColor(MINI_BLUE);
  gfx.drawString(x + xcoord_distance + previousTextPixelsLength, y + 25, String(forecasts[dayIndex].rain, 1) + (IS_METRIC ? " mm" : " in"));
}

// draw moonphase, sunrise/set and moonrise/set
void drawAstronomy() {

  // gfx.setFont(MoonPhases_Regular_36);
  // gfx.setColor(MINI_WHITE);
  // gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  // // gfx.drawString(120, 275, String((char) (97 + (moonData.illumination * 26))));
  // gfx.drawString(120, 275, String(determineMoonIcon()));

  // gfx.setColor(MINI_WHITE);
  // gfx.setFont(ArialRoundedMTBold_14);
  // gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  // gfx.setColor(MINI_YELLOW);
  // gfx.drawString(120, 250, MOON_PHASES[moonData.phase.index]);

  uint16_t xcoord = 5;
  uint16_t previousTextPixelsLength = 0;

  gfx.setTextAlignment(TEXT_ALIGN_LEFT);

  gfx.setColor(MINI_YELLOW);
  time_t time = currentWeather.sunrise;
  previousTextPixelsLength = gfx.drawString(xcoord, 300, "R:");

  xcoord = xcoord + previousTextPixelsLength + NORMAL_TEXT_MARGIN;
  gfx.setColor(MINI_WHITE);
  gfx.drawString(xcoord, 300, getTime(&time));

  float windSpeed = currentWeather.windSpeed;
  float windSpeedKmPerHour = windSpeed * 3.6; // Convert to km per hour
  gfx.setColor(MINI_YELLOW);
  previousTextPixelsLength = gfx.drawString(85, 300, "W:");

  xcoord = 85 + previousTextPixelsLength + NORMAL_TEXT_MARGIN;
  gfx.setColor(MINI_WHITE);
  gfx.drawString(xcoord, 300, String(windSpeedKmPerHour, 0) + " km/h");

  time = currentWeather.sunset;
  gfx.setColor(MINI_YELLOW);
  previousTextPixelsLength = gfx.drawString(183, 300, "S:");

  xcoord = 183 + previousTextPixelsLength + NORMAL_TEXT_MARGIN;
  gfx.setColor(MINI_WHITE);
  gfx.drawString(xcoord, 300, getTime(&time));

  // gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
  // gfx.setColor(MINI_YELLOW);
  // gfx.drawString(235, 250, SUN_MOON_TEXT[3]);
  // gfx.setColor(MINI_WHITE);

  // float lunarMonth = 29.53;
  // // approximate moon age
  // gfx.drawString(190, 276, SUN_MOON_TEXT[4] + ":");
  // gfx.drawString(235, 276, String(moonData.age, 1) + "d");
  // gfx.drawString(190, 291, SUN_MOON_TEXT[5] + ":");
  // gfx.drawString(235, 291, String(moonData.illumination * 100, 0) + "%");
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100) {
    return 0;
  } else if (dbm >= -50) {
    return 100;
  } else {
    return 2 * (dbm + 100);
  }
}

void drawWifiQuality() {
  int8_t quality = getWifiQuality();
  gfx.setColor(MINI_WHITE);
  gfx.setFont(ArialMT_Plain_10);
  gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
  gfx.drawString(228, 3, String(quality) + "%");
  gfx.setColor(MINI_YELLOW);
  for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 25 || j == 0) {
        gfx.setPixel(230 + 2 * i, 12 - j);
      }
    }
  }
}

void calibrationCallback(int16_t x, int16_t y) {
  gfx.setColor(1);
  gfx.fillCircle(x, y, 10);
}

String getTime(time_t *timestamp) {
  struct tm *timeInfo = localtime(timestamp);

  char buf[9];  // "12:34 pm\0"
  char ampm[3];
  ampm[0]='\0'; //Ready for 24hr clock
  uint8_t hour = timeInfo->tm_hour;

  if (IS_STYLE_12HR) {
    if (hour > 12) {
      hour = hour - 12;
      sprintf(ampm, "pm");
    } else {
      sprintf(ampm, "am");
    }
    sprintf(buf, "%2d:%02d %s", hour, timeInfo->tm_min, ampm);
  } else {
    sprintf(buf, "%02d:%02d %s", hour, timeInfo->tm_min, ampm);
  }
  return String(buf);
}

int getDate(time_t *timestamp) {
  struct tm *timeInfo = localtime(timestamp);
  return timeInfo->tm_mday;
}

/*
 *  Convert hour from 24 hr time to 12 hr time.
 *
 *  @return cString with 2 digit hour + am or pm 
 */
char* make12_24(int hour){
  static char hr[6];
  if(hour > 12){
    sprintf(hr, "%2d pm", (hour -12) );
    //sprintf(buf, "%02d:%02d %s", hour, timeInfo->tm_min, ampm);
  } else {
    sprintf(hr, "%2d am", hour);
  }
  return hr;
}

void loadPropertiesFromSpiffs() {
  if (SPIFFS.begin()) {
    const char *msg = "Using '%s' from SPIFFS\n";
    Serial.println(F("Attempting to read application.properties file from SPIFFS."));
    File f = SPIFFS.open("/application.properties", "r");
    if (f) {
      Serial.println(F("File exists. Reading and assigning properties."));
      while (f.available()) {
        String key = f.readStringUntil('=');
        String value = f.readStringUntil('\n');
        if (key == "ssid") {
          WIFI_SSID = value.c_str();
          Serial.printf(msg, "ssid");
        } else if (key == "password") {
          WIFI_PASS = value.c_str();
          Serial.printf(msg, "password");
        } else if (key == "timezone") {
          TIMEZONE = getTzInfo(value.c_str());
          Serial.printf(msg, "timezone");
        } else if (key == "owmApiKey") {
          OPEN_WEATHER_MAP_API_KEY = value.c_str();
          Serial.printf(msg, "owmApiKey");
        } else if (key == "owmLocationId") {
          OPEN_WEATHER_MAP_LOCATION_ID = value.c_str();
          Serial.printf(msg, "owmLocationId");
        } else if (key == "locationName") {
          DISPLAYED_LOCATION_NAME = value.c_str();
          Serial.printf(msg, "locationName");
        } else if (key == "isMetric") {
          IS_METRIC = value == "true" ? true : false;
          Serial.printf(msg, "isMetric");  
        } else if (key == "is12hStyle") {
          IS_STYLE_12HR = value == "true" ? true : false;
          Serial.printf(msg, "is12hStyle");
        }
      }
    } else {
      Serial.println(F("Does not exist."));
    }
    f.close();
    Serial.println(F("Effective properties now as follows:"));
    Serial.println(F("\tssid: ") + WIFI_SSID);
    Serial.println(F("\tpassword: ") + WIFI_PASS);
    Serial.println(F("\timezone: ") + TIMEZONE);
    Serial.println(F("\tOWM API key: ") + OPEN_WEATHER_MAP_API_KEY);
    Serial.println(F("\tOWM location id: ") + OPEN_WEATHER_MAP_LOCATION_ID);
    Serial.println(F("\tlocation name: ") + DISPLAYED_LOCATION_NAME);
    Serial.println(F("\tmetric: ") + String(IS_METRIC ? "true" : "false"));
    Serial.println(F("\t12h style: ") + String(IS_STYLE_12HR ? "true" : "false"));
  } else {
    Serial.println(F("SPIFFS mount failed."));
  }
}

/*
 * Change screen based on touchpoint location.
 */
uint8_t changeScreen(TS_Point p, uint8_t screen) {
  uint8_t page = screen;

  // Serial.printf("Touch point detected at %d/%d.\n", p.x, p.y);
  // From the screen's point of view commented values for the 240 X 320 touch screen
  // if (p.y < dividerTop)      Serial.print(" top ");    // < 80
  // if (p.y > dividerBottom)   Serial.print(" bottom "); // > 240
  // if (p.x > dividerMiddle)   Serial.print(" left ");   // > 120
  // if (p.x <= dividerMiddle)  Serial.print(" right ");  // <= 120
  // Serial.println();

  if (p.y < dividerTop) {            // top -> change 12/24h style
    IS_STYLE_12HR = !IS_STYLE_12HR;
  } else if (p.y > dividerBottom) {  // bottom -> go to screen 0
    page = 0;
  } else if (p.x > dividerMiddle) {  // left -> previous page
    if (page == 0) {            // Note type is unsigned
      page = screenCount;       // Last screen is max -1
    }
    page--;
  } else {                      // right -> next screen
    page = (page + 1) % screenCount;
  }
  return page;
}

void drawDisplayDebug() {
  if (DISPLAY_DEBUG != 1) {
    return;
  }

  gfx.setColor(MINI_YELLOW);

  for (int counter = 0; counter < DISPLAY_WIDTH; counter++) {
    if (counter % 2 == 0) {
      gfx.setPixel(counter, DISPLAY_SPACER_MARGIN);
      gfx.setPixel(counter, DISPLAY_HEIGHT - DISPLAY_SPACER_MARGIN);
    }
  }

  for (int counter = 0; counter < DISPLAY_HEIGHT; counter++) {
    if (counter % 2 == 0) {
      gfx.setPixel(DISPLAY_SPACER_MARGIN, counter);
      gfx.setPixel(DISPLAY_WIDTH - DISPLAY_SPACER_MARGIN, counter);
    }
  }

  gfx.setColor(MINI_WHITE);
}

