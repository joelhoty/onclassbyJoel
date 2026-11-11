// ============================================================================
// 1110-1.ino — NodeMCU-32S (ESP32) + Wi-Fi DHCP Web Server + DHT 溫濕度 + SSD1306 OLED
// 整合功能：
//   1. 連接 Wi-Fi (SSID: ite-eng, WPA2: cygsh1811) 並透過 DHCP 取得 IP
//   2. 建立 HTTP Web Server (Port 80)，瀏覽器輸入 http://<IP>/ 即時顯示溫濕度與天氣符號
//   3. SSD1306 128x64 OLED 同時顯示：48x48 天氣符號、溫度、濕度、以及 DHCP IP 位址
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
// 【需要的函式庫】
//   - WiFi.h / WebServer.h (Arduino ESP32 核心內建)
//   - DHT sensor library (by Adafruit)
//   - Adafruit Unified Sensor
//   - U8g2 (by olikraus)
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// -------------------- 1. Wi-Fi 與 Web Server 設定 --------------------
const char* WIFI_SSID = "ite-eng";
const char* WIFI_PASS = "cygsh1811";

WebServer server(80); // 建立 HTTP Web Server 於 Port 80

// -------------------- 2. DHT 感測器設定 --------------------
#define DHTPIN 16          // DHT 資料腳位接 NodeMCU-32S 的 GPIO 16

// 請依實際使用的感測器型號取消註解對應行：
//#define DHTTYPE    DHT11   // DHT 11 (藍色外殼)
#define DHTTYPE    DHT22   // DHT 22 / AM2302 (白色外殼，1110-1.txt 預設)
//#define DHTTYPE    DHT21   // DHT 21 / AM2301

DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS = 2000;
uint32_t lastSensorRead = 0;

// 儲存最新感測資料供 OLED 與 Web Server 共用
float   currentTemp   = NAN;
float   currentHum    = NAN;
uint8_t currentSymbol = 2; // 預設 CLOUD

// -------------------- 3. OLED SSD1306 (128x64) 設定 --------------------
// 使用硬體 I2C (NodeMCU-32S 預設：SCL = GPIO 22, SDA = GPIO 21)
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// -------------------- 4. 天氣符號常數定義 (取自 Weather.ino) --------------------
#define SUN        0   // 晴天 (Glyph 69)
#define SUN_CLOUD  1   // 晴時多雲 (Glyph 65)
#define CLOUD      2   // 多雲 / 陰天 (Glyph 64)
#define RAIN       3   // 下雨 / 高濕 (Glyph 67)
#define THUNDER    4   // 雷雨 / 悶熱高濕 (Embedded Glyph 67)

// -------------------- 5. 依溫濕度決定天氣符號與名稱 --------------------
uint8_t getWeatherSymbol(float temp, float hum) {
  if (isnan(temp) || isnan(hum)) {
    return CLOUD;                 // 讀取失敗時預設顯示雲朵
  }
  if (hum >= 85.0 && temp >= 28.0) {
    return THUNDER;               // 高溫且高濕：雷雨符號
  } else if (hum >= 80.0) {
    return RAIN;                  // 濕度 >= 80%：下雨符號
  } else if (hum >= 70.0) {
    return CLOUD;                 // 濕度 70% ~ 79%：多雲符號
  } else if (hum >= 55.0) {
    return SUN_CLOUD;             // 濕度 55% ~ 69%：晴時多雲
  } else {
    return SUN;                   // 濕度 < 55%：太陽符號
  }
}

const char* getWeatherName(uint8_t symbol) {
  switch (symbol) {
    case SUN:       return "晴天 (Sunny)";
    case SUN_CLOUD: return "晴時多雲 (Partly Cloudy)";
    case CLOUD:     return "多雲 (Cloudy)";
    case RAIN:      return "下雨 / 潮濕 (Rainy)";
    case THUNDER:   return "雷雨 / 悶熱 (Thunderstorm)";
    default:        return "未知 (Unknown)";
  }
}

const char* getWeatherEmoji(uint8_t symbol) {
  switch (symbol) {
    case SUN:       return "☀️";
    case SUN_CLOUD: return "⛅";
    case CLOUD:     return "☁️";
    case RAIN:      return "🌧️";
    case THUNDER:   return "⛈️";
    default:        return "☁️";
  }
}

// -------------------- 6. 繪製 48x48 天氣圖示 (取自 Weather.ino) --------------------
void drawWeatherSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) {
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

// -------------------- 7. 繪製 OLED 畫面：天氣符號 + 溫濕度 + IP 位址 --------------------
void updateOLED() {
  String ipStr = (WiFi.status() == WL_CONNECTED)
                   ? WiFi.localIP().toString()
                   : String("Connecting...");

  u8g2.firstPage();
  do {
    // (A) 左上 (0, 48) 繪製 48x48 天氣圖示
    drawWeatherSymbol(0, 48, currentSymbol);

    // (B) 右上區塊 (x=52) 分兩行顯示溫度與濕度
    u8g2.setFont(u8g2_font_helvB14_tf);

    // 第一行 (y=20)：溫度
    u8g2.setCursor(52, 20);
    if (isnan(currentTemp)) {
      u8g2.print("--.-°C");
    } else {
      u8g2.print(currentTemp, 1);
      u8g2.print("°C");
    }

    // 第二行 (y=44)：濕度
    u8g2.setCursor(52, 44);
    if (isnan(currentHum)) {
      u8g2.print("--.- %");
    } else {
      u8g2.print(currentHum, 1);
      u8g2.print(" %");
    }

    // (C) 底部分隔線與 DHCP IP 位址 (y=62)
    u8g2.drawHLine(0, 50, 128);
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.setCursor(0, 62);
    u8g2.print("IP:");
    u8g2.print(ipStr);
  } while (u8g2.nextPage());
}

// -------------------- 8. HTTP Web Server 路由處理 --------------------
// 回傳 JSON 格式資料 (/api/weather)，供網頁每秒自動無縫更新
void handleApiWeather() {
  String json = "{";
  json += "\"valid\":" + String((!isnan(currentTemp) && !isnan(currentHum)) ? "true" : "false") + ",";
  json += "\"temp\":" + (isnan(currentTemp) ? String("null") : String(currentTemp, 1)) + ",";
  json += "\"hum\":" + (isnan(currentHum) ? String("null") : String(currentHum, 1)) + ",";
  json += "\"symbol\":" + String(currentSymbol) + ",";
  json += "\"weatherName\":\"" + String(getWeatherName(currentSymbol)) + "\",";
  json += "\"emoji\":\"" + String(getWeatherEmoji(currentSymbol)) + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"ssid\":\"" + String(WIFI_SSID) + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI());
  json += "}";
  server.send(200, "application/json; charset=utf-8", json);
}

// 回傳主網頁 (/)，支援電腦與手機瀏覽器即時檢視溫濕度與天氣符號
void handleRoot() {
  String tempText = isnan(currentTemp) ? "--.-" : String(currentTemp, 1);
  String humText  = isnan(currentHum)  ? "--.-" : String(currentHum, 1);

  String html = R"rawliteral(<!DOCTYPE html>
<html lang="zh-TW">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>NodeMCU-32S 溫濕度與天氣觀測站</title>
  <style>
    body {
      margin: 0; padding: 24px 16px;
      font-family: "Segoe UI", "Microsoft JhengHei", sans-serif;
      background: #0f172a; color: #f8fafc;
      display: flex; justify-content: center; align-items: center; min-height: 90vh;
    }
    .card {
      background: #1e293b; border: 1px solid #334155; border-radius: 18px;
      max-width: 420px; width: 100%; padding: 28px 24px;
      box-shadow: 0 12px 32px rgba(0,0,0,0.45); text-align: center;
    }
    .title { font-size: 1.25rem; font-weight: 700; color: #38bdf8; margin-bottom: 6px; }
    .sub { font-size: 0.85rem; color: #94a3b8; margin-bottom: 20px; }
    .weather-icon { font-size: 4.8rem; line-height: 1.1; margin: 10px 0; }
    .weather-name {
      display: inline-block; background: rgba(56, 189, 248, 0.15);
      color: #7dd3fc; border: 1px solid rgba(56, 189, 248, 0.35);
      padding: 4px 14px; border-radius: 999px; font-size: 0.95rem; font-weight: 600;
      margin-bottom: 20px;
    }
    .metrics { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; margin-bottom: 20px; }
    .box {
      background: #0f172a; border: 1px solid #334155; border-radius: 12px; padding: 14px 10px;
    }
    .label { font-size: 0.82rem; color: #94a3b8; margin-bottom: 6px; }
    .val { font-size: 1.75rem; font-weight: 700; color: #f8fafc; }
    .unit { font-size: 0.95rem; color: #38bdf8; }
    .footer {
      font-size: 0.8rem; color: #64748b; border-top: 1px solid #334155; padding-top: 12px;
      display: flex; justify-content: space-between;
    }
  </style>
</head>
<body>
  <div class="card">
    <div class="title">🌤️ NodeMCU-32S 即時天氣站</div>
    <div class="sub">DHT Sensor (GPIO 16) + SSD1306 OLED (I2C)</div>
    <div id="emoji" class="weather-icon">)rawliteral" + String(getWeatherEmoji(currentSymbol)) + R"rawliteral(</div>
    <div id="wname" class="weather-name">)rawliteral" + String(getWeatherName(currentSymbol)) + R"rawliteral(</div>
    <div class="metrics">
      <div class="box">
        <div class="label">🌡️ 溫度 (Temperature)</div>
        <div class="val"><span id="temp">)rawliteral" + tempText + R"rawliteral(</span> <span class="unit">°C</span></div>
      </div>
      <div class="box">
        <div class="label">💧 濕度 (Humidity)</div>
        <div class="val"><span id="hum">)rawliteral" + humText + R"rawliteral(</span> <span class="unit">%</span></div>
      </div>
    </div>
    <div class="footer">
      <span>SSID: <strong id="ssid">)rawliteral" + String(WIFI_SSID) + R"rawliteral(</strong></span>
      <span>IP: <strong id="ip">)rawliteral" + WiFi.localIP().toString() + R"rawliteral(</strong></span>
    </div>
  </div>
  <script>
    setInterval(async () => {
      try {
        const res = await fetch('/api/weather');
        const d = await res.json();
        document.getElementById('emoji').textContent = d.emoji;
        document.getElementById('wname').textContent = d.weatherName;
        document.getElementById('temp').textContent = d.temp !== null ? d.temp.toFixed(1) : '--.-';
        document.getElementById('hum').textContent = d.hum !== null ? d.hum.toFixed(1) : '--.-';
        document.getElementById('ip').textContent = d.ip;
      } catch (e) {}
    }, 2000);
  </script>
</body>
</html>)rawliteral";

  server.send(200, "text/html; charset=utf-8", html);
}

// -------------------- 9. 讀取感測器並更新畫面 --------------------
void readSensorAndUpdate() {
  sensors_event_t temp_event, hum_event;
  dht.temperature().getEvent(&temp_event);
  dht.humidity().getEvent(&hum_event);

  currentTemp = temp_event.temperature;
  currentHum  = hum_event.relative_humidity;

  if (isnan(currentTemp)) {
    Serial.println(F("Error reading temperature!"));
  } else {
    Serial.print(F("Temperature: "));
    Serial.print(currentTemp, 1);
    Serial.println(F(" °C"));
  }

  if (isnan(currentHum)) {
    Serial.println(F("Error reading humidity!"));
  } else {
    Serial.print(F("Humidity:    "));
    Serial.print(currentHum, 1);
    Serial.println(F(" %"));
  }

  currentSymbol = getWeatherSymbol(currentTemp, currentHum);
  updateOLED();
}

// -------------------- 10. 初始化設定 setup() --------------------
void setup(void) {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println(F("============================================"));
  Serial.println(F("NodeMCU-32S DHT + OLED + WiFi WebServer 啟動"));
  Serial.println(F("============================================"));

  // 1. 初始化 OLED
  u8g2.begin();
  u8g2.enableUTF8Print();

  // 2. 初始化 DHT 感測器
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
  if (delayMS < 2000) {
    delayMS = 2000;
  }

  // 3. 連接 Wi-Fi (使用 DHCP 自動取得 IP)
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print(F("正在連線至 Wi-Fi SSID: "));
  Serial.println(WIFI_SSID);

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 30) {
    delay(500);
    Serial.print(".");
    retryCount++;

    // 在 OLED 顯示連線進度
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_8x13_mf);
      u8g2.setCursor(0, 18);
      u8g2.print("WiFi Connecting");
      u8g2.setCursor(0, 36);
      u8g2.print("SSID: ");
      u8g2.print(WIFI_SSID);
      u8g2.setCursor(0, 54);
      for (int i = 0; i < (retryCount % 15); i++) {
        u8g2.print(".");
      }
    } while (u8g2.nextPage());
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("Wi-Fi 連線成功！"));
    Serial.print(F("DHCP IP 位址: http://"));
    Serial.print(WiFi.localIP());
    Serial.println(F("/"));
  } else {
    Serial.println(F("Wi-Fi 連線逾時，將在背景持續嘗試重連..."));
  }

  // 4. 註冊 Web Server 路由並啟動服務
  server.on("/", handleRoot);
  server.on("/api/weather", handleApiWeather);
  server.begin();
  Serial.println(F("HTTP Web Server 已啟動 (Port 80)"));

  // 5. 立即執行第一次溫濕度讀取與 OLED 畫面更新
  readSensorAndUpdate();
  lastSensorRead = millis();
}

// -------------------- 11. 主迴圈 loop() --------------------
void loop(void) {
  // 1. 處理客戶端 HTTP 網頁請求 (非阻塞，確保網頁秒開)
  server.handleClient();

  // 2. 每隔 delayMS (2000ms) 讀取一次 DHT 溫濕度並刷新 OLED
  uint32_t now = millis();
  if (now - lastSensorRead >= delayMS) {
    lastSensorRead = now;
    readSensorAndUpdate();
  }
}
