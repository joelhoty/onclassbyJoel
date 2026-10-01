# 🛠️ Arduino IDE 開發環境教學與 I2C LCD 1602 實體硬體接線指南

> **參考實拍照**: `D:\CLASS_NOTE\ec\2026-10-01-145140.jpg` / [2026-10-01-145140.jpg](./2026-10-01-145140.jpg)  
> **對應 HTML 互動筆記**: [arduino_ide_i2c_lcd1602_guide.html](./arduino_ide_i2c_lcd1602_guide.html)  
> **建立時間**: 2026-10-01  
> **適用硬體**: Arduino Uno R3、LCD 1602 液晶模組（帶 PCF8574 I2C 轉接背板）  

---

## 📷 一、實際硬體接線照片解構 (Hardware Wiring)

![Arduino Uno 與 I2C LCD 1602 實體接線圖](./2026-10-01-145140.jpg)

### 🔌 1.1 實體 4-Pin 導線連接清單

在您的照片中，LCD 1602 背部焊接了一塊 **I2C 轉接板（PCF8574 背板）**，只引出 4 根引腳，透過 4 條杜邦線直接連接至 Arduino Uno：

| I2C 轉接板引腳 | 訊號功能 | Arduino Uno 對應腳位 | 接線說明與導線顏色建議 |
| :--- | :--- | :--- | :--- |
| **GND** | 電源負極（接地） | **GND** (POWER 排針) | 黑色或棕色線，接至電源排針的 GND |
| **VCC** | 模組電源正極 | **5V** (POWER 排針) | 紅色或紫色線，接至電源排針的 5V（請勿接 3.3V，對比度會不足） |
| **SDA** | 串列資料線 (Serial Data) | **類比 Pin A4** | 雙向資料傳輸（在 Uno 上，A4 與右上角 AREF 旁的 SDA 引腳內部相通） |
| **SCL** | 串列時脈線 (Serial Clock) | **類比 Pin A5** | 同步時脈傳輸（在 Uno 上，A5 與右上角 AREF 旁的 SCL 引腳內部相通） |

> [!NOTE]
> **為什麼 Uno 的 SDA 是 A4、SCL 是 A5？**  
> ATmega328P 微控制器的硬體 TWI（Two-Wire Interface / I2C）模組的內部物理線路就是硬體綁定在 Port C 的第 4 腳 (PC4 = SDA) 與第 5 腳 (PC5 = SCL)，也就是 Arduino 標示的 `A4` 與 `A5`。Arduino Uno R3 也在數位排針的最上方（AREF 旁邊）額外拉出兩個標示為 `SDA` 和 `SCL` 的孔位，兩者是完全相通的。

---

## 🖥️ 二、Arduino IDE 2.x 開發環境速成手冊

### 2.1 介面導覽與三大核心功能

1. **✔ 編譯 / 驗證 (Verify, `Ctrl + R`)**：
   - 檢查 C++ 語法是否有打錯、檢查 `#include` 函式庫是否存在，**只做編譯不燒錄**。
2. **➔ 上傳 (Upload, `Ctrl + U`)**：
   - 先自動執行編譯，編譯成功後透過 USB 序列埠將機器碼寫入 ATmega328P 晶片中。
3. **🔍 序列埠監視器 (Serial Monitor, `Ctrl + Shift + M`)**：
   - 右上角的放大鏡圖示。用於查看 Arduino 透過 `Serial.print()` 回傳的除錯文字，**視窗右下角鮑率（Baud Rate）必須與程式碼 `Serial.begin(9600)` 一致**。

---

### 2.2 上傳前的兩大必選設定（新手最常卡關點！）

在按下「➔ 上傳」前，必須確認 IDE 上方的下拉選單：
1. **開發板 (Board)**：選擇 `Arduino Uno`。
2. **通訊埠 (Port)**：
   - Windows 系統通常為 `COM3`、`COM4` 或 `COM5`（若拔掉 USB 線該選項會消失，重新插上出現的就是正確的埠）。
   - 若顯示 `COM1` 通常是主機板內建序列埠，**絕不是 Arduino**。

---

### 2.3 安裝必備函式庫：`LiquidCrystal_I2C`

1. 點擊 Arduino IDE 左側側邊欄的 **「書籍/函式庫圖示 (Library Manager)」**（快捷鍵 `Ctrl + Shift + I`）。
2. 在上方搜尋列輸入：`LiquidCrystal I2C`。
3. 找到由 **Frank de Brabander** 或 **Marco Schwartz** 維護的 **`LiquidCrystal I2C`** 函式庫。
4. 點選 **「INSTALL (安裝)」**，安裝完成後才能在程式中使用 `#include <LiquidCrystal_I2C.h>`。

---

## ⚠️ 三、實體硬體上機三大天坑與除錯秘訣（95% 初學者必看！）

如果程式已經上傳成功，但螢幕卻**什麼字都沒顯示**，請立刻檢查以下三點：

### 坑 1：I2C 設備位址不符（Tinkercad 是 0x20，實體通常是 0x27！）
- **Tinkercad 模擬器**中，PCF8574 預設將地址腳位全接地，位址為 `32`（即 `0x20`）。
- **市售實體模組**中，背板的 A0、A1、A2 三個位址焊點預設都是**懸空未焊接**：
  - 若晶片型號為 **PCF8574T**（晶片表面有印），預設位址是 **`0x27`**。
  - 若晶片型號為 **PCF8574AT**（型號帶 A），預設位址是 **`0x3F`**。
- **解法**：請先執行下文的 **I2C Scanner 程式**，立刻抓出真身！

---

### 坑 2：背部「藍色十字可變電阻」未調整（對比度過高/過低）
- 轉接板背後有一個**藍色方塊，中間有十字螺絲**（如圖）：
  - **對比度太低**：背光有亮，但字跡全透明，看起來像壞掉沒通電。
  - **對比度太高**：第一列顯示 16 個實心黑方塊。
- **解法**：拿小一字或十字起子，**順時針或逆時針慢慢旋轉**，直到黑方塊變淡、清晰英文字母浮現為止！

---

### 坑 3：背光跳線帽（Jumper Cap）鬆脫
- 轉接板邊緣有兩根排針，上面插著一個黑色或黃色的**小跳線帽**。
- 這是 LCD 背光 LED 的硬體通電開關，如果它掉落或遺失，螢幕背景燈就不會亮，畫面會非常暗。

---

## 🚀 四、實體驗證兩部曲（程式碼清單）

### 步驟 1：燒錄「I2C Scanner (位址自動掃描器)」
先確認實體硬體連線是否正常，並取得正確的十六進制位址：

```cpp
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  while (!Serial); // 等待序列埠連線
  Serial.println(F("\n================================="));
  Serial.println(F("🔍 I2C 匯流排位址自動掃描開始..."));
  Serial.println(F("================================="));
  
  byte count = 0;
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print(F("✅ 找到 I2C 裝置！位址為: 0x"));
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      count++;
    } else if (error == 4) {
      Serial.print(F("⚠️ 位址 0x"));
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      Serial.println(F(" 發生不明錯誤"));
    }
  }

  if (count == 0) {
    Serial.println(F("❌ 未找到任何 I2C 裝置！請檢查 5V/GND/SDA/SCL 接線。"));
  } else {
    Serial.print(F("🎉 掃描完成！共找到 "));
    Serial.print(count);
    Serial.println(F(" 個裝置。"));
  }
}

void loop() {
  // 掃描完畢靜止
}
```

> **預期輸出範例**：
> ```
> =================================
> 🔍 I2C 匯流排位址自動掃描開始...
> =================================
> ✅ 找到 I2C 裝置！位址為: 0x27
> 🎉 掃描完成！共找到 1 個裝置。
> ```

---

### 步驟 2：燒錄「實體 LCD 1602 顯示測試程式」
將掃描到的位址（例如 `0x27`）填入第 4 行：

```cpp
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ⚠️ 若剛才 Scanner 掃出來是 0x3F 或 0x20，請將 0x27 改為該位址
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // 1. 初始化 LCD
  lcd.init();
  
  // 2. 開啟 LCD 背光
  lcd.backlight();

  // 3. 游標移至第 0 列第 0 格 (第一行)
  lcd.setCursor(0, 0);
  lcd.print("Arduino Uno I2C");

  // 4. 游標移至第 1 列第 0 格 (第二行)
  lcd.setCursor(0, 1);
  lcd.print("LCD 1602 Ready!");
}

void loop() {
  // 靜態文字展示
}
```

---

## 🎯 五、後續結合 PIR 與繼電器（回顧 Stage 3）

當 LCD 1602 測試成功顯示文字後，即可插上其餘兩組元件：
1. **PIR 人體感測器**：
   - 訊號線（SIG）接 **Pin 8**
   - VCC 接 **5V**，GND 接 **GND**
2. **繼電器模組 (Relay)**：
   - 控制端（IN）接 **Pin 4**
   - VCC 接 **5V**，GND 接 **GND**
3. 搭配先前設計的狀態切換程式：
   - 待機時顯示：`"DECT...."`
   - 感應移動時顯示：`"Warning"` 並驅動繼電器開燈
   - 人員離開 5 秒後熄燈並自動切換回待機！
