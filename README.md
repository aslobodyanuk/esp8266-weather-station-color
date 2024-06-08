# ThingPulse ESP8266 Weather Station Color

# There should be settings.h file and it should look something like this

```c++
#ifndef SETTINGS_H
#define SETTINGS_H
#include <Arduino.h>
#include "TZinfo.h"

// Setup
String WIFI_SSID = "yourssid";
String WIFI_PASS = "yourpassw0rd";
// Limited to 31 chars
#define WIFI_HOSTNAME "ThingPulse-weatherstation-color"

const int UPDATE_INTERVAL_SECS = 10 * 60; // Update every 10 minutes
const int SLEEP_INTERVAL_SECS = 0;        // Going to sleep after idle times, set 0 for insomnia
const boolean HARD_SLEEP = false;         // true go into deepSleep false = turn Back light off

// OpenWeatherMap Settings
// Sign up here to get an API key: https://docs.thingpulse.com/how-tos/openweathermap-key/
String OPEN_WEATHER_MAP_API_KEY = "";

/*
Go to https://openweathermap.org/find?q= and search for a location. Go through the
result set and select the entry closest to the actual location you want to display 
data for. It'll be a URL like https://openweathermap.org/city/2657896. The number
at the end is what you assign to the constant below.
 */
String OPEN_WEATHER_MAP_LOCATION_ID = "2657896";
String DISPLAYED_LOCATION_NAME = "Zurich";
//String OPEN_WEATHER_MAP_LOCATION_ID = "3833367";
//String DISPLAYED_LOCATION_NAME = "Ushuaia";
//String OPEN_WEATHER_MAP_LOCATION_ID = "2147714";
//String DISPLAYED_LOCATION_NAME = "Sydney";
//String OPEN_WEATHER_MAP_LOCATION_ID = "5879400";
//String DISPLAYED_LOCATION_NAME = "Anchorage";

/*
Arabic -> ar, Bulgarian -> bg, Catalan -> ca, Czech -> cz, German -> de, Greek -> el,
English -> en, Persian (Farsi) -> fa, Finnish -> fi, French -> fr, Galician -> gl,
Croatian -> hr, Hungarian -> hu, Italian -> it, Japanese -> ja, Korean -> kr,
Latvian -> la, Lithuanian -> lt, Macedonian -> mk, Dutch -> nl, Polish -> pl,
Portuguese -> pt, Romanian -> ro, Russian -> ru, Swedish -> se, Slovak -> sk,
Slovenian -> sl, Spanish -> es, Turkish -> tr, Ukrainian -> ua, Vietnamese -> vi,
Chinese Simplified -> zh_cn, Chinese Traditional -> zh_tw.
*/
const String OPEN_WEATHER_MAP_LANGUAGE = "en";

// Adjust according to your language
const String WDAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const String MONTH_NAMES[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
const String SUN_MOON_TEXT[] = {"Sun", "Rise", "Set", "Moon", "Age", "Illum"};
const String MOON_PHASES[] = {"New Moon", "Waxing Crescent", "First Quarter", "Waxing Gibbous",
                              "Full Moon", "Waning Gibbous", "Third quarter", "Waning Crescent"};

// pick one from TZinfo.h
String TIMEZONE = getTzInfo("Europe/Zurich");

// values in metric or imperial system?
bool IS_METRIC = true;

// Change for 12 Hour/ 24 hour style clock
bool IS_STYLE_12HR = false;

// Change for HH:MM/ HH:MM:SS format clock
bool IS_STYLE_HHMM = false; // true => HH:MM

// change for different NTP (time servers)
#define NTP_SERVERS "pool.ntp.org"
// #define NTP_SERVERS "us.pool.ntp.org", "time.nist.gov", "pool.ntp.org"

// Locations on the northern hemisphere (latitude > 0) and those on the southern hemisphere need 
// an inverted set of moon phase icons/characters.
// fully illuminated -> full moon -> char 48
// zero illumination -> new moon -> char 64
const char MOON_ICONS_NORTH_WANING[] = {64, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 48};
const char MOON_ICONS_NORTH_WAXING[] = {64, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 48};
const char MOON_ICONS_SOUTH_WANING[] = {64, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 48};
const char MOON_ICONS_SOUTH_WAXING[] = {64, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 48};

// August 1st, 2018
#define NTP_MIN_VALID_EPOCH 1533081600

// Pins for the ILI9341
#define TFT_DC D2
#define TFT_CS D1
#define TFT_LED D8

#define HAVE_TOUCHPAD
#define TOUCH_CS D3
#define TOUCH_IRQ  D4


/***************************
 * End Settings
 **************************/
#endif
```

[![ThingPulse logo](https://thingpulse.com/assets/ThingPulse-w300.svg)](https://thingpulse.com)

ESP8266 Weather Station Color using ILI9341 240x320 TFT display

## Hardware Requirements

The code in this project supports an ILI9341 240x320 TFT display with code running on an ESP8266. To get you up and running in no time we created a kit which contains all the necessary parts including a custom PCB to connect display and WiFi chip:
[https://thingpulse.com/product/esp8266-wifi-color-display-kit-2-4/](https://thingpulse.com/product/esp8266-wifi-color-display-kit-2-4/)

Buy the kit from us to support future development of this application. Thank you!

[![ThingPulse ESP8266 Color Display Kit](resources/ESP8266ColorDisplayKit.jpg)](https://thingpulse.com/product/esp8266-wifi-color-display-kit-2-4/)

## Service level promise

<table><tr><td><img src="https://thingpulse.com/assets/ThingPulse-open-source-prime.png" width="150">
</td><td>This is a ThingPulse <em>prime</em> project. See our <a href="https://thingpulse.com/about/open-source-commitment/">open-source commitment declaration</a> for what this means.</td></tr></table>

## Step-by-step tutorial

A complete step-by-step tutorial/guide is available at [https://docs.thingpulse.com/guides/wifi-color-display-kit/](https://docs.thingpulse.com/guides/wifi-color-display-kit/).

## Licensing, contributions and maintenance

The code in this repository is licensed under [MIT](https://en.wikipedia.org/wiki/MIT_License), a short and simple permissive license with conditions only requiring preservation of copyright and license notices. Thus, you're free to fork the project and use the code for your own projects as long as you keep the copyright notices in place.

ThingPulse is committed to open-source development and will continue to maintain this code. We welcome contributions from the community given they are roughly in line with our [guidelines](CONTRIBUTING.md). However, please understand that we primarily developed this application to be run on our own hardware kit mentioned above. It's the only platform we regularly test the code against. You are of course free to run the code on any hardware you think is compatible but you have to rely on community support should you run into problems. 

ThingPulse runs a support forum for its customers that is better suited to answering user questions than the issues list here.

## Wiring

The [wiring diagram](https://docs.thingpulse.com/specs/wifi-color-display-kit/#wiring) is only needed when you do _not_ buy the self-contained kit from ThingPulse but rather assemble the components yourself. The kit provides a custom PCB that solidly connects microcontroller and display.

## Operation

See ["Operation" in the official documentation](https://docs.thingpulse.com/guides/wifi-color-display-kit/#operation).
