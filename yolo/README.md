# 🖐️ YOLO 結合視訊鏡頭手勢辨識系統 (YOLO Hand Gesture Recognition System)

本專案是一個基於 **YOLO (Ultralytics)** 與 **OpenCV 視訊鏡頭** 的實時手勢辨識系統。具備即時視訊串流處理、骨架與關鍵點標註、中文手勢標籤顯示、截圖保存，以及自訂模型訓練等完整功能。

---

## 📁 專案檔案結構 (Project Structure)

```text
yolo/
├── gesture_detector.py   # YOLO 手勢偵測核心模組 (支援 21 點骨架與特徵幾何計算)
├── main.py               # 視訊鏡頭實時推論主程式 (OpenCV GUI & HUD 介面)
├── train_gesture.py      # 自訂手勢資料集收集與 YOLO 模型訓練腳本
├── requirements.txt      # Python 套件依賴清單
├── screenshots/          # 截圖自動保存目錄
└── README.md             # 說明文件
```

---

## ⚙️ 環境安裝 (Installation)

請確認已安裝 Python 3.8+，並在 `yolo/` 目錄下執行以下指令安裝套件：

```bash
pip install -r requirements.txt
```

> **主要依賴庫：** `ultralytics`, `opencv-python`, `pillow`, `numpy`, `torch`

---

## 🚀 快速開始 (Quick Start)

### 1. 執行視訊鏡頭實時辨識

直接執行 `main.py` 啟動預設攝影機並載入 YOLO 模型：

```bash
python main.py
```

### 2. 進階參數選項

| 參數 | 預設值 | 說明 |
| :--- | :--- | :--- |
| `--model` | `yolov8n-pose.pt` | 使用的模型檔案路徑 (亦可指定自訂 `models/best.pt`) |
| `--camera` | `0` | 視訊鏡頭 ID (0 為內建鏡頭，1, 2 為外接鏡頭) |
| `--conf` | `0.4` | 辨識置信度門檻 (0.0 ~ 1.0) |
| `--width` | `1280` | 畫面寬度 |
| `--height` | `720` | 畫面高度 |

**範例：**
```bash
python main.py --camera 0 --conf 0.5 --model yolov8n-pose.pt
```

---

## 🖐️ 支援辨識手勢 (Supported Gestures)

系統內建強大的手部幾何關鍵點分析演算法，能辨識以下多種常見手勢：

- ✊ **握拳 (Fist)**
- 🖐️ **張開手掌 (Open Palm)**
- ✌️ **勝利手勢 (Victory / Peace)**
- 👍 **豎大拇指 (Thumbs Up)**
- 👎 **拇指向下 (Thumbs Down)**
- ☝️ **比 1 / 指點 (Pointing)**
- 👌 **OK 手勢 (OK Sign)**
- 🤟 **愛心 / 搖滾 (I Love You)**
- 3️⃣ **比 3 (Three)**
- 4️⃣ **比 4 (Four)**

---

## 🎮 鍵盤快捷鍵 (Control Keys)

在視訊畫面視窗開啟時，可使用以下快捷鍵控制系統：

| 按鍵 | 功能說明 |
| :--- | :--- |
| `Q` 或 `ESC` | 結束程式 |
| `S` | 拍攝當前畫面截圖 (自動儲存至 `screenshots/` 資料夾) |
| `C` | 切換攝影機鏡頭 (0 ↔ 1) |
| `H` | 開啟 / 隱藏手部骨架與關鍵點繪製 (Skeleton) |
| `P` | 暫停 / 繼續實時視訊推論 |

---

## 🏋️ 自訂手勢訓練指南 (Train Custom Gestures)

若想讓 YOLO 學習特定專屬手勢，可使用 `train_gesture.py`：

### 第一步：使用鏡頭收集手勢照片

執行以下命令收集特定手勢（例如 `heart` 手勢 30 張）：

```bash
python train_gesture.py --collect --class-name heart --count 30
```
- 按 **空白鍵 (SPACE)** 拍照記錄。

### 第二步：開始 YOLO 模型微調訓練

當收集完成多個手勢類別後，執行訓練：

```bash
python train_gesture.py --train --epochs 30
```
- 訓練完成後，最佳權重檔將會自動儲存至 `models/best.pt`。

### 第三步：使用自訂模型執行實時辨識

```bash
python main.py --model models/best.pt
```

---

## 💡 技術說明 (Technical Highlights)

1. **實時間姿態推論**: 利用 Ultralytics YOLO 高效能神經網路模型，達到每秒 >30 FPS 的即時推論速度。
2. **多國語言 HUD 介面**: 透過 PIL/Pillow 動態繪製微軟正黑體，解決傳統 OpenCV 中文字體亂碼問題。
3. **旋轉無關關鍵點分析 (Rotation-Invariant Analysis)**: 基於手掌中心與關節向量距離計算，不受手掌角度變換影響。
