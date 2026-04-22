#include <Adafruit_GFX.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <time.h>

// Timer
#include <passive_timer.h>

// Debouncer
#include <EasyButton.h>

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

WiFiMulti wifiMulti;

// Setup display
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire, -1);
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Clock offset (Italian offset)
const char* TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0/3";

// Offset Temperature
float offsetTemp = -1.0;
// Offset Humidity
float offsetHum = -3.0;

char timeBuffer[6];

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
  configTzTime(TZ_INFO, "it.pool.ntp.org", "pool.ntp.org", "time.nist.gov");

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
void printStringDisplay(int x, int y, const String& text, int fontSize = 1, bool clearDisplay = false) {
  if(clearDisplay){
    display.clearDisplay();
  }
  display.setFont();
  display.setTextSize(fontSize);
  display.setTextColor(WHITE);
  display.setTextWrap(false);

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
  display.setFont();
  display.setTextWrap(false);
  display.dim(true);
}
void setup_wifi() {
  // Connecting WIFI
  Serial.println("Starting WIFI!");
  printStringDisplay(5, 5, "WIFI Starting", 1, true);

  WiFi.mode(WIFI_STA);

  wifiMulti.addAP("ssid", "password");

  WiFi.setTxPower(WIFI_POWER_11dBm);

  int dotCount = 0;
  String dots = "";

  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    
    dotCount++;
    if (dotCount > 3) dotCount = 1;
    dots = "";
    for(int j=0; j<dotCount; j++) dots += ".";

    Serial.print(".");

    printStringDisplay(5, 5, "Connecting", 2, true);
    printStringDisplay(21, 25, "WiFi" + dots, 2);
  }

  WiFi.setTxPower(WIFI_POWER_2dBm);

  Serial.println("\nConnected to: " + WiFi.SSID());
  
  printStringDisplay(5, 5, "WiFi Online", 1, true);
  printStringDisplay(5, 20, WiFi.SSID().c_str(), 1);
  printStringDisplay(5, 40, "Connected!", 1);
  
  delay(500);
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
  // Getting the time and temperature
  time_t rawtime = time(nullptr);
  struct tm* timeinfo = localtime(&rawtime);

  display.clearDisplay();

  display.setFont();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  display.setCursor(6, 18);

  sprintf(timeBuffer, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  display.print(timeBuffer);

  display.display();
}
void showTemperature() {
  float temp = sht31.readTemperature() + offsetTemp;
  float hum = sht31.readHumidity() + offsetHum;

  display.clearDisplay();

  display.setFont();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  // Temperature
  display.setCursor(12, 6);
  display.print(temp, 1);
  display.setCursor(101, 6);
  display.print("C");

  // Humidity
  display.setCursor(12, 36);
  display.print(hum, 1);
  display.setCursor(101, 36);
  display.print("%");

  display.display();
}