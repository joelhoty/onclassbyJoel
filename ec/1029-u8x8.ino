// DHT11 溫濕度感測器 + SSD1306 128x64 OLED (U8x8 API) — 動態效果版
//
// 畫面配置 (U8x8 = 16 欄 x 8 列，每格 8x8 像素)
//   第 0 列    : [ TEMPERATURE  ⟳]  反白標題 + 右上角旋轉指示器
//   第 1 列    : ██████░░░░░░░░░░   溫度長條圖 (0~50°C)
//   第 2~3 列  :   25.0 °C      ↑   2x2 放大字 + 趨勢箭頭
//   第 4 列    : [   HUMIDITY   ⟳]  反白標題 + 旋轉指示器
//   第 5 列    : ████████░░░░░░░░   濕度長條圖 (0~100%)
//   第 6~7 列  :   60.0 %       ↓   2x2 放大字 + 趨勢箭頭
//
// 動態效果:
//   1. 數字滾動：數值從舊值逐步「跑」到新值 (開機時從 0 往上數)
//   2. 長條圖：跟著數字一起平滑伸縮
//   3. 趨勢箭頭：上升 ↑ / 下降 ↓，數字滾動期間會閃爍
//   4. 旋轉指示器：標題右側 | / - \ 持續轉動，表示程式運作中
//
// 接線 (Arduino UNO):
//   OLED SDA -> A4, SCL -> A5, VCC -> 5V, GND -> GND
//   DHT11 DATA -> D8
//
// 需要的函式庫:
// - U8g2 (含 U8x8): https://github.com/olikraus/u8g2
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include <Arduino.h>
#include <U8x8lib.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 8          // DHT 感測器資料腳接 D8
#define DHTTYPE DHT11     // 感測器型號 DHT11
//#define DHTTYPE DHT22   // DHT 22 (AM2302)

#define FRAME_MS   40     // 動畫每格間隔 (毫秒)，約 25 FPS
#define TEMP_MAX   50.0   // 溫度長條圖滿格值 (DHT11 量測上限 50°C)
#define HUMI_MAX   100.0  // 濕度長條圖滿格值

DHT_Unified dht(DHTPIN, DHTTYPE);

// SSD1306 128x64 OLED，硬體 I2C (A4/A5)
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
// 若畫面異常可改用軟體 I2C:
//U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

// ---- 自訂圖塊 (8x8，每個 byte 為一直行，LSB 在上方) ----
uint8_t degreeTile[8] = {0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00, 0x00}; // °
uint8_t upTile[8]     = {0x00, 0x08, 0x0C, 0xFE, 0x0C, 0x08, 0x00, 0x00}; // ↑
uint8_t downTile[8]   = {0x00, 0x10, 0x30, 0x7F, 0x30, 0x10, 0x00, 0x00}; // ↓
uint8_t blankTile[8]  = {0, 0, 0, 0, 0, 0, 0, 0};

// ---- 狀態變數 ----
uint32_t delayMS;                 // 感測器讀取間隔
unsigned long lastRead = 0;       // 上次讀取時間
unsigned long lastFrame = 0;      // 上次動畫更新時間
uint16_t frameCount = 0;          // 動畫格數計數

float tgtT = 0, tgtH = 0;         // 目標值 (感測器最新讀數)
float dspT = 0, dspH = 0;         // 目前畫面顯示值 (動畫中逐步接近目標)
bool  okT = false, okH = false;   // 讀數是否有效
bool  firstRead = true;
int8_t trendT = 0, trendH = 0;    // 趨勢：1 上升 / -1 下降 / 0 持平

// 用來避免重複繪製 (減少 I2C 傳輸、畫面不閃)
int  shownT = -32768, shownH = -32768;  // 已顯示數值 (x10 整數)
int  barPxT = -1, barPxH = -1;          // 已顯示長條像素
int8_t arrowStT = -2, arrowStH = -2;    // 已顯示箭頭狀態

// 讓 cur 平滑接近 tgt：差距大時走快，差距小時至少走 0.1
float approach(float cur, float tgt) {
  float d = tgt - cur;
  if (fabs(d) < 0.05) return tgt;
  float step = d / 6.0;
  if (fabs(step) < 0.1) step = (d > 0) ? 0.1 : -0.1;
  return cur + step;
}

// 繪製 2x2 大數字 (只有數值改變時才重畫)
void drawValue(uint8_t row, float val, bool ok, int &shown) {
  int v10 = ok ? (int)round(val * 10) : -32767;  // -32767 代表錯誤
  if (v10 == shown) return;
  shown = v10;
  if (!ok) {
    u8x8.draw2x2String(2, row, "----");
  } else {
    char buf[8];
    dtostrf(val, 4, 1, buf);   // 固定 4 字寬，例如 "25.0"
    u8x8.draw2x2String(2, row, buf);
  }
}

// 繪製長條圖 (整列 16 格 = 128 像素)
void drawBar(uint8_t row, float val, float maxV, bool ok, int &lastPx) {
  int px = ok ? (int)(constrain(val / maxV, 0.0, 1.0) * 128) : 0;
  if (px == lastPx) return;
  lastPx = px;

  uint8_t buf[128];
  for (int i = 0; i < 128; i++) {
    if (i == 0 || i == 127)  buf[i] = 0x7E;  // 左右邊框
    else if (i < px)         buf[i] = 0x7E;  // 已填滿：實心
    else                     buf[i] = 0x42;  // 未填滿：只有上下框線
  }
  u8x8.drawTile(0, row, 16, buf);
}

// 繪製趨勢箭頭；動畫進行中 (dsp != tgt) 會閃爍
void drawArrow(uint8_t row, int8_t trend, bool animating, int8_t &lastSt) {
  int8_t st = trend;
  if (animating && ((frameCount / 5) % 2)) st = 0;   // 閃爍：每 5 格切換
  if (st == lastSt) return;
  lastSt = st;
  if (st > 0)      u8x8.drawTile(15, row, 1, upTile);
  else if (st < 0) u8x8.drawTile(15, row, 1, downTile);
  else             u8x8.drawTile(15, row, 1, blankTile);
}

// 標題右側旋轉指示器 | / - \ .
void drawSpinner() {
  static const char spin[] = "|/-\\";
  if (frameCount % 4) return;                 // 每 4 格轉一次
  char c[2] = { spin[(frameCount / 4) % 4], 0 };
  u8x8.setInverseFont(1);
  u8x8.drawString(15, 0, c);
  u8x8.drawString(15, 4, c);
  u8x8.setInverseFont(0);
}

// 畫出固定的標題與單位符號
void drawLayout() {
  u8x8.clear();
  u8x8.setFont(u8x8_font_chroma48medium8_r);

  u8x8.setInverseFont(1);
  u8x8.drawString(0, 0, "  TEMPERATURE   ");
  u8x8.drawString(0, 4, "    HUMIDITY    ");
  u8x8.setInverseFont(0);

  u8x8.drawTile(11, 2, 1, degreeTile);   // °
  u8x8.draw2x2String(12, 2, "C");
  u8x8.draw2x2String(12, 6, "%");
}

// 讀取 DHT11，更新目標值與趨勢
void readSensor() {
  sensors_event_t event;

  dht.temperature().getEvent(&event);
  okT = !isnan(event.temperature);
  if (okT) {
    float t = event.temperature;
    if (!firstRead) trendT = (t > tgtT + 0.05) ? 1 : (t < tgtT - 0.05) ? -1 : 0;
    tgtT = t;
    Serial.print(F("Temperature: "));
    Serial.print(t);
    Serial.println(F(" C"));
  } else {
    Serial.println(F("Error reading temperature!"));
  }

  dht.humidity().getEvent(&event);
  okH = !isnan(event.relative_humidity);
  if (okH) {
    float h = event.relative_humidity;
    if (!firstRead) trendH = (h > tgtH + 0.05) ? 1 : (h < tgtH - 0.05) ? -1 : 0;
    tgtH = h;
    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.println(F("%"));
  } else {
    Serial.println(F("Error reading humidity!"));
  }

  if (okT && okH) firstRead = false;
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 3, " DHT11 Starting ");

  // 依感測器規格設定讀取間隔
  sensor_t sensor;
  dht.humidity().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
  if (delayMS < 2000) delayMS = 2000;   // DHT11 建議至少間隔 1~2 秒

  delay(1000);
  drawLayout();
  lastRead = millis() - delayMS;        // 讓 loop 一開始就立即讀取
}

void loop() {
  unsigned long now = millis();

  // ---- 1. 定時讀取感測器 (非阻塞) ----
  if (now - lastRead >= delayMS) {
    lastRead = now;
    readSensor();
  }

  // ---- 2. 動畫更新 ----
  if (now - lastFrame >= FRAME_MS) {
    lastFrame = now;
    frameCount++;

    // 顯示值逐步接近目標值
    if (okT) dspT = approach(dspT, tgtT);
    if (okH) dspH = approach(dspH, tgtH);

    u8x8.setFont(u8x8_font_chroma48medium8_r);

    // 溫度區
    drawBar(1, dspT, TEMP_MAX, okT, barPxT);
    drawValue(2, dspT, okT, shownT);
    drawArrow(2, trendT, dspT != tgtT, arrowStT);

    // 濕度區
    drawBar(5, dspH, HUMI_MAX, okH, barPxH);
    drawValue(6, dspH, okH, shownH);
    drawArrow(6, trendH, dspH != tgtH, arrowStH);

    drawSpinner();
  }
}
