// ============================================================================
// 專案名稱: Arduino Uno + I2C LCD 1602 + DHT11 溫濕度實體螢幕即時監控系統
// 檔案路徑: D:\CLASS_NOTE\ec\arduino_i2c_lcd1602_dht11_unified.ino
// 參考草稿: sketch_oct20b.ino
// 建立日期: 2026-10-20
// 顯示規格:
//   第一行 (Row 0): TEMP: xx.x°C
//   第二行 (Row 1): HUMD: xx.x %
// 硬體接線:
//   LCD 1602 (I2C): GND->GND, VCC->5V, SDA->A4, SCL->A5 (位址 0x27)
//   DHT11:          VCC->5V, DATA->Pin 7, GND->GND
// ============================================================================

#include <Wire.h>                 // I2C 雙線通訊基礎函式庫
#include <LiquidCrystal_I2C.h>    // I2C LCD 1602 驅動函式庫
#include <Adafruit_Sensor.h>      // Adafruit 統一感測器介面
#include <DHT.h>                  // DHT 感測器基礎函式庫
#include <DHT_U.h>                // DHT Unified 感測器驅動

// --- 1. 設定 DHT 感測器 ---
#define DHTPIN 7                  // DHT 訊號線接在數位腳位 7
#define DHTTYPE DHT11             // 感測器型號指定為 DHT11
DHT_Unified dht(DHTPIN, DHTTYPE);

// --- 2. 設定 LCD 1602 螢幕 ---
// 預設 I2C 位址 0x27，16 個字元寬度、2 行顯示
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 自定義溫度計圖示 (5x8 點陣，註冊至 CGRAM 0)
byte thermometerIcon[8] = {
  0b00100,
  0b01010,
  0b01010,
  0b01110,
  0b01110,
  0b11111,
  0b11111,
  0b01110
};

// 自定義水滴圖示 (5x8 點陣，註冊至 CGRAM 1)
byte waterDropIcon[8] = {
  0b00100,
  0b00100,
  0b01010,
  0b01010,
  0b10001,
  0b10001,
  0b01110,
  0b00000
};

uint32_t delayMS;

void setup() {
  // 初始化序列埠除錯視窗，鮑率 9600
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("========================================"));
  Serial.println(F("Arduino LCD1602 + DHT11 溫濕度整合系統啟動"));
  Serial.println(F("========================================"));

  // --- 初始化 LCD 模組 ---
  lcd.init();
  lcd.init();                     // 雙重初始化，有效清除上電雜訊與垂直條紋
  lcd.backlight();                // 開啟液晶背光
  lcd.clear();                    // 清空螢幕

  // 註冊自定義圖示
  lcd.createChar(0, thermometerIcon);
  lcd.createChar(1, waterDropIcon);

  // 開機迎賓畫面
  lcd.setCursor(0, 0);
  lcd.print("System Starting");
  lcd.setCursor(0, 1);
  lcd.print("DHT11 + LCD1602 ");
  delay(1500);
  lcd.clear();

  // --- 初始化 DHT11 感測器 ---
  dht.begin();
  
  // 取得感測器硬體規格以決定採樣間隔
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
  
  // DHT11 物理限制：採樣週期至少 1.5 秒 ~ 2 秒
  if (delayMS < 1500) {
    delayMS = 1500;
  }

  Serial.print(F("採樣週期已設定為: "));
  Serial.print(delayMS);
  Serial.println(F(" ms"));
}

void loop() {
  // 保持安全物理採樣延遲
  delay(delayMS);

  sensors_event_t event;

  // ==========================================
  // 1. 讀取並顯示溫度 (第一行 Row 0)
  // ==========================================
  dht.temperature().getEvent(&event);
  lcd.setCursor(0, 0);            // 游標定位至 (第 0 欄, 第 0 列)
  
  if (isnan(event.temperature)) {
    Serial.println(F("✖ 溫度讀取失敗 (NaN)！"));
    lcd.print(F("TEMP: Error     "));
  } else {
    // 輸出至序列埠
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature, 1);
    Serial.println(F(" °C"));

    // 輸出至 LCD 螢幕第一行
    lcd.write(0);                 // 印出自訂溫度計圖示
    lcd.print(F(" TEMP: "));
    lcd.print(event.temperature, 1); // 顯示數值至小數點後 1 位
    lcd.write(223);               // HD44780 內建「°」度數符號 (ASCII 223 / 0xDF)
    lcd.print(F("C   "));          // 補空格確保覆蓋前一次舊字元，防止畫面狂閃
  }

  // ==========================================
  // 2. 讀取並顯示濕度 (第二行 Row 1)
  // ==========================================
  dht.humidity().getEvent(&event);
  lcd.setCursor(0, 1);            // 游標定位至 (第 0 欄, 第 1 列)
  
  if (isnan(event.relative_humidity)) {
    Serial.println(F("✖ 濕度讀取失敗 (NaN)！"));
    lcd.print(F("HUMD: Error     "));
  } else {
    // 輸出至序列埠
    Serial.print(F("Humidity:    "));
    Serial.print(event.relative_humidity, 1);
    Serial.println(F(" %"));

    // 輸出至 LCD 螢幕第二行
    lcd.write(1);                 // 印出自訂水滴圖示
    lcd.print(F(" HUMD: "));
    lcd.print(event.relative_humidity, 1); // 顯示數值至小數點後 1 位
    lcd.print(F(" %   "));         // 補空格定長輸出
  }
}
