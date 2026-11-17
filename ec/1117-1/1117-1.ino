// NodeMCU-32S + DHT22 + SEN0159 CO2 + SSD1306 I2C 128x64 + WiFi WebServer 整合程式
// 整合自 1117.txt 與 DFRobot SEN0159 (MG-811) 範例，支援：
// 1. DHT22 溫溼度感測與圖示自動切換
// 2. DFRobot SEN0159 CO2 感測器 (GPIO 32) 濃度計算與分級警語
// 3. WiFi (SSID: ite-eng) DHCP 取得 IP 並顯示於 OLED 螢幕
// 4. WebServer 支援 AJAX (Fetch JSON API) 即時無刷新數據更新、數值變化高亮提示與 HTML5 Canvas 即時動態折線圖
//
// 硬體接線建議 (NodeMCU-32S):
// - DHT22 DATA 腳位   -> GPIO 16
// - SEN0159 CO2 腳位  -> GPIO 32 (ADC1_CH4，支援與 WiFi 同時使用)
// - SSD1306 I2C SCL   -> GPIO 22 (ESP32 預設硬體 I2C SCL)
// - SSD1306 I2C SDA   -> GPIO 21 (ESP32 預設硬體 I2C SDA)

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// ==================== WiFi 與 WebServer 設定 ====================
const char* WIFI_SSID = "ite-eng";
const char* WIFI_PASS = "cygsh1811";

WebServer server(80);

// ==================== DHT 感測器設定 ====================
#define DHTPIN 16     // DHT 感測器連接的數位腳位 (GPIO 16)
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS = 2000;
unsigned long lastReadTime = 0;

float currentTemp = NAN;
float currentHum = NAN;

// ==================== DFRobot SEN0159 CO2 感測器設定 ====================
// 參考來源：https://wiki.dfrobot.com/sen0159/
#define MG_PIN                 32      // CO2 感測器連接至 GPIO 32 (ESP32 ADC1_CH4)
#define DC_GAIN                8.5     // 放大器直流增益 (DC Gain)
#define READ_SAMPLE_INTERVAL   10      // 每次取樣間隔 (ms)
#define READ_SAMPLE_TIMES      5       // 每次讀取取樣次數
#define ZERO_POINT_VOLTAGE     0.220   // 400ppm 潔淨空氣下之基準電壓 (V)
#define REACTION_VOLTGAE       0.030   // 從潔淨空氣移至 1000ppm CO2 時之電壓降 (V)

// CO2 特性曲線：{ log10(400), ZERO_POINT_VOLTAGE, 斜率 }
float CO2Curve[3] = {2.602, ZERO_POINT_VOLTAGE, (REACTION_VOLTGAE / (2.602 - 4.0))};

float currentCO2Volts = 0.0;
int currentCO2PPM = -1; // -1 代表低於 400ppm (潔淨空氣或預熱中)

// ==================== SSD1306 OLED 設定 ====================
// 使用硬體 I2C (NodeMCU-32S 預設 SDA=21, SCL=22)
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// ==================== 圖示定義 ====================
#define SUN        0  // 晴朗 / 高溫乾燥
#define SUN_CLOUD  1  // 多雲時晴 / 舒適溫溼度
#define CLOUD      2  // 陰天 / 偏涼或乾燥
#define RAIN       3  // 下雨 / 潮濕 (高溼度)
#define THUNDER    4  // 雷雨 / 悶熱潮濕 (高溫+高溼)

// ==================== SEN0159 CO2 讀取與計算函式 ====================
// 讀取 GPIO 32 的平均類比電壓 (ESP32 ADC 為 12-bit: 0~4095 對應 0~3.3V)
float MGRead(int mg_pin)
{
  float v = 0.0;
  for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
    v += analogRead(mg_pin);
    delay(READ_SAMPLE_INTERVAL);
  }
  v = (v / READ_SAMPLE_TIMES) * 3.3 / 4095.0;
  return v;
}

// 根據 SEN0159 曲線將電壓轉換為 CO2 濃度 (ppm)
int MGGetPercentage(float volts, float *pcurve)
{
  if ((volts / DC_GAIN) >= ZERO_POINT_VOLTAGE) {
    return -1; // 代表濃度 < 400 ppm (或感測器正在預熱)
  } else {
    float ppm = pow(10, ((volts / DC_GAIN) - pcurve[1]) / pcurve[2] + pcurve[0]);
    if (ppm > 10000.0) {
      ppm = 10000.0; // 限制最大顯示上限為 10000 ppm (避免腳位浮接時數值溢位)
    }
    return (int)ppm;
  }
}

// 根據 CO2 數值取得對應等級、顏色與警語
void getCO2Warning(int ppm, String &levelTitle, String &warningMsg, String &badgeColor, String &bgColor)
{
  if (ppm == -1 || ppm < 400) {
    levelTitle = "空氣清新 (< 400 ppm)";
    warningMsg = "✅ 空氣品質極佳（濃度低於 400 ppm 或感測器預熱中），室內環境非常清新。";
    badgeColor = "#16a34a"; // 綠色
    bgColor    = "#f0fdf4";
  } else if (ppm <= 800) {
    levelTitle = "空氣品質良好 (正常)";
    warningMsg = "🟢 室內二氧化碳濃度正常，空氣流通良好，適合長時間學習與辦公。";
    badgeColor = "#0284c7"; // 藍色
    bgColor    = "#f0f9ff";
  } else if (ppm <= 1000) {
    levelTitle = "空氣品質普通 (建議通風)";
    warningMsg = "🟡 提醒：CO2 濃度略微上升，空氣開始變悶，建議適度開啟門窗或啟動換氣扇。";
    badgeColor = "#ca8a04"; // 黃色
    bgColor    = "#fefce8";
  } else if (ppm <= 2000) {
    levelTitle = "空氣渾濁 (中度警告)";
    warningMsg = "🟠 警告：CO2 濃度偏高！容易導致嗜睡、精神不濟與注意力下降，請立即開窗通風！";
    badgeColor = "#ea580c"; // 橘色
    bgColor    = "#fff7ed";
  } else {
    levelTitle = "嚴重超標 (危險警報)";
    warningMsg = "🔴 危險警報：CO2 濃度嚴重超標！可能引發頭痛、暈眩、心悸與缺氧不適，請立即強制排風換氣或離開密閉空間！";
    badgeColor = "#dc2626"; // 紅色
    bgColor    = "#fef2f2";
  }
}

// ==================== OLED 繪圖與溫溼度圖示函式 ====================
// 繪製 48x48 天氣/環境圖示
void drawWeatherSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol)
{
  switch(symbol)
  {
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

// 根據溫溼度決定圖示、OLED 簡短狀態與網頁描述
uint8_t selectWeatherSymbol(float temp, float hum, const char **shortStatus, const char **webStatus)
{
  if (isnan(temp) || isnan(hum)) {
    if (shortStatus) *shortStatus = "Sensor Error";
    if (webStatus)   *webStatus   = "感測器讀取失敗，請檢查 DHT 接線";
    return CLOUD;
  }

  // 1. 高溫且高溼 (T >= 28°C 且 H >= 75%) -> 雷電圖示 (悶熱潮濕)
  if (temp >= 28.0 && hum >= 75.0) {
    if (shortStatus) *shortStatus = "Hot & Humid";
    if (webStatus)   *webStatus   = "悶熱潮濕 (建議開啟冷氣或除濕機)";
    return THUNDER;
  }
  // 2. 高溼度 (H >= 75%) -> 下雨圖示 (潮濕)
  if (hum >= 75.0) {
    if (shortStatus) *shortStatus = "Wet / Humid";
    if (webStatus)   *webStatus   = "環境潮濕 (溼度偏高)";
    return RAIN;
  }
  // 3. 高溫 (T >= 28°C) -> 太陽圖示 (炎熱晴朗)
  if (temp >= 28.0) {
    if (shortStatus) *shortStatus = "Warm / Sunny";
    if (webStatus)   *webStatus   = "溫暖炎熱 (請注意補充水分)";
    return SUN;
  }
  // 4. 舒適區間 (20°C <= T < 28°C 且 40% <= H < 75%) -> 多雲時晴 (舒適)
  if (temp >= 20.0 && hum >= 40.0) {
    if (shortStatus) *shortStatus = "Comfortable";
    if (webStatus)   *webStatus   = "舒適宜人 (溫溼度適中)";
    return SUN_CLOUD;
  }
  // 5. 偏冷 (T < 20°C) 或偏乾 (H < 40%) -> 雲朵圖示
  if (shortStatus) *shortStatus = "Cool / Dry";
  if (webStatus)   *webStatus   = "微涼或乾燥";
  return CLOUD;
}

// 繪製 WiFi 連線中畫面
void drawWiFiConnecting(int step)
{
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawStr(0, 16, "Connecting WiFi...");
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.setCursor(0, 34);
    u8g2.print("SSID: ");
    u8g2.print(WIFI_SSID);

    u8g2.setCursor(0, 52);
    for (int i = 0; i <= (step % 16); i++) {
      u8g2.print(".");
    }
  } while (u8g2.nextPage());
}

// 繪製主畫面：左方 48x48 圖示 + 右方溫溼度與 CO2 + 底部顯示 DHCP IP
void drawEnvDashboard()
{
  const char *shortStatus = "";
  uint8_t symbol = selectWeatherSymbol(currentTemp, currentHum, &shortStatus, NULL);

  u8g2.firstPage();
  do {
    // 1. 左側區域 (x: 0~47, y: 0~48): 顯示 48x48 天氣/環境圖示
    drawWeatherSymbol(0, 48, symbol);

    // 2. 右側區域 (x: 52~127): 顯示溫度、溼度與 CO2
    u8g2.setFont(u8g2_font_helvB12_tf);

    // 第一行：溫度 (y = 15)
    u8g2.setCursor(52, 15);
    if (isnan(currentTemp)) {
      u8g2.print("T: --.-C");
    } else {
      u8g2.print("T:");
      u8g2.print(currentTemp, 1);
      u8g2.print("°C");
    }

    // 第二行：溼度 (y = 33)
    u8g2.setCursor(52, 33);
    if (isnan(currentHum)) {
      u8g2.print("H: --.-%");
    } else {
      u8g2.print("H:");
      u8g2.print(currentHum, 1);
      u8g2.print("%");
    }

    // 第三行：CO2 數值 (y = 46)
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(52, 46);
    if (currentCO2PPM == -1) {
      u8g2.print("CO2:<400ppm");
    } else {
      u8g2.print("CO2:");
      u8g2.print(currentCO2PPM);
      u8g2.print("ppm");
    }

    // 3. 分隔線 (y = 50)
    u8g2.drawHLine(0, 50, 128);

    // 4. 底部區域 (y = 62): 顯示 DHCP 取得的 IP 位址
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.setCursor(0, 62);
    if (WiFi.status() == WL_CONNECTED) {
      u8g2.print("IP:");
      u8g2.print(WiFi.localIP().toString());
    } else {
      u8g2.print("WiFi Disconnected");
    }
  } while (u8g2.nextPage());
}

// 格式化系統運行時間 (Uptime)
String getUptimeString()
{
  unsigned long totalSeconds = millis() / 1000;
  unsigned long days = totalSeconds / 86400;
  unsigned long hours = (totalSeconds % 86400) / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;

  char buf[64];
  snprintf(buf, sizeof(buf), "%lu 天 %02lu 小時 %02lu 分 %02lu 秒", days, hours, minutes, seconds);
  return String(buf);
}

// 根據圖示代碼回傳對應 Emoji
const char* getSymbolEmoji(uint8_t symbol)
{
  switch (symbol) {
    case SUN:       return "☀️";
    case SUN_CLOUD: return "⛅";
    case CLOUD:     return "☁️";
    case RAIN:      return "🌧️";
    case THUNDER:   return "⛈️";
    default:        return "☁️";
  }
}

// AJAX JSON 資料端點 (/data)：供前端無刷新即時取得感測器數值與系統狀態
void handleData()
{
  const char *webStatus = "";
  uint8_t symbol = selectWeatherSymbol(currentTemp, currentHum, NULL, &webStatus);
  const char *iconEmoji = getSymbolEmoji(symbol);

  String tempStr = isnan(currentTemp) ? "--.-" : String(currentTemp, 1);
  String humStr  = isnan(currentHum)  ? "--.-" : String(currentHum, 1);
  String co2DisplayStr = (currentCO2PPM == -1) ? "< 400" : String(currentCO2PPM);
  int co2ChartVal = (currentCO2PPM == -1) ? 400 : currentCO2PPM;

  String co2LevelTitle, co2WarningMsg, co2BadgeColor, co2BgColor;
  getCO2Warning(currentCO2PPM, co2LevelTitle, co2WarningMsg, co2BadgeColor, co2BgColor);

  String json = "{";
  json += "\"tempValid\":" + String(isnan(currentTemp) ? "false" : "true") + ",";
  json += "\"temp\":" + String(isnan(currentTemp) ? 0.0 : currentTemp, 1) + ",";
  json += "\"tempStr\":\"" + tempStr + "\",";
  json += "\"humValid\":" + String(isnan(currentHum) ? "false" : "true") + ",";
  json += "\"hum\":" + String(isnan(currentHum) ? 0.0 : currentHum, 1) + ",";
  json += "\"humStr\":\"" + humStr + "\",";
  json += "\"iconEmoji\":\"" + String(iconEmoji) + "\",";
  json += "\"webStatus\":\"" + String(webStatus) + "\",";
  json += "\"co2ppm\":" + String(currentCO2PPM) + ",";
  json += "\"co2ChartVal\":" + String(co2ChartVal) + ",";
  json += "\"co2Display\":\"" + co2DisplayStr + "\",";
  json += "\"co2Volts\":" + String(currentCO2Volts, 3) + ",";
  json += "\"co2VoltsStr\":\"" + String(currentCO2Volts, 3) + "\",";
  json += "\"co2LevelTitle\":\"" + co2LevelTitle + "\",";
  json += "\"co2WarningMsg\":\"" + co2WarningMsg + "\",";
  json += "\"co2BadgeColor\":\"" + co2BadgeColor + "\",";
  json += "\"co2BgColor\":\"" + co2BgColor + "\",";
  json += "\"uptime\":\"" + getUptimeString() + "\",";
  json += "\"rssi\":\"" + String(WiFi.RSSI()) + " dBm\",";
  json += "\"freeHeap\":\"" + String(ESP.getFreeHeap() / 1024.0, 1) + " KB / " + String(ESP.getHeapSize() / 1024.0, 1) + " KB\",";
  json += "\"minFreeHeap\":\"" + String(ESP.getMinFreeHeap() / 1024.0, 1) + " KB\"";
  json += "}";

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "application/json; charset=utf-8", json);
}

// WebServer 主頁面處理函式 (AJAX 無刷新即時面板 + HTML5 Canvas 動態折線圖)
void handleRoot()
{
  const char *webStatus = "";
  uint8_t symbol = selectWeatherSymbol(currentTemp, currentHum, NULL, &webStatus);
  const char *iconEmoji = getSymbolEmoji(symbol);

  String tempStr = isnan(currentTemp) ? "--.-" : String(currentTemp, 1);
  String humStr  = isnan(currentHum)  ? "--.-" : String(currentHum, 1);
  String co2DisplayStr = (currentCO2PPM == -1) ? "&lt; 400" : String(currentCO2PPM);

  String co2LevelTitle, co2WarningMsg, co2BadgeColor, co2BgColor;
  getCO2Warning(currentCO2PPM, co2LevelTitle, co2WarningMsg, co2BadgeColor, co2BgColor);

  String html = "<!DOCTYPE html><html lang='zh-TW'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>NodeMCU-32S 環境監測與即時圖表面板</title>";
  html += "<style>";
  html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f0f4f8; color: #1e293b; margin: 0; padding: 20px; }";
  html += ".container { max-width: 860px; margin: 0 auto; }";
  html += "h1 { text-align: center; color: #0f172a; margin-bottom: 10px; font-size: 1.6rem; }";
  html += ".status-bar { display: flex; justify-content: center; align-items: center; gap: 12px; flex-wrap: wrap; margin-bottom: 18px; font-size: 0.88rem; }";
  html += ".live-pill { display: inline-flex; align-items: center; gap: 6px; background: #dcfce7; color: #15803d; padding: 4px 12px; border-radius: 999px; font-weight: 600; transition: all 0.3s; }";
  html += ".live-dot { width: 8px; height: 8px; background: #16a34a; border-radius: 50%; animation: blink 1.5s infinite; }";
  html += ".change-pill { display: inline-flex; align-items: center; gap: 5px; background: #e0f2fe; color: #0369a1; padding: 4px 12px; border-radius: 999px; font-weight: 600; transition: all 0.3s; }";
  html += "@keyframes blink { 0%, 100% { opacity: 1; } 50% { opacity: 0.3; } }";
  html += "@keyframes valPulse { 0% { transform: scale(1); background-color: #fef08a; } 50% { transform: scale(1.03); background-color: #fde047; } 100% { transform: scale(1); background-color: #f8fafc; } }";
  html += ".pulse-change { animation: valPulse 0.7s ease-out; }";
  html += ".card { background: #ffffff; border-radius: 12px; padding: 18px 22px; margin-bottom: 18px; box-shadow: 0 2px 8px rgba(0,0,0,0.07); }";
  html += ".card h2 { margin-top: 0; font-size: 1.15rem; color: #0284c7; border-bottom: 2px solid #e2e8f0; padding-bottom: 8px; display: flex; justify-content: space-between; align-items: center; }";
  html += ".env-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 15px; text-align: center; margin-top: 12px; }";
  html += ".env-box { background: #f8fafc; border-radius: 10px; padding: 15px; border: 1px solid #e2e8f0; transition: background-color 0.3s, transform 0.2s; }";
  html += ".env-val { font-size: 2rem; font-weight: 700; color: #0f172a; margin: 6px 0; }";
  html += ".env-label { font-size: 0.9rem; color: #64748b; }";
  html += ".co2-alert { margin-top: 15px; padding: 14px 16px; border-radius: 10px; border-left: 6px solid " + co2BadgeColor + "; background: " + co2BgColor + "; transition: all 0.3s; }";
  html += ".co2-badge { display: inline-block; padding: 3px 10px; border-radius: 999px; color: #fff; font-size: 0.85rem; font-weight: 600; background: " + co2BadgeColor + "; margin-bottom: 6px; transition: background 0.3s; }";
  html += ".co2-msg { font-size: 1rem; font-weight: 600; color: #1e293b; line-height: 1.5; }";
  html += ".chart-wrap { position: relative; width: 100%; margin-top: 10px; }";
  html += "canvas { width: 100%; height: 230px; display: block; background: #f8fafc; border-radius: 8px; border: 1px solid #e2e8f0; }";
  html += ".chart-legend { display: flex; justify-content: center; gap: 18px; font-size: 0.85rem; color: #475569; margin-top: 8px; }";
  html += ".legend-item { display: flex; align-items: center; gap: 6px; font-weight: 600; }";
  html += ".legend-color { width: 12px; height: 12px; border-radius: 3px; display: inline-block; }";
  html += "table { width: 100%; border-collapse: collapse; font-size: 0.95rem; }";
  html += "th, td { padding: 9px 6px; text-align: left; border-bottom: 1px solid #f1f5f9; }";
  html += "th { color: #475569; width: 42%; font-weight: 600; }";
  html += "td { color: #0f172a; font-family: monospace; font-size: 0.95rem; }";
  html += ".footer { text-align: center; font-size: 0.8rem; color: #94a3b8; margin-top: 10px; }";
  html += "</style></head><body><div class='container'>";

  html += "<h1>NodeMCU-32S 溫溼度、CO2 與即時圖表面板</h1>";
  html += "<div class='status-bar'>";
  html += "<div class='live-pill' id='connPill'><span class='live-dot' id='connDot'></span><span id='connText'>AJAX 即時連線中 (無刷新)</span></div>";
  html += "<div class='change-pill' id='changeNotice'>⏱️ 等待感測器數據更新...</div>";
  html += "</div>";

  // 1. 溫溼度感測卡片
  html += "<div class='card'>";
  html += "<h2><span>🌡️ 即時溫溼度資訊 (DHT22)</span><span style='font-size:0.8rem;color:#64748b;font-weight:normal;' id='lastUpdateTime'>--:--:--</span></h2>";
  html += "<div class='env-grid'>";
  html += "<div class='env-box' id='boxTemp'><div class='env-label'>目前溫度</div><div class='env-val'><span id='valTemp'>" + tempStr + "</span> °C</div></div>";
  html += "<div class='env-box' id='boxHum'><div class='env-label'>目前溼度</div><div class='env-val'><span id='valHum'>" + humStr + "</span> %</div></div>";
  html += "<div class='env-box' id='boxStatus'><div class='env-label'>溫溼度評估</div><div class='env-val' id='valEmoji'>" + String(iconEmoji) + "</div><div id='valWebStatus'>" + String(webStatus) + "</div></div>";
  html += "</div>";
  html += "<div class='chart-wrap'><canvas id='tempHumChart'></canvas></div>";
  html += "<div class='chart-legend'>";
  html += "<div class='legend-item'><span class='legend-color' style='background:#ef4444;'></span>溫度 (°C)</div>";
  html += "<div class='legend-item'><span class='legend-color' style='background:#0284c7;'></span>溼度 (%)</div>";
  html += "</div></div>";

  // 2. SEN0159 CO2 二氧化碳感測與警語卡片
  html += "<div class='card'>";
  html += "<h2><span>🫧 二氧化碳濃度監測 (DFRobot SEN0159 / MG-811)</span></h2>";
  html += "<div class='env-grid'>";
  html += "<div class='env-box' id='boxCO2'><div class='env-label'>CO2 濃度</div><div class='env-val' id='valCO2Wrap' style='color:" + co2BadgeColor + ";'><span id='valCO2'>" + co2DisplayStr + "</span> <span style='font-size:1.1rem;'>ppm</span></div></div>";
  html += "<div class='env-box' id='boxVolts'><div class='env-label'>感測器輸出電壓 (GPIO 32)</div><div class='env-val'><span id='valVolts'>" + String(currentCO2Volts, 3) + "</span> <span style='font-size:1.1rem;'>V</span></div></div>";
  html += "</div>";
  html += "<div class='co2-alert' id='co2AlertBox'>";
  html += "<div class='co2-badge' id='co2Badge'>" + co2LevelTitle + "</div>";
  html += "<div class='co2-msg' id='co2Msg'>" + co2WarningMsg + "</div>";
  html += "</div>";
  html += "<div class='chart-wrap' style='margin-top:14px;'><canvas id='co2Chart'></canvas></div>";
  html += "<div class='chart-legend'>";
  html += "<div class='legend-item'><span class='legend-color' style='background:#16a34a;'></span>CO2 濃度 (ppm, 左軸)</div>";
  html += "<div class='legend-item'><span class='legend-color' style='background:#9333ea;'></span>輸出電壓 (V, 右軸)</div>";
  html += "</div></div>";

  // 3. 系統狀態卡片
  html += "<div class='card'>";
  html += "<h2>⚙️ 系統狀態 (System Status)</h2>";
  html += "<table>";
  html += "<tr><th>系統運行時間 (Uptime)</th><td id='sysUptime'>" + getUptimeString() + "</td></tr>";
  html += "<tr><th>WiFi SSID</th><td>" + WiFi.SSID() + "</td></tr>";
  html += "<tr><th>DHCP IP 位址</th><td>" + WiFi.localIP().toString() + "</td></tr>";
  html += "<tr><th>子網路遮罩 (Subnet Mask)</th><td>" + WiFi.subnetMask().toString() + "</td></tr>";
  html += "<tr><th>預設閘道 (Gateway)</th><td>" + WiFi.gatewayIP().toString() + "</td></tr>";
  html += "<tr><th>DNS 伺服器</th><td>" + WiFi.dnsIP().toString() + "</td></tr>";
  html += "<tr><th>WiFi 訊號強度 (RSSI)</th><td id='sysRSSI'>" + String(WiFi.RSSI()) + " dBm</td></tr>";
  html += "<tr><th>MAC 位址</th><td>" + WiFi.macAddress() + "</td></tr>";
  html += "<tr><th>可用記憶體 (Free Heap)</th><td id='sysFreeHeap'>" + String(ESP.getFreeHeap() / 1024.0, 1) + " KB / " + String(ESP.getHeapSize() / 1024.0, 1) + " KB</td></tr>";
  html += "<tr><th>最小曾剩餘記憶體 (Min Free Heap)</th><td id='sysMinHeap'>" + String(ESP.getMinFreeHeap() / 1024.0, 1) + " KB</td></tr>";
  html += "</table></div>";

  // 4. NodeMCU-32S 硬體資訊卡片
  html += "<div class='card'>";
  html += "<h2>🔧 NodeMCU-32S 硬體資訊 (Hardware Info)</h2>";
  html += "<table>";
  html += "<tr><th>晶片型號 (Chip Model)</th><td>" + String(ESP.getChipModel()) + " (Rev " + String(ESP.getChipRevision()) + ")</td></tr>";
  html += "<tr><th>CPU 核心數 (Cores)</th><td>" + String(ESP.getChipCores()) + " Cores</td></tr>";
  html += "<tr><th>CPU 時脈頻率</th><td>" + String(ESP.getCpuFreqMHz()) + " MHz</td></tr>";
  html += "<tr><th>Flash 記憶體容量</th><td>" + String(ESP.getFlashChipSize() / (1024 * 1024)) + " MB (" + String(ESP.getFlashChipSize()) + " Bytes)</td></tr>";
  html += "<tr><th>Flash 運作頻率</th><td>" + String(ESP.getFlashChipSpeed() / 1000000) + " MHz</td></tr>";
  html += "<tr><th>程式碼大小 (Sketch Size)</th><td>" + String(ESP.getSketchSize() / 1024.0, 1) + " KB (剩餘可用: " + String(ESP.getFreeSketchSpace() / 1024.0, 1) + " KB)</td></tr>";
  html += "<tr><th>ESP-IDF SDK 版本</th><td>" + String(ESP.getSdkVersion()) + "</td></tr>";
  html += "<tr><th>感測器與顯示器腳位</th><td>DHT22: GPIO 16 | SEN0159 CO2: GPIO 32 | OLED I2C: SDA=21, SCL=22</td></tr>";
  html += "</table></div>";

  html += "<div class='footer'>AJAX 即時無刷新監測系統 (每 2 秒自動同步感測器數值與繪製動態圖表)</div>";

  // 前端 AJAX 與即時 Canvas 圖表繪製腳本 (零外部 CDN 依賴，支援無外網環境)
  html += "<script>";
  html += "const MAX_POINTS = 30;";
  html += "const history = { labels: [], temp: [], hum: [], co2: [], volts: [] };";
  html += "let prevData = { tempStr: null, humStr: null, co2Display: null, co2VoltsStr: null };";
  html += "function pulseBox(id) {";
  html += "  const el = document.getElementById(id);";
  html += "  if (!el) return;";
  html += "  el.classList.remove('pulse-change');";
  html += "  void el.offsetWidth;";
  html += "  el.classList.add('pulse-change');";
  html += "}";
  html += "function drawDualChart(canvasId, labels, series1, series2, cfg) {";
  html += "  const canvas = document.getElementById(canvasId);";
  html += "  if (!canvas) return;";
  html += "  const dpr = window.devicePixelRatio || 1;";
  html += "  const rect = canvas.getBoundingClientRect();";
  html += "  canvas.width = rect.width * dpr;";
  html += "  canvas.height = rect.height * dpr;";
  html += "  const ctx = canvas.getContext('2d');";
  html += "  ctx.scale(dpr, dpr);";
  html += "  const W = rect.width, H = rect.height;";
  html += "  const pad = { top: 22, right: 52, bottom: 28, left: 48 };";
  html += "  const plotW = W - pad.left - pad.right;";
  html += "  const plotH = H - pad.top - pad.bottom;";
  html += "  ctx.clearRect(0, 0, W, H);";
  html += "  const v1 = series1.filter(v => v !== null && !isNaN(v));";
  html += "  const v2 = series2.filter(v => v !== null && !isNaN(v));";
  html += "  let min1 = v1.length ? Math.min(...v1) : cfg.min1;";
  html += "  let max1 = v1.length ? Math.max(...v1) : cfg.max1;";
  html += "  if (max1 - min1 < cfg.span1) { const mid = (max1 + min1)/2; min1 = mid - cfg.span1/2; max1 = mid + cfg.span1/2; } else { min1 -= cfg.pad1; max1 += cfg.pad1; }";
  html += "  if (cfg.clampMin1 !== undefined && min1 < cfg.clampMin1) min1 = cfg.clampMin1;";
  html += "  let min2 = v2.length ? Math.min(...v2) : cfg.min2;";
  html += "  let max2 = v2.length ? Math.max(...v2) : cfg.max2;";
  html += "  if (max2 - min2 < cfg.span2) { const mid = (max2 + min2)/2; min2 = mid - cfg.span2/2; max2 = mid + cfg.span2/2; } else { min2 -= cfg.pad2; max2 += cfg.pad2; }";
  html += "  if (cfg.clampMin2 !== undefined && min2 < cfg.clampMin2) min2 = cfg.clampMin2;";
  html += "  ctx.strokeStyle = '#e2e8f0'; ctx.lineWidth = 1; ctx.font = '11px monospace';";
  html += "  const steps = 4;";
  html += "  for (let i = 0; i <= steps; i++) {";
  html += "    const y = pad.top + (plotH * i / steps);";
  html += "    ctx.beginPath(); ctx.moveTo(pad.left, y); ctx.lineTo(pad.left + plotW, y); ctx.stroke();";
  html += "    const valL = max1 - (max1 - min1) * (i / steps);";
  html += "    const valR = max2 - (max2 - min2) * (i / steps);";
  html += "    ctx.fillStyle = cfg.color1; ctx.textAlign = 'right';";
  html += "    ctx.fillText(valL.toFixed(cfg.dec1) + cfg.unit1, pad.left - 5, y + 4);";
  html += "    ctx.fillStyle = cfg.color2; ctx.textAlign = 'left';";
  html += "    ctx.fillText(valR.toFixed(cfg.dec2) + cfg.unit2, pad.left + plotW + 5, y + 4);";
  html += "  }";
  html += "  const n = labels.length;";
  html += "  if (n === 0) return;";
  html += "  const xPos = (idx) => n === 1 ? pad.left + plotW / 2 : pad.left + ( idx / (MAX_POINTS - 1) ) * plotW;";
  html += "  ctx.fillStyle = '#64748b'; ctx.textAlign = 'center';";
  html += "  const labelStep = Math.max(1, Math.floor(n / 5));";
  html += "  for (let i = 0; i < n; i += labelStep) {";
  html += "    ctx.fillText(labels[i], xPos(i), H - 8);";
  html += "  }";
  html += "  function plotLine(arr, minV, maxV, color, dec) {";
  html += "    ctx.strokeStyle = color; ctx.lineWidth = 2.4; ctx.beginPath();";
  html += "    let started = false, lastX = 0, lastY = 0, lastVal = null;";
  html += "    for (let i = 0; i < arr.length; i++) {";
  html += "      if (arr[i] === null || isNaN(arr[i])) continue;";
  html += "      const x = xPos(i);";
  html += "      const y = pad.top + plotH - ((arr[i] - minV) / (maxV - minV)) * plotH;";
  html += "      if (!started) { ctx.moveTo(x, y); started = true; } else { ctx.lineTo(x, y); }";
  html += "      lastX = x; lastY = y; lastVal = arr[i];";
  html += "    }";
  html += "    ctx.stroke();";
  html += "    for (let i = 0; i < arr.length; i++) {";
  html += "      if (arr[i] === null || isNaN(arr[i])) continue;";
  html += "      const x = xPos(i);";
  html += "      const y = pad.top + plotH - ((arr[i] - minV) / (maxV - minV)) * plotH;";
  html += "      ctx.fillStyle = '#ffffff'; ctx.beginPath(); ctx.arc(x, y, 3.5, 0, Math.PI * 2); ctx.fill();";
  html += "      ctx.strokeStyle = color; ctx.lineWidth = 2; ctx.stroke();";
  html += "    }";
  html += "    if (lastVal !== null) {";
  html += "      ctx.fillStyle = color; ctx.font = 'bold 11px sans-serif'; ctx.textAlign = 'center';";
  html += "      ctx.fillText(lastVal.toFixed(dec), lastX, Math.max(12, lastY - 8));";
  html += "    }";
  html += "  }";
  html += "  plotLine(series1, min1, max1, cfg.color1, cfg.dec1);";
  html += "  plotLine(series2, min2, max2, cfg.color2, cfg.dec2);";
  html += "}";
  html += "function renderCharts() {";
  html += "  drawDualChart('tempHumChart', history.labels, history.temp, history.hum, {";
  html += "    min1: 15, max1: 35, span1: 6, pad1: 1.5, clampMin1: -10, dec1: 1, unit1: '°C', color1: '#ef4444',";
  html += "    min2: 30, max2: 90, span2: 15, pad2: 3, clampMin2: 0, dec2: 1, unit2: '%', color2: '#0284c7'";
  html += "  });";
  html += "  drawDualChart('co2Chart', history.labels, history.co2, history.volts, {";
  html += "    min1: 350, max1: 1200, span1: 200, pad1: 50, clampMin1: 300, dec1: 0, unit1: '', color1: '#16a34a',";
  html += "    min2: 0.5, max2: 3.0, span2: 0.4, pad2: 0.05, clampMin2: 0, dec2: 3, unit2: 'V', color2: '#9333ea'";
  html += "  });";
  html += "}";
  html += "async function fetchSensorData() {";
  html += "  try {";
  html += "    const res = await fetch('/data', { cache: 'no-store' });";
  html += "    if (!res.ok) throw new Error('HTTP ' + res.status);";
  html += "    const d = await res.json();";
  html += "    const now = new Date();";
  html += "    const timeStr = now.toTimeString().split(' ')[0];";
  html += "    document.getElementById('lastUpdateTime').textContent = '最後更新: ' + timeStr;";
  html += "    document.getElementById('connPill').style.background = '#dcfce7';";
  html += "    document.getElementById('connPill').style.color = '#15803d';";
  html += "    document.getElementById('connDot').style.background = '#16a34a';";
  html += "    document.getElementById('connText').textContent = 'AJAX 即時連線中 (無刷新)';";
  html += "    let changedItems = [];";
  html += "    if (prevData.tempStr !== null && prevData.tempStr !== d.tempStr) { pulseBox('boxTemp'); changedItems.push('溫度'); }";
  html += "    if (prevData.humStr !== null && prevData.humStr !== d.humStr) { pulseBox('boxHum'); changedItems.push('溼度'); }";
  html += "    if (prevData.co2Display !== null && prevData.co2Display !== d.co2Display) { pulseBox('boxCO2'); changedItems.push('CO2濃度'); }";
  html += "    if (prevData.co2VoltsStr !== null && prevData.co2VoltsStr !== d.co2VoltsStr) { pulseBox('boxVolts'); changedItems.push('CO2電壓'); }";
  html += "    prevData = { tempStr: d.tempStr, humStr: d.humStr, co2Display: d.co2Display, co2VoltsStr: d.co2VoltsStr };";
  html += "    const noticeEl = document.getElementById('changeNotice');";
  html += "    if (changedItems.length > 0) {";
  html += "      noticeEl.textContent = '⚡ 感應數值變化 (' + changedItems.join('、') + ') @ ' + timeStr;";
  html += "      noticeEl.style.background = '#fef08a'; noticeEl.style.color = '#854d0e';";
  html += "      setTimeout(() => { noticeEl.style.background = '#e0f2fe'; noticeEl.style.color = '#0369a1'; }, 900);";
  html += "    } else if (history.labels.length <= 1) {";
  html += "      noticeEl.textContent = '✅ 已同步初始感測器數據 @ ' + timeStr;";
  html += "    }";
  html += "    document.getElementById('valTemp').textContent = d.tempStr;";
  html += "    document.getElementById('valHum').textContent = d.humStr;";
  html += "    document.getElementById('valEmoji').textContent = d.iconEmoji;";
  html += "    document.getElementById('valWebStatus').textContent = d.webStatus;";
  html += "    document.getElementById('valCO2').textContent = d.co2Display;";
  html += "    document.getElementById('valCO2Wrap').style.color = d.co2BadgeColor;";
  html += "    document.getElementById('valVolts').textContent = d.co2VoltsStr;";
  html += "    const alertBox = document.getElementById('co2AlertBox');";
  html += "    alertBox.style.borderLeftColor = d.co2BadgeColor;";
  html += "    alertBox.style.background = d.co2BgColor;";
  html += "    const badge = document.getElementById('co2Badge');";
  html += "    badge.style.background = d.co2BadgeColor;";
  html += "    badge.textContent = d.co2LevelTitle;";
  html += "    document.getElementById('co2Msg').textContent = d.co2WarningMsg;";
  html += "    document.getElementById('sysUptime').textContent = d.uptime;";
  html += "    document.getElementById('sysRSSI').textContent = d.rssi;";
  html += "    document.getElementById('sysFreeHeap').textContent = d.freeHeap;";
  html += "    document.getElementById('sysMinHeap').textContent = d.minFreeHeap;";
  html += "    history.labels.push(timeStr.slice(3));";
  html += "    history.temp.push(d.tempValid ? d.temp : null);";
  html += "    history.hum.push(d.humValid ? d.hum : null);";
  html += "    history.co2.push(d.co2ChartVal);";
  html += "    history.volts.push(d.co2Volts);";
  html += "    if (history.labels.length > MAX_POINTS) {";
  html += "      history.labels.shift(); history.temp.shift(); history.hum.shift(); history.co2.shift(); history.volts.shift();";
  html += "    }";
  html += "    renderCharts();";
  html += "  } catch (err) {";
  html += "    document.getElementById('connPill').style.background = '#fee2e2';";
  html += "    document.getElementById('connPill').style.color = '#b91c1c';";
  html += "    document.getElementById('connDot').style.background = '#dc2626';";
  html += "    document.getElementById('connText').textContent = '連線中斷，重試中...';";
  html += "  }";
  html += "}";
  html += "window.addEventListener('resize', renderCharts);";
  html += "fetchSensorData();";
  html += "setInterval(fetchSensorData, 2000);";
  html += "</script>";
  html += "</div></body></html>";

  server.send(200, "text/html; charset=utf-8", html);
}

// 讀取 DHT 溫溼度與 SEN0159 CO2 感測器 (非阻塞式定時更新)
void updateSensorData()
{
  unsigned long now = millis();
  if (now - lastReadTime >= delayMS || lastReadTime == 0) {
    lastReadTime = now;

    sensors_event_t event;

    // 1. 讀取溫度
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
      currentTemp = NAN;
    } else {
      currentTemp = event.temperature;
      Serial.print(F("Temperature: "));
      Serial.print(currentTemp);
      Serial.println(F("°C"));
    }

    // 2. 讀取溼度
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
      currentHum = NAN;
    } else {
      currentHum = event.relative_humidity;
      Serial.print(F("Humidity: "));
      Serial.print(currentHum);
      Serial.println(F("%"));
    }

    // 3. 讀取 SEN0159 CO2 濃度 (GPIO 32)
    currentCO2Volts = MGRead(MG_PIN);
    currentCO2PPM = MGGetPercentage(currentCO2Volts, CO2Curve);
    Serial.print(F("SEN0159 Voltage: "));
    Serial.print(currentCO2Volts, 3);
    Serial.print(F(" V | CO2: "));
    if (currentCO2PPM == -1) {
      Serial.println(F("< 400 ppm"));
    } else {
      Serial.print(currentCO2PPM);
      Serial.println(F(" ppm"));
    }

    // 4. 每次感測器更新後同步刷新 OLED 畫面
    drawEnvDashboard();
  }
}

void setup(void)
{
  Serial.begin(115200);

  // 設定 GPIO 32 類比輸入腳位
  pinMode(MG_PIN, INPUT);

  // 1. 初始化 U8g2 SSD1306 OLED
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

  // 3. 連線至 WiFi (DHCP 自動取得 IP)
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print(F("Connecting to WiFi SSID: "));
  Serial.println(WIFI_SSID);

  int step = 0;
  while (WiFi.status() != WL_CONNECTED && step < 40) { // 最多等待約 20 秒
    drawWiFiConnecting(step);
    delay(500);
    Serial.print(".");
    step++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("WiFi Connected!"));
    Serial.print(F("DHCP IP Address: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("WiFi Connection Timeout (will keep retrying in background)."));
  }

  // 4. 啟動 WebServer (支援主頁與 /data AJAX JSON 端點)
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println(F("HTTP WebServer started on port 80."));

  // 5. 初次讀取感測器並繪製主畫面
  updateSensorData();
}

void loop(void)
{
  // 處理瀏覽器 HTTP 請求
  server.handleClient();

  // 定時更新感測器數值與 OLED 畫面
  updateSensorData();
}
