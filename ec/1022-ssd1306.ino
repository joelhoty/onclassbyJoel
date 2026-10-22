#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// --- 感測器設定 ---
#define DHTPIN 8          // DHT11 訊號腳位接在數位腳位 8
#define DHTTYPE DHT11     // 感測器型號 DHT11

DHT_Unified dht(DHTPIN, DHTTYPE);

// --- OLED SSD1306 (128x64) 設定 (取自 u8g2.txt 第 87 行) ---
// 軟體 I2C：SCL 接 Arduino SCL (A5)，SDA 接 Arduino SDA (A4)
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

// 若要改用硬體 I2C (速度更快)，可註解上一行並取消註解下一行：
// U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

uint32_t delayMS;

void setup() {
  Serial.begin(9600);

  // 初始化 U8g2 OLED 螢幕
  u8g2.begin();

  // 初始化 DHT 感測器
  dht.begin();

  // 取得感測器規格並設定延遲時間 (DHT11 建議間隔至少 1.5 秒)
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
  if (delayMS < 1500) {
    delayMS = 1500;
  }
}

void loop() {
  // 感測器讀取間隔延遲
  delay(delayMS);

  // 讀取溫度與濕度數值
  sensors_event_t temp_event, hum_event;
  dht.temperature().getEvent(&temp_event);
  dht.humidity().getEvent(&hum_event);

  // 序列埠監控輸出 (偵錯用)
  if (isnan(temp_event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  } else {
    Serial.print(F("Temperature: "));
    Serial.print(temp_event.temperature);
    Serial.println(F(" *C"));
  }

  if (isnan(hum_event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  } else {
    Serial.print(F("Humidity: "));
    Serial.print(hum_event.relative_humidity);
    Serial.println(F(" %"));
  }

  // --- U8g2 繪圖畫面迴圈 (Page Buffer) ---
  u8g2.firstPage();
  do {
    // 設定字型 (使用 u8g2.txt 範例的 u8g2_font_ncenB10_tr)
    u8g2.setFont(u8g2_font_ncenB10_tr);

    // 第一行：顯示 temp: 溫度 (y=24)
    u8g2.setCursor(0, 24);
    u8g2.print("temp: ");
    if (isnan(temp_event.temperature)) {
      u8g2.print("Error");
    } else {
      u8g2.print(temp_event.temperature, 1); // 顯示小數點後 1 位
      u8g2.print(" C");
    }

    // 第二行：顯示 humd: 濕度 (y=52)
    u8g2.setCursor(0, 52);
    u8g2.print("humd: ");
    if (isnan(hum_event.relative_humidity)) {
      u8g2.print("Error");
    } else {
      u8g2.print(hum_event.relative_humidity, 1); // 顯示小數點後 1 位
      u8g2.print(" %");
    }
  } while (u8g2.nextPage());
}
