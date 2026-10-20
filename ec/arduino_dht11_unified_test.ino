// ============================================================================
// 專案名稱: Arduino Uno + DHT11 溫濕度感測器基本測試 (Adafruit Unified Sensor)
// 檔案路徑: D:\CLASS_NOTE\ec\arduino_dht11_unified_test.ino
// 建立日期: 2026-10-20
// 相依函式庫: 
//   1. DHT sensor library by Adafruit
//   2. Adafruit Unified Sensor Driver
// 接線說明:
//   DHT11 VCC (+)  -> Arduino 5V
//   DHT11 DATA (S) -> Arduino Pin 7
//   DHT11 GND (-)  -> Arduino GND
// ============================================================================

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// 定義 DHT11 訊號引腳連接至 Arduino 數位腳位 7
#define DHTPIN 7

// 定義使用的感測器型號為 DHT11
#define DHTTYPE DHT11

// 實例化 DHT_Unified 物件
DHT_Unified dht(DHTPIN, DHTTYPE);

// 採樣延遲時間 (毫秒)
uint32_t delayMS;

void setup() {
  // 初始化序列埠通訊，鮑率設定為 9600 bps
  Serial.begin(9600);
  while (!Serial) {
    ; // 等待序列埠連接（適用於部分原生 USB 板子）
  }

  Serial.println(F("========================================"));
  Serial.println(F("DHT11 溫濕度感測器單元測試 (Unified API)"));
  Serial.println(F("========================================"));

  // 初始化 DHT 感測器
  dht.begin();

  // 取得感測器規格與推薦最小採樣延遲
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("--- 溫度感測器規格 ---"));
  Serial.print  (F("感測器型號: ")); Serial.println(sensor.name);
  Serial.print  (F("驅動程式版本: ")); Serial.println(sensor.version);
  Serial.print  (F("量測最大值: ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("量測最小值: ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("解析度:     ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("----------------------"));

  dht.humidity().getSensor(&sensor);
  Serial.println(F("--- 濕度感測器規格 ---"));
  Serial.print  (F("感測器型號: ")); Serial.println(sensor.name);
  Serial.print  (F("量測最大值: ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("量測最小值: ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("解析度:     ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("----------------------"));

  // 設定採樣間隔（至少需大於 min_delay，DHT11 建議至少 1000ms）
  delayMS = sensor.min_delay / 1000;
  if (delayMS < 1000) {
    delayMS = 1000; // 保障每次讀取間隔至少 1 秒
  }
}

void loop() {
  // 延遲指定採樣週期
  delay(delayMS);

  // 1. 讀取溫度事件
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("✖ 溫度讀取失敗 (NaN)！請檢查接線。"));
  } else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature, 2);
    Serial.println(F("°C"));
  }

  // 2. 讀取相對濕度事件
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("✖ 濕度讀取失敗 (NaN)！請檢查接線。"));
  } else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity, 2);
    Serial.println(F("%"));
  }
}
