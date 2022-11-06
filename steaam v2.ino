#include <DHTesp.h>         // DHT for ESP32 library
#include <WiFi.h>           // WiFi control for ESP32
#include <ThingsBoard.h>  
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
 
// Helper macro to calculate array size
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
 
// WiFi access point
#define WIFI_AP_NAME        "Serial"
// WiFi password
#define WIFI_PASSWORD       "password"
 
// See https://thingsboard.io/docs/getting-started-guides/helloworld/
// to understand how to obtain an access token
#define TOKEN               "8iLh6HLcvKAciVauabfO"
// ThingsBoard server instance.
#define THINGSBOARD_SERVER  "44.204.112.174"
 
// Baud rate for debug serial
#define SERIAL_DEBUG_BAUD    9600
 
// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;
 
// Array with LEDs that should be lit up one by one
uint8_t leds_cycling[] = { 25, 26, 32 };
// Array with LEDs that should be controlled from ThingsBoard, one by one
uint8_t leds_control[] = { 19, 22, 21 };
 
// DHT object
DHTesp dht;
// ESP32 pin used to query DHT22
#define ONE_WIRE_BUS 23 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
 
// Main application loop delay
int quant = 250;
 int buttonState;
// Initial period of LED cycling.
int led_delay = 250;
// Period of sending a temperature/humidity data.
int send_delay = 250;
 
// Time passed after LED was turned ON, milliseconds.
int led_passed = 0;
// Time passed after temperature/humidity data was sent, milliseconds.
int send_passed = 0;
 
// Set to true if application is subscribed for the RPC messages.
bool subscribed = false;
// LED number that is currenlty ON.
int current_led = 0;
 
// Processes function for RPC call "setValue"
// RPC_Data is a JSON variant, that can be queried using operator[]
// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
RPC_Response processDelayChange(const RPC_Data &data)
{
  Serial.println("Received the set delay RPC method");
 
  // Process data
 
  led_delay = data;
 
  Serial.print("Set new delay: ");
  Serial.println(led_delay);
 
  return RPC_Response(NULL, led_delay);
}
 
// Processes function for RPC call "getValue"
// RPC_Data is a JSON variant, that can be queried using operator[]
// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
RPC_Response processGetDelay(const RPC_Data &data)
{
  Serial.println("Received the get value method");
 
  return RPC_Response(NULL, led_delay);
}
 
// Processes function for RPC call "setGpioStatus"
// RPC_Data is a JSON variant, that can be queried using operator[]
// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
RPC_Response processSetGpioState(const RPC_Data &data)
{
  Serial.println("Received the set GPIO RPC method");
 
  int pin = data["pin"];
  bool enabled = data["enabled"];
 
  if (pin < COUNT_OF(leds_control)) {
    Serial.print("Setting LED ");
    Serial.print(pin);
    Serial.print(" to state ");
    Serial.println(enabled);
 
    digitalWrite(leds_control[pin], enabled);
  }
 
  return RPC_Response(data["pin"], (bool)data["enabled"]);
}
 
// RPC handlers
RPC_Callback callbacks[] = {
  { "setValue",         processDelayChange },
  { "getValue",         processGetDelay },
  { "setGpioStatus",    processSetGpioState },
};
 
LiquidCrystal_I2C lcd(0x27, 16, 2);
const int waterLavel = 2;
 int sp =5;

void setup() {
  // Initialize serial for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);
  WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
  InitWiFi();
  lcd.begin();
  lcd.display();
  lcd.backlight();
  pinMode(sp,OUTPUT);
 
  // Pinconfig
 
  for (size_t i = 0; i < COUNT_OF(leds_cycling); ++i) {
    pinMode(leds_cycling[i], OUTPUT);
  }
 
  for (size_t i = 0; i < COUNT_OF(leds_control); ++i) {
    pinMode(leds_control[i], OUTPUT);
  }
 
  // Initialize temperature sensor
  sensors.begin();

  
}
 
// Main application loop
void loop() {
  
  buttonState = digitalRead(waterLavel);
    sensors.requestTemperatures(); 
    float temp = sensors.getTempCByIndex(0);
    buttonState = digitalRead(waterLavel);
    sensors.requestTemperatures();
  lcd.setCursor(0,0);
  lcd.print("Temp:");
  lcd.setCursor(0,1);
  lcd.print("Wather:");  
 if (buttonState == HIGH) {
        lcd.setCursor(8,1);
        lcd.print("LOW");
        digitalWrite(sp,1);
        delay(500);
        digitalWrite(sp,0);
        delay(500);
       } else {
        lcd.setCursor(8,1);
        lcd.print("HIGH");
        digitalWrite(sp,0);
        }
      
      
  led_passed += quant;
  send_passed += quant;
  
    
  // Check if next LED should be lit up
  if (led_passed > led_delay) {
    // Turn off current LED
    digitalWrite(leds_cycling[current_led], LOW);
    led_passed = 0;
    current_led = current_led >= 2 ? 0 : (current_led + 1);
    // Turn on next LED in a row
    digitalWrite(leds_cycling[current_led], HIGH);
  }
 
  // Reconnect to WiFi, if needed
  if (WiFi.status() != WL_CONNECTED) {
    reconnect();
    return;
  }
 
  // Reconnect to ThingsBoard, if needed
  if (!tb.connected()) {
    subscribed = false;
 
   lcd.setCursor(0,0);
    lcd.print("Connecting to: ");
    
    Serial.print(THINGSBOARD_SERVER);
    lcd.setCursor(0,1);
    lcd.print(" with token ");
    Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
      Serial.println("Failed to connect");
      return;
    }
  }
 
  // Subscribe for RPC, if needed
  if (!subscribed) {
    Serial.println("Subscribing for RPC...");
 
    // Perform a subscription. All consequent data processing will happen in
    // callbacks as denoted by callbacks[] array.
    if (!tb.RPC_Subscribe(callbacks, COUNT_OF(callbacks))) {
      Serial.println("Failed to subscribe for RPC");
      return;
    }
 
    Serial.println("Subscribe done");
    subscribed = true;
  }
 
      lcd.setCursor(6,0);
      lcd.print(temp);
  if (send_passed > send_delay) {
    Serial.println("Sending data...");   
    if (isnan(temp) || isnan(buttonState)) {
      Serial.println("Failed to read from ds18B20 sensor!");
    } else {
      tb.sendTelemetryFloat("temperature", temp);
      tb.sendTelemetryFloat("humidity", buttonState);
      Serial.println(temp);
      Serial.println(buttonState);
      
      
      
    }
 
    send_passed = 0;
  }
      
  // Process messages
  tb.loop();
  lcd.clear();   
  
}
 
void InitWiFi()
{
  lcd.setCursor(0,0);
  lcd.println("Connecting to AP ...");
  // attempt to connect to WiFi network
 
  WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}
 
void reconnect() {
  // Loop until we're reconnected
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected to AP");
  }
}
