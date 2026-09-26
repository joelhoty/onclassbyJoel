---
marp: true
theme: default
paginate: true
header: '新莊土壤液化與地質心理學 // 高中課堂分享'
footer: 'Geological Memory & Risk Psychology // Tech Style'
---

<style>
@import url('https://fonts.googleapis.com/css2?family=Noto+Sans+TC:wght@400;500;700&family=Fira+Code:wght@400;500;700&display=swap');

:root {
  --color-background: #0d1117;
  --color-foreground: #c9d1d9;
  --color-heading: #58a6ff;
  --color-accent: #7ee787;
  --color-code-bg: #161b22;
  --color-border: #30363d;
  --font-default: 'Noto Sans TC', 'Segoe UI', 'Microsoft JhengHei', sans-serif;
  --font-code: 'Fira Code', 'Consolas', 'Courier New', monospace;
}

section {
  background-color: var(--color-background);
  color: var(--color-foreground);
  font-family: var(--font-default);
  font-weight: 400;
  box-sizing: border-box;
  border-left: 5px solid var(--color-accent);
  position: relative;
  line-height: 1.6;
  font-size: 21px;
  padding: 50px 65px;
}

h1, h2, h3, h4, h5, h6 {
  font-weight: 700;
  color: var(--color-heading);
  margin: 0;
  padding: 0;
  font-family: var(--font-code), var(--font-default);
}

h1 {
  font-size: 40px;
  line-height: 1.3;
  text-align: left;
}

h1::before {
  content: '# ';
  color: var(--color-accent);
}

h2 {
  font-size: 30px;
  margin-bottom: 24px;
  padding-bottom: 10px;
  border-bottom: 2px solid var(--color-border);
}

h2::before {
  content: '## ';
  color: var(--color-accent);
}

h3 {
  color: #79c0ff;
  font-size: 22px;
  margin-top: 18px;
  margin-bottom: 10px;
}

h3::before {
  content: '### ';
  color: var(--color-accent);
}

ul, ol {
  padding-left: 28px;
  margin-top: 12px;
}

li {
  margin-bottom: 12px;
  line-height: 1.5;
}

li::marker {
  color: var(--color-accent);
}

.terminal-card {
  background-color: var(--color-code-bg);
  border: 1px solid var(--color-border);
  border-radius: 8px;
  padding: 12px 18px;
  margin-top: 18px;
  font-family: var(--font-code), var(--font-default);
  font-size: 15px;
  color: #8b949e;
  border-left: 3px solid var(--color-heading);
}

.terminal-card strong {
  color: var(--color-accent);
}

.stat-badge {
  display: inline-block;
  background: rgba(88, 166, 255, 0.15);
  border: 1px solid rgba(88, 166, 255, 0.4);
  border-radius: 4px;
  padding: 2px 8px;
  color: #58a6ff;
  font-weight: bold;
  font-family: var(--font-code);
}

footer {
  font-size: 13px;
  color: #8b949e;
  font-family: var(--font-code);
  position: absolute;
  left: 65px;
  right: 65px;
  bottom: 24px;
  text-align: right;
}

footer::before {
  content: '// ';
  color: var(--color-accent);
}

header {
  font-size: 13px;
  color: #8b949e;
  font-family: var(--font-code);
  position: absolute;
  left: 65px;
  top: 20px;
}

section.lead {
  border-left: 5px solid var(--color-accent);
  display: flex;
  flex-direction: column;
  justify-content: center;
}

section.lead h1 {
  margin-bottom: 18px;
}

section.lead p {
  font-size: 19px;
  color: var(--color-foreground);
}

strong {
  color: var(--color-accent);
  font-weight: 700;
}

.tag-label {
  display: inline-block;
  background-color: #238636;
  color: #ffffff;
  padding: 2px 8px;
  border-radius: 4px;
  font-size: 12px;
  font-family: var(--font-code);
  margin-bottom: 10px;
}
</style>

<!-- _class: lead -->

<span class="tag-label">SLIDE 01 // OVERVIEW & HOOK</span>

# 腳下地基在搖晃，我們為什麼集體假裝沒事？

> **新莊土壤液化區的居民自救困境與心理盲點**

- **解開新莊土壤液化**：探討高潛勢區居民的自救與結構困境
- **歷史圖資 × 心理學**：結合百年臺灣堡圖與認知偏差找出盲點
- **治理突破路徑**：探索走出風險認知斷層的具體行動方案

<div class="terminal-card">
  <strong>Goal</strong>: 揭露生活地質真相，突破「知道危險卻不動」的心理防線。
</div>

<!--
🗣️ 口說重點：
大家今天聽完就會發現，原來我家客廳地板底下藏著歷史秘密，而我們的大腦一直在騙自己很安全！

🎨 影像生成 Prompt：
An artistic cross-section illustration of an old urban street in Taiwan, modern apartment buildings on the asphalt surface, but underneath reveals an ancient sandy riverbed with water saturation, cinematic lighting, editorial conceptual style, high detail --ar 16:9
-->

---

<span class="tag-label">SLIDE 02 // REALITY GAP</span>

## 政府公開地質圖，大家反而更沉默？

- **公開潛勢地質圖**：<span class="stat-badge">2016</span> 美濃地震後政府全面公開土壤液化圖資
- **預期效果破滅**：原預期資訊透明會驅動老屋自主補強與避險投保
- **現實極度低迷**：公開後自主補強率與地震險投保反而陷入停滯

<div class="terminal-card">
  <strong>System Status</strong>: 資訊公開 ≠ 行動展開。透明度反倒誘發了群體鴕鳥心態。
</div>

<!--
🗣️ 口說重點：
政府原本以為把紅黃綠地質圖放上網大家就會去修房子，結果大家反而像鴕鳥一樣把頭埋進沙子裡。

🎨 影像生成 Prompt：
An editorial illustration of Taiwanese city dwellers looking down at glowing smartphone screens showing a red hazard map in an alley, while ignoring visible cracks in the walls, moody atmosphere, conceptual art --ar 16:9
-->

---

<span class="tag-label">SLIDE 03 // GEOLOGICAL OVERLAY</span>

## 水泥蓋得住河道，蓋不住地質記憶！

- **百年 GIS 圖資套疊**：比對 <span class="stat-badge">1904</span> 年臺灣堡圖與現代高潛勢地質圖
- **八成二曾是水體**：新莊高潛勢區有 <span class="stat-badge">82%</span> 原是大漢溪舊河道與古埤塘
- **地質記憶抹不掉**：水泥柏油封蓋地表，地下依舊是未固結飽和細砂

<div class="terminal-card">
  <strong>Data Overlay</strong>: 空間開發僅花數十年，但沉積地質記憶跨越千百年。
</div>

<!--
🗣️ 口說重點：
我們每天走的新莊馬路，百年前其實是大漢溪分支或埤塘，水泥只是蓋住表面，地下的水和沙一直都在。

🎨 影像生成 Prompt：
A split-screen double exposure map visual: on one side a 1904 vintage Japanese topographical map with blue streams and ponds, blending into modern GIS red hazard zones and dense city street blocks, clean infographic aesthetic --ar 16:9
-->

---

<span class="tag-label">SLIDE 04 // SURVEY DISCOVERY</span>

## 超怕房子下陷，卻僅 3% 做諮詢？

- **不知自家在紅區**：實體問卷顯示超過 <span class="stat-badge">60%</span> 居民不知自家位於液化紅區
- **租屋族知情極低**：租屋族群知情比例更僅有 <span class="stat-badge">12.5%</span>，面臨資訊盲區
- **驚人知行割裂**：高達 <span class="stat-badge">81.6%</span> 恐懼倒塌下陷，但申請結構諮詢僅 <span class="stat-badge">3.3%</span>

<div class="terminal-card">
  <strong>Cognitive Gap</strong>: 81.6% 恐懼下陷 vs. 3.3% 諮詢補強。巨大落差揭示了行動阻力。
</div>

<!--
🗣️ 口說重點：
調查最驚人的就是這種「認知分裂」——大家心裡明明怕得要死，但真正找政府諮詢老屋健檢的竟然只有3.3%！

🎨 影像生成 Prompt：
A minimalist conceptual infographic: a giant red gauge showing 81.6% extreme fear of collapse, beside a tiny 3.3% progress bar of structural inspection, clean modern vector style, contrast of scale --ar 16:9
-->

---

<span class="tag-label">SLIDE 05 // PSYCHOLOGICAL DEFENSE</span>

## 「921 都沒倒」成了最危險心理盾牌？

- **虛假安全感**：高達 <span class="stat-badge">73%</span> 居民認同「既然 921 沒倒，未來也不會倒」
- **防衛性迴避**：**樂觀偏誤**與**鴕鳥效應**讓人本能屏蔽威脅性警訊
- **現時偏誤驅使**：寧可把預算花在眼前看得到的室內裝潢，犧牲看不見的地基

<div class="terminal-card">
  <strong>Cognitive Bias</strong>: 921 的倖存經驗，從「幸運歷史」異化成了「未來安全的麻醉劑」。
</div>

<!--
🗣️ 口說重點：
很多長輩常說「當年921這裡都沒事啦」，但這種樂觀偏誤反而成了最讓人安心、卻最危險的麻醉劑。

🎨 影像生成 Prompt：
A resident wearing rose-tinted sunglasses smiling peacefully inside a vintage apartment living room, while subtle ground fissures form beneath the floor, satirical editorial art --ar 16:9
-->

---

<span class="tag-label">SLIDE 06 // ECONOMIC PRAGMATISM</span>

## 怕房價下跌，比怕地基掏空更要命？

- **房價衝擊禁忌**：房仲坦言公開液化風險將重創成交行情與銀行鑑價
- **老公寓整合困境**：地基高壓灌漿動輒數百萬，無管委會老公寓極難分攤
- **時間尺度脫節**：三十年房貸與居住週期，讓自住戶滋生「輪不到我」賭徒心態

<div class="terminal-card">
  <strong>Root Cause</strong>: 當房價貶值是「明天即刻發生」，而大地震是「機率事件」，人性選擇沉默。
</div>

<!--
🗣️ 口說重點：
訪談發現大家不敢提的原因超現實：講了房價會跌、整棟公寓幾百萬修繕談不攏，最後乾脆賭這輩子不會輪到自己。

🎨 影像生成 Prompt：
Isometric cutaway of an old 4-story walk-up apartment building in Taiwan, residents arguing over repair cost estimates, a glowing red real estate price tag hanging on the exterior, detailed miniature style --ar 16:9
-->

---

<span class="tag-label">SLIDE 07 // INSTITUTIONAL MISALIGNMENT</span>

## 百年地質撞上三十年房貸與四年任期？

- **能量釋放跨百年**：板塊構造能量累積以世紀計，與人類生命週期嚴重錯位
- **任期難挑隱形工**：四年政治任期難以主動推動「埋在地下看無政績」的灌漿工程
- **弱勢被困紅區**：資訊不均使經濟弱勢者無力換屋，陷入貶值與高風險雙重牢籠

<div class="terminal-card">
  <strong>Structural Dilemma</strong>: 地質尺度 (100yr) vs. 財務尺度 (30yr) vs. 政治尺度 (4yr)。
</div>

<!--
🗣️ 口說重點：
大自然是以百年在算地震的，但首長只看四年任期、我們只看三十年房貸，這種時間錯位讓大家集體踢皮球。

🎨 影像生成 Prompt：
Conceptual surrealist clock with three mismatched interlocking gears labeled "Geological Time", "30-Year Mortgage", and "4-Year Term", floating over an urban landscape, cinematic lighting --ar 16:9
-->

---

<span class="tag-label">SLIDE 08 // INCENTIVE REDESIGN</span>

## 把抗震從「負面扣分」變「資產增值」！

- **建物韌性履歷**：推動房產登錄結構基樁工法，抗震優良享透明市場溢價
- **保費即時折扣**：家具防倒扣件拍照驗證，即享地震險保費實質減免
- **綠色降息貸款**：老屋通過地基加固與灌漿者，公股銀行給予專案利差減讓

<div class="terminal-card">
  <strong>Mechanism Design</strong>: 揚棄道德勸說，建立「做補強就能省錢、賺錢」的正向商業誘因。
</div>

<!--
🗣️ 口說重點：
要讓大家願意動起來，就必須給胡蘿蔔：拍照固定家具就減保費、地基灌漿就降房貸利息，把安全轉化成賺錢誘因！

🎨 影像生成 Prompt：
A modern smartphone interface showing a "Building Resilience A-Grade" green certified badge, next to an insurance discount coupon with a verified checkmark on furniture earthquake-straps, clean UI/UX design --ar 16:9
-->

---

<span class="tag-label">SLIDE 09 // GOVERNANCE INNOVATION</span>

## 公部門主動圈定高危街廓防災都更！

- **劃定高潛勢街廓**：政府主動整併歷史水體古河道區域，優先實施公辦都更
- **全額補助先期探勘**：公部門負擔前期地質鑽探與試灌漿，消除住戶啟動門檻
- **數位孿生與 AR 走讀**：導入 3D 虛擬實境與 AR 科技，讓里民透視門前地下古河道

<div class="terminal-card">
  <strong>Action Framework</strong>: 從「被動等待市民申請」轉型為「主動精準街廓公辦介入」。
</div>

<!--
🗣️ 口說重點：
與其等建商挑熱門地段蓋豪宅，政府應該主動圈出最危險的老街廓，出錢幫忙鑽探灌漿，用科技讓居民親眼看見地下危機。

🎨 影像生成 Prompt：
Community residents in a town hall wearing augmented reality glasses, looking at a 3D holographic projection of the ancient riverbed sediments flowing directly beneath their neighborhood street, inspiring and futuristic --ar 16:9
-->

---

<span class="tag-label">SLIDE 10 // CALL TO ACTION</span>

## 聽完這場分享，今天回家我們能做什麼？

- **打破鴕鳥心態**：辨識出樂觀偏誤與資訊迴避，用理性科學取代僥倖安慰
- **今晚動手兩件事**：
  1. 上網至經濟部地質調查及礦業管理中心「土壤液化潛勢查詢系統」輸入自家地址
  2. 動手檢查高重衣櫃與書架，安裝 L 型金屬防倒扣件
- **參與水文文史走讀**：探索鄰里水文歷史地名（如港底、埤角），重塑環境記憶

<div class="terminal-card">
  <strong>Execute Command</strong>: 防災始於客廳。今晚回家鎖緊一個衣櫃扣件，就是最實質的自救！
</div>

<!--
🗣️ 口說重點：
防災不用等政府發錢或大動工，今晚回家先打開地質系統查查你家、動手鎖緊衣櫃防倒扣件，這就是最務實的自救第一步！

🎨 影像生成 Prompt：
A high school student in a cozy bedroom actively securing a tall bookshelf to the wall using safety brackets, with an open laptop nearby displaying a map query system, warm evening lighting, practical realistic action --ar 16:9
-->

<!--
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
  - 土壤液化查詢主管機關：經濟部地質調查及礦業管理中心「土壤液化潛勢查詢系統」（原寫內政部地質敏感區查詢系統）
-->
