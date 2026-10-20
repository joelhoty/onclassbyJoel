#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// --- 設定 DHT 感測器 ---
#define DHTPIN 7       // DHT 接在數位腳位 7
#define DHTTYPE DHT11  // 使用 DHT11，若為 DHT22 請改為 DHT22
DHT_Unified dht(DHTPIN, DHTTYPE);

// --- 設定 LCD 螢幕 ---
// 預設 I2C 位址為 0x27，16 個字元、2 行 (若螢幕沒反應可檢查是否為 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

uint32_t delayMS;

void setup() {
  Serial.begin(9600);
  
  // 初始化 LCD 螢幕
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // 初始化 DHT 感測器
  dht.begin();
  
  // 取得感測器規格並設定延遲時間
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
  
  // DHT11 讀取間隔建議至少 1~2 秒 (1000ms~2000ms)
  if (delayMS < 1500) {
    delayMS = 1500;
  }
}

void loop() {
  // 感測器讀取間隔延遲
  delay(delayMS);

  sensors_event_t event;

  // --- 讀取並顯示溫度 (第一行) ---
  dht.temperature().getEvent(&event);
  lcd.setCursor(0, 0); // 游標移至第 0 欄、第 0 列 (第一行)
  
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    lcd.print(F("TEMP: Error     "));
  } else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F(" *C"));

    lcd.print(F("TEMP: "));
    lcd.print(event.temperature, 1); // 顯示到小數點後 1 位
    lcd.write(223);                  // LCD 的「°」度數符號
    lcd.print(F("C   "));             // 多留空白確保清除前一次殘留文字
  }

  // --- 讀取並顯示濕度 (第二行) ---
  dht.humidity().getEvent(&event);
  lcd.setCursor(0, 1); // 游標移至第 0 欄、第 1 列 (第二行)
  
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
    lcd.print(F("HUMD: Error     "));
  } else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F(" %"));

    lcd.print(F("HUMD: "));
    lcd.print(event.relative_humidity, 1); // 顯示到小數點後 1 位
    lcd.print(F(" %   "));                 // 多留空白確保清除前一次殘留文字
  }
}