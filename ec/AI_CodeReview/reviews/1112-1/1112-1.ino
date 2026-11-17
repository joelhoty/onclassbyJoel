// ============================================================================
// 1112-1.ino — NodeMCU-32S (ESP32) + DHT 溫濕度感測 + SSD1306 128x64 OLED 版面設計
// 整合來源：1112-1.txt (Adafruit DHT_Unified 範例 + U8g2 Weather.ino 範例)
//
// 【功能特色】
//   1. 透過 NodeMCU-32S 的 GPIO 16 讀取 DHT22/DHT11 溫度 (°C) 與相對濕度 (%RH)
//   2. 依據即時溫濕度自動判斷 5 種天氣狀態，於左側繪製 48x48 Open Iconic 天氣符號
//   3. 規劃 128x64 OLED 黃金三區版面（左區 48x48 天氣圖示、右區溫度計/水滴符號與數值、底區狀態列）
//   4. 採用 millis() 非阻塞定時讀取感測器，搭配底部平滑跑馬燈狀態列
//
// 【硬體接線 (NodeMCU-32S / ESP32)】
//   1. DHT 溫濕度感測器：
//      - VCC  -> 3V3 (3.3V)
//      - DATA -> GPIO 16 (板子上標示 RX2 / IO16 / 16)
//      - GND  -> GND
//   2. SSD1306 OLED (128x64, I2C 介面)：
//      - GND  -> GND
//      - VCC  -> 3V3 (3.3V)
//      - SCL  -> GPIO 22 (ESP32 預設硬體 I2C SCL)
//      - SDA  -> GPIO 21 (ESP32 預設硬體 I2C SDA)
//
// 【需要的 Arduino 函式庫】
//   - DHT sensor library (by Adafruit)
//   - Adafruit Unified Sensor (by Adafruit)
//   - U8g2 (by olikraus)
// ============================================================================

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// -------------------- 1. DHT 溫濕度感測器設定 --------------------
#define DHTPIN 16          // Digital pin connected to the DHT sensor (NodeMCU-32S GPIO 16)

// 請依實際使用的感測器型號取消註解對應行：
//#define DHTTYPE    DHT11   // DHT 11 (藍色外殼)
#define DHTTYPE    DHT22   // DHT 22 (AM2302，白色外殼，1112-1.txt 預設)
//#define DHTTYPE    DHT21   // DHT 21 (AM2301)

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS = 2000;        // 感測器取樣間隔 (ms)
uint32_t lastSensorRead = 0;    // 上次讀取感測器的時間戳記

// -------------------- 2. SSD1306 128x64 OLED 設定 --------------------
// 使用 ESP32 硬體 I2C (預設腳位：SCL = GPIO 22, SDA = GPIO 21)
// 採用 _1_ (Page Buffer 模式) 兼顧記憶體效率與相容性
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// -------------------- 3. 天氣符號常數定義 (取自 Weather.ino) --------------------
#define SUN        0   // 晴天 (Glyph 69)
#define SUN_CLOUD  1   // 晴時多雲 (Glyph 65)
#define CLOUD      2   // 多雲 / 陰天 (Glyph 64)
#define RAIN       3   // 下雨 / 潮濕 (Glyph 67)
#define THUNDER    4   // 雷雨 / 悶熱高濕 (Embedded Glyph 67)

// 儲存最新溫濕度與天氣狀態
float   currentTemp   = NAN;
float   currentHum    = NAN;
uint8_t currentSymbol = CLOUD;
int16_t scrollOffset  = -128;   // 底部狀態列跑馬燈位移量

// -------------------- 4. 依溫濕度決定天氣符號與狀態文字 --------------------
uint8_t getWeatherSymbol(float temp, float hum) {
  if (isnan(temp) || isnan(hum)) {
    return CLOUD;                 // 感測器讀取失敗時預設顯示雲朵
  }
  if (hum >= 85.0 && temp >= 28.0) {
    return THUNDER;               // 高溫且高濕：雷雨符號
  } else if (hum >= 80.0) {
    return RAIN;                  // 濕度 >= 80%：下雨符號
  } else if (hum >= 70.0) {
    return CLOUD;                 // 濕度 70% ~ 79.9%：多雲符號
  } else if (hum >= 55.0) {
    return SUN_CLOUD;             // 濕度 55% ~ 69.9%：晴時多雲
  } else {
    return SUN;                   // 濕度 < 55%：晴天太陽符號
  }
}

const char* getWeatherStatusText(uint8_t symbol, bool sensorOk) {
  if (!sensorOk) {
    return "Sensor Error! Check GPIO 16";
  }
  switch (symbol) {
    case SUN:       return "SUNNY - Dry & Clear";
    case SUN_CLOUD: return "PARTLY CLOUDY - Comfort";
    case CLOUD:     return "CLOUDY - Mild Humid";
    case RAIN:      return "RAINY - High Humidity";
    case THUNDER:   return "THUNDER - Hot & Humid";
    default:        return "Monitoring...";
  }
}

// -------------------- 5. 繪製左側 48x48 天氣圖示 (取自 Weather.ino) --------------------
void drawWeatherSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) {
  // 使用 U8g2 內建 Open Iconic 6x (48x48) 向量圖示字型
  switch (symbol) {
    case SUN:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 69);
      break;
    case SUN_CLOUD:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 65);
      break;
    case CLOUD:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 64);
      break;
    case RAIN:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 67);
      break;
    case THUNDER:
      u8g2.setFont(u8g2_font_open_iconic_embedded_6x_t);
      u8g2.drawGlyph(x, y, 67);
      break;
  }
}

// -------------------- 6. 繪製右側小圖示：溫度計與水滴符號 --------------------
// 繪製 7x16 px 迷你溫度計符號 (左上座標 x, y)
void drawThermometerIcon(u8g2_uint_t x, u8g2_uint_t y) {
  u8g2.drawRFrame(x + 1, y, 5, 11, 1); // 溫度計玻璃管外框
  u8g2.drawDisc(x + 3, y + 12, 3);     // 底部感溫球
  u8g2.drawVLine(x + 3, y + 4, 6);     // 內部水銀柱
  u8g2.drawHLine(x + 6, y + 2, 2);     // 右側刻度線 1
  u8g2.drawHLine(x + 6, y + 5, 2);     // 右側刻度線 2
  u8g2.drawHLine(x + 6, y + 8, 2);     // 右側刻度線 3
}

// 繪製 7x15 px 迷你水滴濕度符號 (左上座標 x, y)
void drawDropletIcon(u8g2_uint_t x, u8g2_uint_t y) {
  u8g2.drawTriangle(x + 3, y, x, y + 9, x + 6, y + 9); // 水滴上方尖錐
  u8g2.drawDisc(x + 3, y + 10, 3);                     // 水滴下方圓弧
  u8g2.drawPixel(x + 2, y + 9);                        // 修補交界像素
}

// -------------------- 7. 繪製底部跑馬燈文字 (改良自 Weather.ino) --------------------
void drawScrollString(int16_t offset, const char *s) {
  static char buf[36];
  size_t len = strlen(s);
  size_t char_offset = 0;
  u8g2_uint_t dx = 0;
  size_t visible = 0;

  u8g2.setFont(u8g2_font_8x13_mf);
  if (offset < 0) {
    char_offset = (-offset) / 8;
    dx = offset + char_offset * 8;
    if (char_offset >= u8g2.getDisplayWidth() / 8) return;
    visible = u8g2.getDisplayWidth() / 8 - char_offset + 1;
    if (visible > len) visible = len;
    strncpy(buf, s, visible);
    buf[visible] = '\0';
    u8g2.drawStr(char_offset * 8 - dx, 63, buf);
  } else {
    char_offset = offset / 8;
    if (char_offset >= len) return;
    dx = offset - char_offset * 8;
    visible = len - char_offset;
    if (visible > u8g2.getDisplayWidth() / 8 + 1) {
      visible = u8g2.getDisplayWidth() / 8 + 1;
    }
    strncpy(buf, s + char_offset, visible);
    buf[visible] = '\0';
    u8g2.drawStr(-dx, 63, buf);
  }
}

// -------------------- 8. 繪製 128x64 OLED 整合版面 --------------------
/*
  【128x64 像素版面規劃】
  ┌───────────────────┬─────────────────────────────────────────┐ y=0
  │                   │ [🌡️] 26.5 °C   (溫度區 y: 0~24)          │
  │   48x48 天氣符號   ├─────────────────────────────────────────┤ y=25 (分隔線)
  │   (x: 0~47)       │ [💧] 62.0 %    (濕度區 y: 26~49)         │
  ├───────────────────┴─────────────────────────────────────────┤ y=50 (分隔線)
  │  PARTLY CLOUDY - Comfort       (底部跑馬燈狀態列 y: 51~63)   │
  └─────────────────────────────────────────────────────────────┘ y=63
*/
void drawDashboard(const char *statusMsg) {
  // 1. 左區 (x: 0~47, y: 0~48)：繪製 48x48 天氣大圖示
  drawWeatherSymbol(0, 48, currentSymbol);

  // 2. 垂直與水平分隔線，建構儀表板網格感
  u8g2.drawVLine(50, 0, 50);   // 左圖示區與右數值區垂直分隔線 (x=50)
  u8g2.drawHLine(50, 25, 78);  // 溫度與濕度水平分隔線 (y=25)
  u8g2.drawHLine(0, 50, 128);  // 主顯示區與底部狀態列水平分隔線 (y=50)

  // 3. 右上區 (x: 54~127, y: 0~24)：溫度計符號 + 即時溫度
  drawThermometerIcon(54, 4);
  u8g2.setFont(u8g2_font_helvB12_tf);
  u8g2.setCursor(66, 19);
  if (isnan(currentTemp)) {
    u8g2.print("--.-°C");
  } else {
    u8g2.print(currentTemp, 1);
    u8g2.print("°C");
  }

  // 4. 右下區 (x: 54~127, y: 26~49)：水滴符號 + 即時相對濕度
  drawDropletIcon(54, 30);
  u8g2.setFont(u8g2_font_helvB12_tf);
  u8g2.setCursor(66, 45);
  if (isnan(currentHum)) {
    u8g2.print("--.- %");
  } else {
    u8g2.print(currentHum, 1);
    u8g2.print(" %");
  }

  // 5. 底部狀態列 (x: 0~127, y: 51~63)：跑馬燈顯示天氣與舒適度描述
  drawScrollString(scrollOffset, statusMsg);
}

// -------------------- 9. 讀取 DHT 感測器並輸出至序列埠 --------------------
void readDHTSensor() {
  sensors_event_t event;

  // 讀取溫度事件
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    currentTemp = NAN;
  } else {
    currentTemp = event.temperature;
    Serial.print(F("Temperature: "));
    Serial.print(currentTemp, 1);
    Serial.println(F("°C"));
  }

  // 讀取濕度事件
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
    currentHum = NAN;
  } else {
    currentHum = event.relative_humidity;
    Serial.print(F("Humidity:    "));
    Serial.print(currentHum, 1);
    Serial.println(F("%"));
  }

  // 更新對應的天氣圖示代碼
  currentSymbol = getWeatherSymbol(currentTemp, currentHum);
  Serial.println(F("------------------------------------"));
}

// -------------------- 10. 初始化設定 setup() --------------------
void setup(void) {
  Serial.begin(115200); // ESP32 建議使用 115200 baud (亦可改回 9600)
  delay(100);

  // 1. 初始化 SSD1306 OLED
  u8g2.begin();
  u8g2.enableUTF8Print(); // 啟用 UTF-8 支援以正常顯示 "°C"

  // 2. 初始化 DHT 溫濕度感測器並印出感測器硬體資訊
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor + U8g2 OLED Weather Station (1112-1)"));

  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));

  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));

  // 設定感測器最小讀取間隔 (至少 2000ms 以確保穩定)
  delayMS = sensor.min_delay / 1000;
  if (delayMS < 2000) {
    delayMS = 2000;
  }

  // 啟動時立即讀取一次溫濕度
  readDHTSensor();
  lastSensorRead = millis();
}

// -------------------- 11. 主迴圈 loop() --------------------
void loop(void) {
  // 1. 非阻塞定時讀取 DHT 溫濕度 (每 delayMS 毫秒更新一次數值)
  uint32_t now = millis();
  if (now - lastSensorRead >= delayMS) {
    lastSensorRead = now;
    readDHTSensor();
  }

  // 2. 取得目前天氣狀態字串
  bool sensorOk = (!isnan(currentTemp) && !isnan(currentHum));
  const char *statusMsg = getWeatherStatusText(currentSymbol, sensorOk);

  // 3. U8g2 Page Buffer 繪圖迴圈
  u8g2.firstPage();
  do {
    drawDashboard(statusMsg);
  } while (u8g2.nextPage());

  // 4. 更新底部狀態列跑馬燈位移量
  delay(25);
  scrollOffset += 2;
  if (scrollOffset > (int16_t)strlen(statusMsg) * 8 + 4) {
    scrollOffset = -(int16_t)u8g2.getDisplayWidth();
  }
}
