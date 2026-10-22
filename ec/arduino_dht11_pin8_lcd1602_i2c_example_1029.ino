#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// --- 設定 DHT 感測器 ---
#define DHTPIN 8          // DHT 感測器腳位接在數位腳位 8
#define DHTTYPE DHT11     // 感測器型號 DHT 11
DHT_Unified dht(DHTPIN, DHTTYPE);

// --- 設定 LCD 螢幕 ---
// I2C 位址為 0x27，尺寸為 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

uint32_t delayMS;

void setup() {
  Serial.begin(9600);

  // 初始化 LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sensor Starting");

  // 初始化 DHT 感測器
  dht.begin();
  
  // 取得感測器最低讀取間隔時間
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
  if (delayMS < 1000) {
    delayMS = 1000; // 確保讀取間隔至少 1 秒
  }

  delay(1000);
  lcd.clear();
}

void loop() {
  // 等待感測器間隔時間
  delay(delayMS);

  // --- 讀取並顯示溫度 ---
  sensors_event_t event;
  dht.temperature().getEvent(&event);

  lcd.setCursor(0, 0); // 第一行
  lcd.print("Temp: ");
  if (isnan(event.temperature)) {
    lcd.print("Error   ");
    Serial.println(F("Error reading temperature!"));
  } else {
    lcd.print(event.temperature, 1); // 顯示小數點後 1 位
    lcd.print((char)223);            // 顯示度數符號 '°'
    lcd.print("C   ");
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
  }

  // --- 讀取並顯示濕度 ---
  dht.humidity().getEvent(&event);

  lcd.setCursor(0, 1); // 第二行
  lcd.print("Humi: ");
  if (isnan(event.relative_humidity)) {
    lcd.print("Error   ");
    Serial.println(F("Error reading humidity!"));
  } else {
    lcd.print(event.relative_humidity, 1); // 顯示小數點後 1 位
    lcd.print("%   ");
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
  }
}