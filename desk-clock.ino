#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include <Adafruit_GFX.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#include <time.h>

// Timer
#include <passive_timer.h>

// Debouncer
#include <EasyButton.h>

// Font
#include "Org_01.h"

// Pin I2C
#define SDA_PIN 9
#define SCL_PIN 10

// Pin interrupt menu selection
#define MENU_BUTTON_PIN 0
EasyButton menuButton(MENU_BUTTON_PIN, 40, true);
int menuSelection;

// Pin interrupt display on/off
#define DISPLAY_BUTTON_PIN 1
EasyButton displayButton(DISPLAY_BUTTON_PIN, 40, true);
bool enableDisplay;

// WIFI Credentials
const char* ssid = "";
const char* password = "";

// Setup display
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire, -1);
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Clock offset
int GMTOffset = 3600;
int daylightOffset = 3600;

// Offset Temperature
int offsetTemp = 0;
// Offset Humidity
int offsetHum = 0;

// Timer
#define UPDATE_INTERVAL 2000
PassiveTimer clock_timer;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Setup Menu Button
  menuButton.begin();
  menuButton.onPressed(handleButtonMenuPress);
  menuSelection = 0;

  // Setup Display Button
  displayButton.begin();
  displayButton.onPressed(handleButtonDisplayPress);
  enableDisplay = true;

  setup_display();
  setup_wifi();

  setup_termometer();

  // Connecting to NTP server
  configTime(GMTOffset, daylightOffset, "it.pool.ntp.org", "pool.ntp.org", "time.nist.gov");

  delay(1000);
}
void loop() {
  menuButton.read();
  displayButton.read();

  if (enableDisplay) {
    display.ssd1306_command(SSD1306_DISPLAYON);

    if (clock_timer.time_millis() >= UPDATE_INTERVAL) {
      updateScreen();
      clock_timer.restart();
    }
  } else {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
}

// Print Screen
void printStringDisplay(int x, int y, const String& text, int fontSize = 1, bool clearDisplay = false, const GFXfont* font = nullptr) {
  if (clearDisplay) {
    display.clearDisplay();
  }
  display.setFont(font);
  display.setTextSize(fontSize);
  display.setTextColor(WHITE);

  display.setCursor(x, y);
  display.print(text);

  display.display();
}

// -- Setup functions --
void setup_display() {
  // Connecting display
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Allocation Display");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Display allocation failed"));
    for (;;);
  }
  Serial.println("Display allocated!");

  display.clearDisplay();
  display.setFont(&Org_01);
  display.dim(true);
}
void setup_wifi() {
  // Connecting WIFI
  Serial.println("Starting WIFI!");
  printStringDisplay(5, 5, "WIFI Starting", 2, true);

  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_2dBm);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);

    i += 1;

    printStringDisplay(5, 5, "Connecting", 2, true);
    printStringDisplay(21, 25, "WiFi", 2);

    switch (i) {
      case 1:
        printStringDisplay(69, 25, ".", 2);
        break;
      case 2:
        printStringDisplay(81, 25, ".", 2);
        break;
      case 3:
        printStringDisplay(93, 25, ".", 2);
        i = 0;
        break;
      default:
        break;
    }
  }
  Serial.println("Connected to WIFI!");
  printStringDisplay(41, 14, "WiFi", 2, true);
  printStringDisplay(11, 36, "Connected", 2);
}
void setup_termometer() {
  if (!sht31.begin(0x44)) {  // Set to 0x45 for alternate I2C address
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
}

// -- Event handler --
void handleButtonMenuPress() {
  menuSelection = (menuSelection + 1) % 2;
  updateScreen();
}
void handleButtonDisplayPress() {
  enableDisplay = !enableDisplay;
  updateScreen();
}

// -- Loop functions --
void updateScreen() {
  if (menuSelection == 0) {
    showClock();
  } else if (menuSelection == 1) {
    showTemperature();
  }
}

// -- Helper functions --
// Format the time with the 0 if the time is 1 digit
String format(int time) {
  return (time < 10 ? "0" : "") + String(time);
}
void showClock() {
  // Getting the time and temerature
  time_t rawtime = time(nullptr);
  struct tm* timeinfo = localtime(&rawtime);

  display.clearDisplay();

  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  // Hour
  if (format(timeinfo->tm_hour).startsWith("1")) {
    display.setCursor(20, 37);
  } else {
    display.setCursor(1, 37);
  }
  display.print(format(timeinfo->tm_hour));

  // :
  display.setCursor(62, 37);
  display.print(":");

  // Minutes
  display.setCursor(72, 37);
  display.print(format(timeinfo->tm_min));

  display.display();
}
void showTemperature() {
  float temp = sht31.readTemperature() + offsetTemp;
  float hum = sht31.readHumidity() + offsetHum;

  display.clearDisplay();

  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  // Temperature
  display.setCursor(6, 22);
  display.print(temp, 1);
  display.setCursor(97, 22);
  display.print("C");

  // Humidity
  display.setCursor(6, 55);
  display.print(hum, 1);
  display.setCursor(97, 55);
  display.print("%");

  display.display();
}