/* THIS CODE IS STILL IN PROGRESS!!
 * See my GitHub for further update(maybe¯\_(ツ)_/¯):https://github.com/Neutrino331/ESP12-Smartwatch
*/

#include <Wire.h>
#include <SSD1306Wire.h>
#include <RtcDS3231.h> //https://github.com/Makuna/Rtc
#include <JC_Button.h> //https://github.com/JChristensen/JC_Button
#include <WifiLocation.h> //https://github.com/gmag11/WifiLocation
#include <WifiUDP.h>
#include <NTPClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <ArduinoJson.h>

// Define NTP properties
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "time.stdtime.gov.tw"

const char* googleApiKey = "Your-Google-API-Key"; //Enter your Google API key here
const char* ssid = "Your-Wifi-SSID"; //Enter Your SSID
const char* passwd = "Your-Wifi-Password"; //Enter Your Password
const String gday[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
float temperature;
const int Home_PIN = 0;
const int Up_PIN = 14;
const int Select_PIN = 12;
const int Down_PIN = 13;
const int LED_PIN = A0;
unsigned long standbyTimer;
boolean active = false;
byte activeTime = 15; //how many sec until entering standby
byte menuVal = 0;
byte currentPage = 0;

SSD1306Wire  display(0x3c, 4, 5);

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

RtcDS3231<TwoWire> Rtc(Wire);

RtcDateTime tm;
RtcTemperature temp;
location_t loc;


Button HomeBTN(Home_PIN, true, true, 20);
Button UpBTN(Up_PIN, true, true, 20);
Button SelectBTN(Select_PIN, true, true, 20);
Button DownBTN(Down_PIN, true, true, 20);


void display_initialize() {
  display.init();


  display.flipScreenVertically();
  display.clear();
  display.setFont(ArialMT_Plain_10);

}

void setup() {
  Wire.begin(4, 5);

  display_initialize();



  pinMode(Home_PIN, INPUT_PULLUP); // Home Button Pullup
  HomeBTN.begin();
  pinMode(Up_PIN, INPUT_PULLUP); // Up Button Pullup
  UpBTN.begin();
  pinMode(Select_PIN, INPUT_PULLUP); // Down Button Pullup
  SelectBTN.begin();
  pinMode(Down_PIN, INPUT_PULLUP);
  DownBTN.begin();
  pinMode(LED_PIN, OUTPUT); //
  analogWrite(LED_PIN, 0);

  Rtc.Begin();

  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  if (!Rtc.IsDateTimeValid()) {
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing
    // following line sets the RTC to the date & time this sketch was compiled
    // it will also reset the valid flag internally unless the Rtc device is
    // having an issue

    Rtc.SetDateTime(compiled);
  }
  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) //time outdated
  {
    Rtc.SetDateTime(compiled);
  }
  standbyTimer = millis() + activeTime * 1000;
  SelectBTN.read();
  UpBTN.read();
  DownBTN.read();
  HomeBTN.read();
}

void loop() {
  active = (millis() <= standbyTimer);
  SelectBTN.read();
  if (active && (UpBTN.read() || DownBTN.read())) standbyTimer = millis() + activeTime * 1000; //buttons reset Standby Timer
  if (currentPage == 0 && SelectBTN.wasPressed()) { //read Buttons
    currentPage = 9; //switch to menu
    menuVal = 0;
    SelectBTN.read(); //make sure wasPressed is not activated again
  }
  tm = Rtc.GetDateTime();
  temp = Rtc.GetTemperature();


  if (currentPage == 0 || !active) drawWatchface();
  else if (currentPage == 9) drawMenu();
  else if (currentPage == 1) drawWifiScan();
  else if (currentPage == 2) Geolocation();
  else if (currentPage == 3) NTPupdate();
  else if (currentPage == 4) Flashlight();
  else display.clear();


  display.display();

  if (!active) { //switch to watchface and sleep
    currentPage = 0;
    display.clear();

    ESP.deepSleep(5000, WAKE_RF_DISABLED); //ESP enter deepsleep mode,wake up after 5 Sec w/ Wi-Fi disabled
  }


}

void drawWatchface() {
  String t;
  if (tm.Hour() < 10) t += "0";
  t += String(tm.Hour());
  t += ":";
  t += String(tm.Minute());
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  display.drawString(64, 10, t);
  String date;
  date += String(tm.Year());
  date += ".";
  date += String(tm.Month());
  date += ".";
  date += String(tm.Day());
  date += " ";
  date += String(gday[tm.DayOfWeek()]);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 34, date);
  display.drawString(64, 44, String(temp.AsFloatDegC()) + "C ");
}

void drawMenu() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(15, 10, "Wifi Staion Scanner");
  display.drawString(15, 20, "Geolocation");
  display.drawString(15, 30, "NTP Update");
  display.drawString(15, 40 , "Flashlight");

  if (UpBTN.wasPressed()) menuVal++;
  if (DownBTN.wasPressed()) menuVal--;

  if (menuVal > 4 && menuVal < 99) menuVal = 1; //circle navigation
  else if (menuVal > 99) menuVal = 4;

  if (SelectBTN.wasPressed()) {
    currentPage = menuVal;
    SelectBTN.read(); //make sure wasPressed is not activated again
  }
  switch (menuVal) {
    case 1:
      display.drawString(7, 10, ">");
      display.display();
      break;
    case 2:
      display.drawString(7, 20, ">");
      display.display();
      break;
    case 3:
      display.drawString(7, 30, ">");
      display.display();
      break;
    case 4:
      display.drawString(7, 40, ">");
      display.display();
      break;
  }

  if (HomeBTN.wasPressed()) {
    currentPage = 0; //switch back to Watchface
    HomeBTN.read(); //make sure wasPressed is not activated again
  }
}

void Geolocation() {
  standbyTimer = millis() + activeTime * 1000;
  boolean tried = false;

  if (WiFi.status() != WL_CONNECTED) {
    display.clear();
    display.drawString(10, 10, "Connecting to Wifi...");
    display.display();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, passwd);
    display.drawString(10, 24, "Connected.");
    display.display();
  }

  if (!tried) {
    display.clear();
    WifiLocation location(googleApiKey);
    loc = location.getGeoFromWiFi();
    tried = true;
  }
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(10, 0, "Geoocation");
  display.setFont(ArialMT_Plain_10);
  display.drawString(10, 16, "Latitude: " + String(loc.lat, 7));
  display.drawString(10, 26, "Longitude: " + String(loc.lon, 7));
  display.drawString(10, 36, "Accuracy: " + String(loc.accuracy));

  if (HomeBTN.wasPressed()) {
    currentPage = 0; //switch back to Watchface
    HomeBTN.read(); //make sure wasPressed is not activated again
  }
}

void drawWifiScan() {
  standbyTimer = millis() + activeTime * 1000;
  boolean tried = false;
  display.clear();
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
  }
  if (!tried) {
    display.drawString(10, 0, "scan start");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    display.drawString(10, 10, "scan done");
    if (n == 0) {
      display.drawString(10, 20, "no networks found");
    }
    else {
      display.drawString(10, 20, String(n) + " networks found");
    }

    tried = true;
  }

  display.clear();
  for (int i = 0; i < 6; ++i) {
    // Print SSID and RSSI for each network found
    display.drawString(10, 10 * i, String(i + 1) + ": " + (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*" + String(WiFi.SSID(i)) + " " + String(WiFi.RSSI(i)));
  }

  if (HomeBTN.wasPressed()) {
    currentPage = 0; //switch back to Watchface
    HomeBTN.read(); //make sure wasPressed is not activated again
  }

}

void NTPupdate() {
  standbyTimer = millis() + activeTime * 1000;
  boolean tried = false;
  display.clear();
  if (WiFi.status() != WL_CONNECTED) {
    display.drawString(0, 10, "Connecting to Wifi...");
    display.display();
    WiFi.begin(ssid, passwd);
    display.drawString(0, 24, "Connected.");
    display.display();
  }

  if (!tried) {
    timeClient.update();
    unsigned long epochTime =  timeClient.getEpochTime();
    display.clear();
    display.drawString(0, 0, "NTP Update");
    display.drawString(0, 10, "Get Internet Time From:");
    display.drawString(0, 20, "time.stdtime.gov.tw");
    display.display();
    // convert received time stamp to time_t object
    time_t local, utc;
    utc = epochTime;

    // Then convert the UTC UNIX timestamp to local time

    TimeChangeRule twNST = {"NST", First, Sun, Jan, 2, 420};   
    Timezone tw(twNST, twNST);
    local = tw.toLocal(utc);
    Rtc.SetDateTime(local);
    display.drawString(0, 30, "New Time Set");
    tried = true;
  }

  if (HomeBTN.wasPressed()) {
    currentPage = 0; //switch back to menu
    HomeBTN.read(); //make sure wasPressed is not activated again
  }
}

void Flashlight() {
  standbyTimer = millis() + activeTime * 1000;
  boolean state = false;
  display.clear();
  int light = 0;
  if (SelectBTN.wasPressed()) state = !state;
  if (state) {
    while (light > 0 && light < 1024) {
      if (UpBTN.wasPressed()) menuVal++;
      if (DownBTN.wasPressed()) menuVal--;
      analogWrite(LED_PIN, light);
    }
    display.drawProgressBar(0, 32, 120, 10, light / 10.24);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, "lightness" + String(light / 10.24) + "%");
  }
  else {
    analogWrite(LED_PIN, 0);
  }
  if (HomeBTN.wasPressed()) {
    analogWrite(LED_PIN, 0);
    currentPage = 0; //switch back to menu
    HomeBTN.read(); //make sure wasPressed is not activated again
  }
}
