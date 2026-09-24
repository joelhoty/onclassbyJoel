---
marp: true
theme: tech
paginate: true
html: true
header: '車用收音機事件廣播 // 隧道插播 × RDS-TA'
footer: 'onclassbyJoel // Tech Theme // 2026-09-24'
---

<!-- _class: lead -->

<span class="tag-label">SLIDE 01 // TITLE</span>

# 進雪隧時，音樂會消失、收音機卻突然播路況？

> **車用收音機的事件廣播：隧道插播、RDS-TA 與收聽條件**

- **問題**：經過雪山隧道等特定地點，收音機突然插入事件廣播
- **答案不是一種技術**：隧道蓋台插播 ≠ RDS-TA 自動切音源
- **關鍵條件**：音源是否走 FM，以及車廠允不允許打斷

<div class="terminal-card">
  <strong>Source</strong>: <code>ec/car_radio_traffic_broadcast_rds_ta_tunnel.md</code>
</div>

<p class="meta-line">最後更新時間: 2026-09-24 22:55:00 (UTC+8) · Cursor Grok 4.6</p>

<!--
🗣️ 口說重點：
這份簡報把「隧道裡突然播路況」拆成兩套完全不同的系統：一套是把廣播內容直接蓋掉，一套是用 1 bit 旗標叫車機切音源。
-->

---

<span class="tag-label">SLIDE 02 // TWO SYSTEMS</span>

## 兩套機制，不要混在一起

- **雪山隧道插播**：高公局 **FM 全頻道隧道廣播**。洞外電台進不去，系統轉播進來；行控中心可在**所有轉播頻道**同時插播
- **RDS-TA**：類比 FM 夾帶 1 bit「正在播路況」旗標，車機**自動把音源切到收音機**
- **收聽條件差很多**：插播只要切到 FM；TA 還要電台送旗標、車機開啟、車廠允許打斷

<div class="terminal-card">
  <strong>Rule</strong>: CarPlay / 藍牙聽音樂時，<strong>隧道插播一定聽不到</strong>；RDS-TA 看車廠；PWS 走手機。
</div>

<!--
🗣️ 口說重點：
雪隧是「換掉聲音內容」，RDS-TA 是「舉旗請車機切台」。一個不需要車機配合，一個幾乎全靠車機韌體。
-->

---

<span class="tag-label">SLIDE 03 // LANDSCAPE</span>

## 四種路況廣播怎麼分

| 技術 | 一句話 | 自動切音源？ | 常見地區 |
|---|---|---|---|
| **隧道插播** | 轉播外部電台，必要時蓋台 | 否，蓋的是內容 | 台灣雪山隧道 |
| **RDS-TA／EON** | FM 夾帶 TA 旗標 | **是** | 歐洲普遍；台灣不明 |
| **HAR** | 路邊低功率重複播路況 | 否，要自己轉台 | 美／日 1620 kHz |
| **PWS** | 基地台推送到手機 | 不經收音機 | 台灣與多數國家 |

<div class="terminal-card">
  <strong>Noise</strong>: 同頻強訊號壓過（capture effect）或互調干擾，不是刻意廣播。
</div>

---

<span class="tag-label">SLIDE 04 // DIAGNOSE</span>

## 你聽到的到底是哪一種？

| 觀察 | 最可能 |
|---|---|
| 隧道內，FM 轉哪一台都是同一段 | **隧道插播** |
| 車機顯示 TA／Traffic，CD／USB 也被切斷 | **RDS-TA** |
| 路邊告示「請轉 XXX kHz」 | **HAR** |
| 聲音從手機來，伴隨刺耳警報 | **PWS** |
| 一般節目、斷斷續續 | **訊號壓過／干擾** |

<div class="terminal-card">
  <strong>Hint</strong>: 雪隧插播與隧道揚聲器 PA 是兩回事，不要當成同一套系統。
</div>

---

<span class="tag-label">SLIDE 05 // HSUEHSHAN</span>

## 雪隧：12.9 km 洞外訊號進不去

1. 外部 FM 在隧道內收不到，系統把電台**整批轉播**進來
2. SDR 實測：洞內可見大量**強度相近**的電台訊號 → 是轉播，不是漏進來
3. 平時：行車安全、路況、應變；報導亦提及速限、逗留、甚至唸車號
4. 緊急時：行控中心人員輪流**插播**（熄火、留車內或棄車疏散）

<div class="terminal-card">
  <strong>Listen</strong>: 音響開著，而且必須在 <span class="stat-badge">FM</span> 模式。官方建議收聽警廣。
</div>

<!--
🗣️ 口說重點：
雪隧不是某一台電台特別服務隧道，而是隧道自己蓋了一套轉播與插播系統。
-->

---

<span class="tag-label">SLIDE 06 // ALL CHANNELS</span>

## 「全頻道」不是整個 FM 波段

- 高公局：隧道內透過 **FM 所有頻道**播放，打開 FM 都能收到該廣播
- 承包商：全台唯一**全頻道調頻 FM 廣播**
- **已確定**：插播不限某一台；任何**有轉播的 FM 台**都會聽到
- **未確定**：沒有電台的**空白頻率**有沒有插播

<div class="terminal-card">
  <strong>Industry term</strong>: 日本規格「全チャンネル」= 所有<strong>轉播頻道</strong>，不是每個頻點。
</div>

<!--
🗣️ 口說重點：
第一次查證容易過度解讀成「整個波段都有訊號」。比較穩的結論是：所有轉播頻道都會插播；空白頻率還沒證實。
-->

---

<span class="tag-label">SLIDE 07 // JP SPEC</span>

## 日本規格：逐台處理、IF 層一齊插播

```
洞外天線 → 每台一組接收部（最多 12 ch）
         → 放送切換部 ← 麥克風／預錄
         → 插播只調變一次（10.7 MHz IF）再複製給所有送信部
         → Combiner → LCX 洩漏同軸電纜
```

- **逐台處理**：不是整個波段一起放大；空白頻率沒有發射
- **AF 中繼**會丟掉立體聲／RDS；**IF 中繼**可保留
- 插播音訊頻寬 200 Hz–7.5 kHz → **單聲道、以人聲為主**

<div class="terminal-card">
  <strong>Ref</strong>: 國土交通省《トンネル内ラジオ再放送設備（割込み有り）》2021-03
</div>

---

<span class="tag-label">SLIDE 08 // HYPOTHESES</span>

## 雪隧可能的四種架構（推論）

| 作法 | 空白頻率 | 吻合度 |
|---|---|---|
| ① 逐台處理（日本式） | 沒有 | **中高** |
| ② 寬頻直通＋本地發射機蓋台 | 沒有 | **中高** |
| ③ 數位寬頻 DSP 合成 | 視設計 | 中 |
| ④ 疊加壓台（capture effect） | 視設計 | **低**（易混音） |

- 廠商清單有「全頻道射頻放大器＋Combiner＋光電轉換器」→ 至少一段是 **RF over fiber**
- 末端是 **FM 天線**還是 **LCX**：目前**沒有證據**能確定

<div class="terminal-card">
  <strong>Status</strong>: 四種作法都是推論。需要標案文件或實測才能定案。
</div>

---

<span class="tag-label">SLIDE 09 // EXPERIMENT</span>

## 進隧道就能自己驗證

| 實驗 | 結果 → 傾向 |
|---|---|
| 插播時轉 **空白頻率** | 聽得到 → ③／④；聽不到 → ①／② |
| RDS 台名／立體聲燈 | 消失 → AF 再調變；保留 → 寬頻或 IF 中繼 |
| 跟洞外同一台比 **延遲** | 明顯延遲 → 數位處理（③） |
| SDR 看插播頻譜 | 空白頻率出現新載波 → ③ |
| 插播 **音質** | 窄頻單聲道 → 接近日本插播調變部 |

<div class="terminal-card">
  <strong>Safety</strong>: 實測用副駕／乘客操作，駕駛不要分心轉台。
</div>

---

<span class="tag-label">SLIDE 10 // RDS PHY</span>

## RDS：類比 FM 上掛的數位資料通道

```
kHz  0────15   19   23────53   57
     [ L+R ]  導頻  [ L−R ]  [RDS]
```

- **57 kHz** 副載波 = 導頻 19 kHz × 3，人耳聽不到
- 速率只有 <span class="stat-badge">1187.5 bit/s</span>，BPSK 調變
- 設計重點：**向後相容**。舊收音機照常播，新機器多一條資料通道
- 精確說法：不是「無線數位廣播」，是「**類比 FM 附掛的數位資料**」

<div class="terminal-card">
  <strong>Contrast</strong>: DAB／DAB+ 聲音本身就是數位；HD Radio 則在類比旁加數位旁帶。
</div>

---

<span class="tag-label">SLIDE 11 // TA FLAG</span>

## TP 是「有服務」，TA 是「正在播」

| 欄位 | 大小 | 意義 |
|---|---|---|
| **TP** | 1 bit | 本台**提供**路況服務 |
| **TA** | 1 bit | 本台**現在正在**播路況 |
| EON | 別台資訊 | A 台通知「B 台 TA=1」，車機暫時跳過去 |
| TMC | 事件碼 | 給導航畫地圖，不是給喇叭播 |

```
開 TA → 電台 TP=1？→ 監看 TA
TA 0→1：暫停目前音源，切到收音機
TA 1→0：恢復原本音源與音量
```

<div class="terminal-card">
  <strong>Firmware</strong>: EON 跳台邏輯在 MCU 韌體，不在解碼晶片。電台還得真的送 EON。
</div>

---

<span class="tag-label">SLIDE 12 // CONDITIONS</span>

## 三個條件缺一不可；CarPlay 更嚴

| 技術 | 聽得到的前提 | CarPlay／藍牙 |
|---|---|---|
| 雪隧插播 | 任何 FM 收音機，切到 **FM** | **聽不到**（音訊不經收音機） |
| RDS-TA | 電台送旗標 ＋ 開啟 TA ＋ 車廠允許打斷 | **看車廠** |
| PWS | 手機開機有訊號 | **收得到**（發聲裝置未驗證） |

- 「能不能打斷」不是 MCU 做不做得到，而是**車廠音訊優先順序**
- TA 不是「低頻訊號」，是 **57 kHz 副載波上的低速率旗標**

<div class="terminal-card">
  <strong>Do this</strong>: 進雪山隧道前，把音源從 CarPlay／藍牙<strong>切回 FM</strong>。
</div>

<!--
🗣️ 口說重點：
這頁是整場最實用的結論。隧道插播不會來救你的 Spotify；想聽應變指示，就先切回收音機。
-->

---

<span class="tag-label">SLIDE 13 // COMPARE</span>

## 雪隧插播 vs RDS-TA

| 比較 | 雪山隧道插播 | RDS-TA |
|---|---|---|
| 做法 | 直接換掉**聲音內容** | 送**旗標**，車機決定要不要切 |
| 覆蓋 | 隧道內所有轉播頻道 | 送 TA 的電台服務範圍 |
| 車機配合 | 不用 | 要支援且開啟 |
| 打斷非 FM 音源 | 不能 | 可以，看車廠 |
| 誰按下按鈕 | 高公局行控中心 | 電台播控 |
| 台灣現況 | **已查證在運作** | 不明 |

<div class="terminal-card">
  <strong>Open</strong>: 空白頻率、AM、轉播清單、台灣電台是否送 TP／TA，都還沒查證。
</div>

---

<span class="tag-label">SLIDE 14 // SOURCES</span>

## 主要來源與可信度

| ID | 來源 | 用途 |
|---|---|---|
| **S001** | 高公局雪隧 FAQ | 全頻道、行控插播（摘要曾截斷） |
| **S002** | 通馳科技承包資料 | 全頻道 FM、設備清單 |
| **S006** | 日本國土交通省規格書 | 逐台／IF 一齊插播架構 |
| **S007** | PIARC VBI 手冊 | Voice Break-In 定義 |
| S003–S004, S008–S009 | 報導／論壇／維基 | 播報內容、SDR、LCX 輔助 |

<div class="terminal-card">
  <strong>Note</strong>: RDS／TA 底層規格對應 IEC 62106，本次未查一手文件，屬通訊工程通識。
</div>

<!--
🗣️ 口說重點：
雪隧結論以高公局與承包商為主；系統架構則是拿日本規格當參照，不要把推論講成已證實。
-->

<!--
## 📝 提示詞歷史與變更記錄 (Prompt Archive)

### 🔹 [2026-09-24 22:55:00] [Cursor Grok 4.6]
- Prompt: 利用marp cli將 @20260924-car-radio-traffic-broadcast-rds-ta-tunnel.md 轉換成html
- 變更摘要: 將筆記整理為 14 頁 Marp 簡報，並以 Marp CLI 輸出 HTML。

### 🔹 [2026-09-24 23:00:00] [Cursor Grok 4.6]
- Prompt: 可以改為 tech theme嗎？
- 變更摘要: 改掛 marp-slide 正式 tech 主題（GitHub 深色、# / ## 標題、// 頁尾），以 theme: tech + Marp CLI --theme-set 重新編譯。
-->
