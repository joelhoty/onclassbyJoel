//YWROBOT
//Compatible with the Arduino IDE 1.0
//Library version:1.1
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // 設定 LCD 位址為 0x27，16 字元 2 行

void setup()
{
  lcd.init();                      // 初始化 LCD
  lcd.backlight();                 // 開啟背光
  
  // 讀取未接線的類比腳位 A0 作為隨機數種子，確保每次開機後的隨機序列不同
  randomSeed(analogRead(A0));
}

void loop()
{
  // --- 第一行：顯示隨機 10 個 ASCII 字元 ---
  lcd.setCursor(0, 0);             // 移至第 1 行開頭
  for (int i = 0; i < 10; i++) {
    // ASCII 可見字元範圍為 33 ('!') 到 126 ('~')
    char randChar = (char)random(33, 127);
    lcd.print(randChar);
  }
  lcd.print("      ");             // 印出 6 個空格補滿 16 格，避免殘留多餘字元

  // --- 第二行：顯示 hello ---
  lcd.setCursor(0, 1);             // 移至第 2 行開頭
  lcd.print("hello           ");   // 印出 hello 並補齊空格

  // --- 等待 2 秒 ---
  delay(2000);
}