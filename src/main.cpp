#include <Arduino.h>
#include <Arduino_JSON.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <TinyGPS++.h>

const int AirValue = 1800;      // you need to replace this value with Value_1
const int WaterValue = 1040; // you need to replace this value with Value_2
const int SensorPin = 33;
int soilMoistureValue = 0;
int soilmoisturepercent = 0;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define TXD2 17
#define RXD2 16

TinyGPSPlus gps;

// const char *ssid = "KKkkkkKKK";
const char *ssid = "redmi";
// const char *password = "12345679";
const char *password = "987654321";

char serverAddress[] = "tanah-longosor-be.herokuapp.com";
// char serverAddress[] = "192.168.43.125";
int serverPort = 3004;

WiFiClient wifi;
// HttpClient client = HttpClient(wifi, serverAddress, serverPort);
HttpClient client = HttpClient(wifi, serverAddress);

String cek_id()
{
  String id, buf_id;
  uint64_t chipid;
  char ssid[13];
  chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
  uint16_t chip = (uint16_t)(chipid >> 32);
  snprintf(ssid, 13, "%04X%08X", chip, (uint32_t)chipid);
  for (int i = 0; i < 12; i++)
  {
    buf_id += String(ssid[i]);
  }
  id = buf_id;
  buf_id = "";
  // Serial.println(id);
  return id;
}

void wifi_connect_setup()
{
  // wifi connection timeout = 15 seconds , if it takes longer than that, reboot the ESP32
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi..");
    display.clearDisplay(); // clears the screen buffer
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Connecting to WiFi..");
    display.display();
    if (millis() - start > 10000)
    {
      Serial.println("WiFi connection timeout");
      display.clearDisplay(); // clears the screen buffer
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.println("WiFi connection timeout");
      display.setCursor(0, 20);
      display.println("Rebooting ESP32..");
      display.display();
      delay(2000);
      ESP.restart();
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to the WiFi network");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  display.clearDisplay(); // clears the screen buffer
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Connected to WiFi");
  display.setCursor(0, 10);
  display.println("SSID: " + WiFi.SSID());
  display.setCursor(0, 20);
  display.println("IP Address: " + ip.toString());
  display.display();
  delay(2000);
}

void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  Serial.println("Started......");
  // initialize with the I2C address 0x3C (for the 128x64)
  display.clearDisplay(); // clears the screen buffer

  // display intro message
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Selamat datang di");
  display.setCursor(0, 10);
  display.println("Sistem Pengamatan");
  display.setCursor(0, 20);
  display.println("Tanah Longsor");
  display.setCursor(0, 30);
  display.println("Cek Koneksi WI-FI");
  display.display();
  delay(2000);

  WiFi.begin(ssid, password);
  pinMode(LED_BUILTIN, OUTPUT);

  wifi_connect_setup();
}

void print_speed(int soilMoistureValue)
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (gps.location.isValid() == 1)
  {
    // String gps_speed = String(gps.speed.kmph());
    display.setTextSize(1);

    display.setCursor(25, 5);
    display.print("Lat: ");
    display.setCursor(50, 5);
    display.print(gps.location.lat(), 6);

    display.setCursor(25, 20);
    display.print("Lng: ");
    display.setCursor(50, 20);
    display.print(gps.location.lng(), 6);

    display.setCursor(25, 35);
    display.print("val: ");
    display.setCursor(65, 35);
    display.print(soilMoistureValue);
    display.print(" %");

    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print("SAT:");
    display.setCursor(25, 50);
    display.print(gps.satellites.value());

    display.setTextSize(1);
    display.setCursor(70, 50);
    display.print("ALT:");
    display.setCursor(95, 50);
    display.print(gps.altitude.meters(), 0);
    display.display();
    // send latitude ,longitude ,id ,soilMoistureValue to server
    client.post("/api/from_esp32", "application/json", "{\"latitude\":\"" + String(gps.location.lat()) + "\",\"longitude\":\"" + String(gps.location.lng()) + "\",\"id\":\"" + cek_id() + "\",\"soilMoistureValue\":\"" + String(soilMoistureValue) + "\"}");
  }
  else
  {
    Serial.println("GPS is not valid in this print_speed()");
    display.setTextSize(1);
    display.setCursor(25, 5);
    display.print("GPS: ");
    display.setCursor(50, 5);
    // display.print(' ');
    display.print("Searching..");

    display.setCursor(25, 20);
    display.print("Val: ");
    display.setCursor(50, 20);
    display.print(soilMoistureValue);
    display.print(" %");
    display.display();
    client.post("/api/from_esp32", "application/json", "{\"latitude\":\"" + String("") + "\",\"longitude\":\"" + String("") + "\",\"id\":\"" + cek_id() + "\",\"soilMoistureValue\":\"" + String(soilMoistureValue) + "\"}");
  }
  client.endRequest();
  // int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  JSONVar myObject = JSON.parse(response);
  if (JSON.typeof(myObject) == "undefined")
  {
    Serial.println("Parsing input failed!");
    return;
  }
  else
  {
    Serial.println("Parsing input success!");
    Serial.println(myObject);
    // Serial.println(myObject["data"]["status"]);
  }
}

void wifi_reconnect()
{
  unsigned long start = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi..");
    display.clearDisplay(); // clears the screen buffer
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Connecting to WiFi..");
    display.display();
    WiFi.disconnect();
    WiFi.reconnect();
    if (millis() - start > 15000)
    {
      Serial.println("WiFi connection timeout");
      display.clearDisplay(); // clears the screen buffer
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.println("WiFi connection timeout");
      display.setCursor(0, 20);
      display.println("Rebooting ESP32..");
      display.display();
      delay(2000);
      ESP.restart();
    }

    delay(500);
  }
  IPAddress ip = WiFi.localIP();
  delay(500);
}

void loop()
{
  // wifi_reconnect();
  soilMoistureValue = analogRead(SensorPin);
  Serial.println(soilMoistureValue);
  // Serial.println(" inital value of soil moisture");
  soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);

  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (Serial2.available())
    {
      if (gps.encode(Serial2.read()))
      {
        newData = true;
      }
    }
  }

  // If newData is true
  if (newData == true)
  {
    newData = false;
    Serial.println(gps.satellites.value());
    print_speed(soilmoisturepercent);
  }
  else
  {
    Serial.println("No Data in this loop()");
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(25, 5);
    display.print("GPS: ");
    display.setCursor(50, 5);
    // display.print();
    display.print("No Data");

    display.setCursor(25, 20);
    display.print("Val: ");
    display.setCursor(50, 20);
    display.print(soilMoistureValue);
    display.print(" %");
    display.display();
    client.post("/api/from_esp32", "application/json", "{\"latitude\":\"" + String("") + "\",\"longitude\":\"" + String("") + "\",\"id\":\"" + cek_id() + "\",\"soilMoistureValue\":\"" + String(soilMoistureValue) + "\"}");
    // int statusCode = client.responseStatusCode();
    String response = client.responseBody();
    JSONVar myObject = JSON.parse(response);
    if (JSON.typeof(myObject) == "undefined")
    {
      Serial.println("Parsing input failed!");
      return;
    }
    else
    {
      Serial.println("Parsing input success!");
      Serial.println(myObject);
    }
  }

  delay(1000);
}