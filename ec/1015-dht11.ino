// 



#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// --- 硬體腳位與設定 ---
#define DHTPIN 8          // DHT 感測器接在數位腳位 8
#define DHTTYPE DHT11     // 感測器型號 DHT11

// 設定 LCD：位址通常為 0x27，規格為 16 字元、2 行
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

void setup() {
  Serial.begin(9600);

  // 初始化 LCD 螢幕與背光
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // 初始化 DHT 感測器
  dht.begin();

  // 取得 DHT 感測器所需的最小讀取間隔時間（DHT11 通常至少需 1~2 秒）
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
  if (delayMS < 1500) {
    delayMS = 1500; // DHT11 建議間隔至少 1.5 秒以上較為穩定
  }

  // 開機提示畫面
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(1000);
  lcd.clear();
}

void loop() {
  // 等待感測器間隔
  delay(delayMS);

  sensors_event_t event;

  // --- 讀取並顯示溫度（第 1 行）---
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    lcd.print("Error   ");
  } else {
    lcd.print(event.temperature, 1); // 顯示小數點後 1 位
    lcd.print((char)223);            // LCD 內建的度數符號 '°'
    lcd.print("C  ");                // 後方補空格避免位數變短時殘留舊字
  }

  // --- 讀取並顯示濕度（第 2 行）---
  lcd.setCursor(0, 1);
  lcd.print("Humd: ");
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    lcd.print("Error   ");
  } else {
    lcd.print(event.relative_humidity, 1); // 顯示小數點後 1 位
    lcd.print("%   ");
  }
}