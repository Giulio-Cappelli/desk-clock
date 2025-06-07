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

// Pin I2C
#define SDA_PIN 9
#define SCL_PIN 10

// Pin interrupt menu selection
#define MENU_BUTTON_PIN 0
int menuSelection;

// Pin interrupt display on/off
#define DISPLAY_BUTTON_PIN 1
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

// Interrupt Handler
void IRAM_ATTR changeModeHandler() {
  menuSelection += 1;
  menuSelection %= 2;
}
void IRAM_ATTR enableDisplayHandler() {
  enableDisplay = !enableDisplay;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Setup Menu Button
  pinMode(MENU_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MENU_BUTTON_PIN), changeModeHandler, FALLING);
  menuSelection = 0;

  // Setup Diaply Button
  pinMode(DISPLAY_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DISPLAY_BUTTON_PIN), enableDisplayHandler, FALLING);
  enableDisplay = true;

  // Connecting display
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Allocation Display");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Display allocation failed"));
    for (;;);
  }
  Serial.println("Display allocated!");

  display.clearDisplay();
  display.dim(true);

  // Connecting WIFI
  Serial.println("Starting WIFI!");
  printStringDisplay("WIFI Starting", 26, 13);

  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_2dBm);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("Connected to WIFI!");
  printStringDisplay("WiFi Connected", 26, 13);

  // Connecting Termometer
  if (!sht31.begin(0x44)) {  // Set to 0x45 for alternate I2C address
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  // Connecting to NTP server
  configTime(GMTOffset, daylightOffset, "it.pool.ntp.org", "pool.ntp.org", "time.nist.gov");

  delay(1000);
}
void loop() {
  // Getting the time and temerature
  time_t rawtime = time(nullptr);
  struct tm* timeinfo = localtime(&rawtime);

  float temp = sht31.readTemperature() + offsetTemp;
  float hum = sht31.readHumidity() + offsetHum;

  // Switch pages with the button
  switch (menuSelection) {
    case 0:
      printTimeDisplay(timeinfo);
      break;
    case 1:
      printTempDisplay(temp, hum);
      break;
    default:
      break;
  }

  if (enableDisplay) {
    // display.dim(true);
    display.ssd1306_command(SSD1306_DISPLAYON)
  } else {
    // display.dim(false);
    display.ssd1306_command(SSD1306_DISPLAYOFF)
  }

  delay(2000);
}

// Format the time with the 0 if the time is 1 digit
String format(int time) {
  return (time < 10 ? "0" : "") + String(time);
}

void printTimeDisplay(struct tm* timeinfo) {
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  display.setCursor(24, 6);
  display.print(format(timeinfo->tm_hour));
  display.setCursor(57, 6);
  display.print(":");
  display.setCursor(72, 6);
  display.print(format(timeinfo->tm_min));

  display.display();
}

void printTempDisplay(float temp, float hum) {
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  display.setCursor(32, 1);
  display.print(temp, 1);
  display.setCursor(100, 1);
  display.print("C");

  display.setCursor(32, 18);
  display.print(hum, 1);
  display.setCursor(100, 18);
  display.print("%");

  display.display();
}

void printStringDisplay(String text, int x, int y) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  // display.drawRect(0, 0, 128, 32, WHITE);

  display.setCursor(x, y);
  display.print(text);

  display.display();
}