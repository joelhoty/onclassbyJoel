// ============================================================================
// 專案名稱: Arduino Uno + I2C LCD 1602 HelloWorld 功能測試與方法應用
// 檔案路徑: D:\CLASS_NOTE\ec\arduino_i2c_lcd1602_helloworld.ino
// 建立日期: 2026-10-20
// 使用函式庫: LiquidCrystal_I2C (由 Frank de Brabander / YWROBOT 提供)
// 接線說明:
//   LCD GND -> Arduino GND
//   LCD VCC -> Arduino 5V
//   LCD SDA -> Arduino A4 (或專用 SDA 腳)
//   LCD SCL -> Arduino A5 (或專用 SCL 腳)
// ============================================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// 初始化 LCD 物件: 位址 0x27 (部分為 0x3F), 16 欄, 2 列
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 自定義字元 1: 愛心圖示 (5x8 點陣)
byte heartIcon[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

// 自定義字元 2: 滿電電池圖示 (5x8 點陣)
byte batteryIcon[8] = {
  0b01110,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};

unsigned long previousMillis = 0;
const long interval = 1000;
int counter = 0;

void setup() {
  // 1. 初始化 LCD 模組
  lcd.init();
  lcd.init();          // 二次初始化確保 I2C 匯流排穩定重置
  
  // 2. 開啟液晶背光
  lcd.backlight();

  // 3. 註冊自定義字元到 CGRAM (編號 0 與 1)
  lcd.createChar(0, heartIcon);
  lcd.createChar(1, batteryIcon);

  // 4. 印出啟動訊息 (對照上課 HelloWorld 範例)
  lcd.setCursor(1, 0); // (欄 1, 列 0)
  lcd.print("Hello, LCD1602");
  lcd.write(0);        // 印出自訂愛心
  
  lcd.setCursor(0, 1); // (欄 0, 列 1)
  lcd.print("Status: Active ");
  lcd.write(1);        // 印出自訂電池

  delay(2500);         // 停留 2.5 秒展示啟動畫面

  // 5. 清除畫面，進入主控監控介面
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Timer Count: ");
  lcd.setCursor(0, 1);
  lcd.print("I2C Addr: 0x27");
}

void loop() {
  unsigned long currentMillis = millis();

  // 每秒更新一次計數器，採用「定長覆寫」避免畫面閃爍
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    counter++;

    // 移動至第 0 列第 12 格更新數字
    lcd.setCursor(12, 0);
    if (counter < 10) {
      lcd.print("00");
      lcd.print(counter);
    } else if (counter < 100) {
      lcd.print("0");
      lcd.print(counter);
    } else {
      lcd.print(counter);
    }

    // 每 10 秒展示一次游標閃爍與背光警示效果
    if (counter % 10 == 0) {
      lcd.setCursor(15, 1);
      lcd.cursor();     // 顯示游標
      lcd.blink();      // 游標閃爍
      delay(500);
      lcd.noBlink();    // 關閉閃爍
      lcd.noCursor();   // 隱藏游標
    }
  }
}
