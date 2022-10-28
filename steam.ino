#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define ONE_WIRE_BUS 23 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

LiquidCrystal_I2C lcd(0x27, 16, 2);
const int waterLavel = 2;
 int sp =5;
void setup(void) {
Serial.begin(9600);
sensors.begin();
lcd.begin();
lcd.display();
lcd.backlight();
pinMode(sp,OUTPUT);
}
 
void loop(void) {
lcd.clear();   
lcd.setCursor(0,0);
lcd.print("Temp:");
lcd.setCursor(0,1);
lcd.print("Wather:");


 
sensors.requestTemperatures(); 
float temp = sensors.getTempCByIndex(0); 

lcd.setCursor(6,0);
lcd.print(temp);

int buttonState = digitalRead(waterLavel);
   
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


delay(250);
}
