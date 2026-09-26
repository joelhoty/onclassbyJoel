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

若載入 **21 點手部關鍵點** 的 YOLO pose 模型（例如以 Ultralytics Hand Keypoints 資料集訓練的模型），系統可用手部幾何規則辨識以下手勢。

> ⚠️ 預設的 `yolov8n-pose.pt` 是 COCO **17 點人體姿態** 模型，只能定位手腕位置（畫面會顯示「偵測到手部位置」），**無法**分辨下列手勢。


- ✊ **握拳 (Fist)**
- 🖐️ **張開手掌 (Open Palm)**
- ✌️ **勝利手勢 (Victory / Peace)**
- 👍 **豎大拇指 (Thumbs Up)**
- 👎 **拇指向下 (Thumbs Down)**
- ☝️ **比 1 / 指點 (Pointing)**
- 👌 **OK 手勢 (OK Sign)**
- 🤟 **我愛你手勢 (I Love You)**
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

> ⚠️ 目前 `train_gesture.py` 只把照片依類別存到 `dataset/images/train/<類別>/`，**不會產生** YOLO 物件偵測所需的標註框檔（`labels/*.txt`），也沒有驗證集；直接用 `yolov8n.pt` 做偵測訓練會找不到標註。可改為：(a) 使用 `yolov8n-cls.pt` 做影像分類（需 `train/`、`val/` 類別資料夾結構），或 (b) 先用 Roboflow、Label Studio 等工具框選手部後再訓練偵測模型。
- 訓練完成後，最佳權重檔將會自動儲存至 `models/best.pt`。

### 第三步：使用自訂模型執行實時辨識

```bash
python main.py --model models/best.pt
```

---

## 💡 技術說明 (Technical Highlights)

1. **實時間姿態推論**: 利用 Ultralytics YOLO 高效能神經網路模型，在具備 GPU 或效能足夠的 CPU 上可達約 30 FPS 以上的即時推論速度（實際依硬體而定）。
2. **多國語言 HUD 介面**: 透過 PIL/Pillow 動態繪製微軟正黑體，解決傳統 OpenCV 中文字體亂碼問題。
3. **旋轉無關關鍵點分析 (Rotation-Invariant Analysis)**: 基於手掌中心與關節向量距離計算，對手掌旋轉具一定容忍度（大角度或側面時仍可能誤判）。

---

## 📝 提示詞歷史與變更記錄 (Prompt Archive)

### 🔹 [2026-09-26 23:01:05] [Claude Opus 5.5 (claude-opus-5-5) / Claude Code] 變更紀錄
- **模型/Agent**: Claude Opus 5.5 (claude-opus-5-5) / Claude Code
- **Prompt 原文**:
  ```text
  檢視所有檔案內容中的敘述與說明，確認概念與敘述的正確性
  修正後，將修改內容，附加在每個檔案的最下方區塊並標誌時間戳記與模型代號
  確認概念與解釋說明都是正確的
  ```
- **變更摘要**: 全面檢視概念與敘述正確性並修正：
  - 預設 yolov8n-pose.pt 為 COCO 17 點人體姿態模型，無法分辨手勢；需 21 點手部關鍵點模型
  - train_gesture.py 未產生 YOLO 標註框與驗證集，補充可行的分類／標註替代做法
  - FPS 與旋轉容忍度改為依硬體與角度而定的保守敘述；🤟 為「我愛你」手勢
