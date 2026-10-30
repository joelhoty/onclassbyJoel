// DHT11 溫濕度感測器 + I2C LCD1602 顯示
// 第一行顯示溫度，第二行顯示濕度
//
// 需要的函式庫:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor
// - LiquidCrystal_I2C

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 8          // DHT 感測器資料腳接 D8
#define DHTTYPE DHT11     // 感測器型號 DHT11
//#define DHTTYPE DHT22   // DHT 22 (AM2302)

DHT_Unified dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD 位址 0x27，16 字 x 2 行

uint32_t delayMS;

void setup() {
  Serial.begin(9600);

  // 初始化 DHT 感測器
  dht.begin();

  // 初始化 LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("DHT11 Starting..");

  // 依感測器規格設定讀取間隔
  sensor_t sensor;
  dht.humidity().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
  if (delayMS < 2000) delayMS = 2000;   // DHT11 建議至少間隔 1~2 秒

  delay(1000);
  lcd.clear();
}

void loop() {
  delay(delayMS);

  sensors_event_t event;

  // ---- 第一行：溫度 ----
  dht.temperature().getEvent(&event);
  lcd.setCursor(0, 0);
  if (isnan(event.temperature)) {
    lcd.print("Temp: Error     ");
    Serial.println(F("Error reading temperature!"));
  } else {
    lcd.print("Temp: ");
    lcd.print(event.temperature, 1);
    lcd.print((char)223);              // LCD 的度數符號 °
    lcd.print("C    ");                // 補空白清除殘留字元
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F(" C"));
  }

  // ---- 第二行：濕度 ----
  dht.humidity().getEvent(&event);
  lcd.setCursor(0, 1);
  if (isnan(event.relative_humidity)) {
    lcd.print("Humi: Error     ");
    Serial.println(F("Error reading humidity!"));
  } else {
    lcd.print("Humi: ");
    lcd.print(event.relative_humidity, 1);
    lcd.print("%     ");               // 補空白清除殘留字元
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
  }
}
