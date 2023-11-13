#include <M5Core2.h>
#include <WiFi.h>
#include <FastLED.h>

const char* ssid = "";
const char* password = "";

struct tm timeinfo;
const char* ntpServer = "ntp.nict.jp";
const long  gmtOffset_sec = 3600 * 9;
const int   daylightOffset_sec = 0;
RTC_TimeTypeDef TimeStruct;
RTC_DateTypeDef DateStruct;
const char* weekday[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
const char* month[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

bool pomodoro = false;
bool shortBreak = false;
bool longBreak = false;
int timerCount = 0;
int pomodoroCount = 0;
int second = 0;

#define NUM_LEDS 10
#define DATA_PIN 25
CRGB leds[NUM_LEDS];

void connectWiFi() {
  WiFi.begin(ssid, password);
  int wifi_cnt = 20;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (--wifi_cnt == 0) {
      disconnectWiFi();
      return;
    }
  }
  setRtc();
  disconnectWiFi();
}

void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void setRtc() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime(&timeinfo);

  TimeStruct.Hours   = timeinfo.tm_hour;
  TimeStruct.Minutes = timeinfo.tm_min;
  TimeStruct.Seconds = timeinfo.tm_sec;
  M5.Rtc.SetTime(&TimeStruct);

  DateStruct.WeekDay = timeinfo.tm_wday;
  DateStruct.Month = timeinfo.tm_mon + 1;
  DateStruct.Date = timeinfo.tm_mday;
  DateStruct.Year = timeinfo.tm_year + 1900;
  M5.Rtc.SetDate(&DateStruct);
}

void printLocalTime() {
  M5.Rtc.GetDate(&DateStruct);
  M5.Rtc.GetTime(&TimeStruct);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setCursor((M5.Lcd.width() - M5.Lcd.textWidth("Sun, Jan 01")) / 2, 0);
  M5.Lcd.printf("%s, %s %02d\n", weekday[DateStruct.WeekDay - 1], month[DateStruct.Month - 1], DateStruct.Date);
  M5.Lcd.setTextSize(6);
  M5.Lcd.setCursor((M5.Lcd.width() - M5.Lcd.textWidth("00:00:00")) / 2, 50);
  M5.Lcd.printf("%02d:%02d:%02d\n", TimeStruct.Hours, TimeStruct.Minutes, TimeStruct.Seconds);
}

void printTimer() {
  M5.Lcd.setTextSize(4);
  M5.Lcd.setCursor((M5.Lcd.width() - M5.Lcd.textWidth("           ")) / 2, 120);
  if (pomodoro == true) {
    M5.Lcd.printf("Pomodoro #%d\n", pomodoroCount);
  } else if (shortBreak == true) {
    M5.Lcd.printf(" Break #%d  \n", pomodoroCount);
  } else if (longBreak == true) {
    M5.Lcd.println("Long Break ");
  } else {
    M5.Lcd.println("           ");
  }
  M5.Lcd.setTextSize(6);
  M5.Lcd.setCursor((M5.Lcd.width() - M5.Lcd.textWidth("00:00")) / 2, 170);
  M5.Lcd.printf("%2d:%02d", timerCount / 60, timerCount % 60);
}

void startPomodoro() {
  shortBreak = false;
  longBreak = false;
  pomodoro = true;
  pomodoroCount++;
  turnOnRedLed();
  delay(1000);
  turnOffLed();
  timerCount = 25 * 60;
  M5.Rtc.GetTime(&TimeStruct);
  second = TimeStruct.Seconds;
}

void startShortBreak() {
  pomodoro = false;
  shortBreak = true;
  turnOnGreenLed();
  timerCount = 5 * 60;
  M5.Rtc.GetTime(&TimeStruct);
  second = TimeStruct.Seconds;
}

void startLongBreak() {
  pomodoro = false;
  longBreak = true;
  pomodoroCount = 0;
  turnOnBlueLed();
  timerCount = 20 * 60;
  M5.Rtc.GetTime(&TimeStruct);
  second = TimeStruct.Seconds;
}

void turnOnRedLed() {
  fill_solid(leds, 10, CRGB::Red);
  FastLED.show();
}

void turnOnGreenLed() {
  fill_solid(leds, 10, CRGB::Green);
  FastLED.show();
}

void turnOnBlueLed() {
  fill_solid(leds, 10, CRGB::Blue);
  FastLED.show();
}

void turnOffLed() {
  fill_solid(leds, 10, CRGB::Black);
  FastLED.show();
}

void setup() {
  // put your setup code here, to run once:
  M5.Rtc.begin();
  M5.begin();
  connectWiFi();
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 225);
  M5.Lcd.println("(Re)start           Stop");
}

void loop() {
  // put your main code here, to run repeatedly:
  M5.update();
  printLocalTime();
  M5.Rtc.GetTime(&TimeStruct);
  if (M5.BtnA.wasPressed()) {
    if (pomodoro == true || shortBreak == false && longBreak == false) {
      startPomodoro();
    }
  }
  if (timerCount > 0 && second != TimeStruct.Seconds) {
    timerCount--;
    second = TimeStruct.Seconds;
  }
  printTimer();

  if (pomodoro == true && timerCount == 0) {
    if (pomodoroCount >= 4) {
      startLongBreak();
    } else {
      startShortBreak();
    }
  } else if (shortBreak == true && timerCount == 0) {
    startPomodoro();
  } else if (longBreak == true && timerCount == 0) {
    startPomodoro();
  }

  if (M5.BtnC.wasPressed()) {
    turnOffLed();
    pomodoro = false;
    shortBreak = false;
    longBreak = false;
    timerCount = 0;
    pomodoroCount = 0;
    return;
  }
}
