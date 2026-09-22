# 🤖 AGENTS.md — Multi-Agent Workspace Rule & Note Generation Guidelines

> **適用對象**: 所有在此工作區 (`D:\CLASS_NOTE`) 運作的 AI Agent / Subagent  
> **最後更新時間**: 2026-09-22 15:12:00 (UTC+8)  
> **規範目標**: 建立跨 Agent 統一的筆記生成、更新與提示詞追蹤標準

---

## 📋 跨 Agent 共用筆記生成三大核心規範

在 `D:\CLASS_NOTE` 工作區內建立或修改任何筆記、研究文件與教學檔案時，**所有 Agent 必須嚴格執行以下三項規範**：

### 1. 自動包含標準中元資料 (Metadata Header)
文件最上方（主標題下方）必須包含統一格式的 Metadata，明確標註編輯時間與當前執行的模型/Agent 代號。

```markdown
> **最後更新時間**: YYYY-MM-DD HH:mm:ss (UTC+8)
> **使用模型**: [Model Name, e.g. Gemini 3.6 Flash]
> **執行 Agent**: [Agent/Subagent Name]
```

---

### 2. 關鍵字自動擴充機制 (Keyword Expansion Framework)
當 Prompt 包含核心關鍵字（演算法、原理、技術專有名詞或主題）時，不可僅作字面回答，必須展開為三個層次：

1. **核心概念剖析 (Core Concepts)**：定義、底層原理、關鍵公式或定理。
2. **實作與應用範例 (Practical Examples)**：實際程式碼範例、數據範例或真實應用場景。
3. **延伸思考與進階主題 (Extensions & Advanced Topics)**：優缺點比較、邊界條件、性能權衡及未來延伸發展。

---

### 3. 累加式提示詞紀錄區塊 (Prompt Archive Logging)
每次編輯或新建檔案時，必須於文件最底部維護 `## 📝 提示詞歷史與變更記錄 (Prompt Archive)` 區塊：

* **不得覆蓋**先前 Agent 所記錄的歷史資訊，必須使用**增補（Append）**方式新增紀錄。
* 提示詞原文必須完整記錄於 `text` 程式碼區塊內。

```markdown
---


### 4.預設儲存格式
1. 每次預設都存md(markdown)與html格式
2. html須加上匯出成pdf按鈕
3. 內容更新時須同時更新md檔與html檔案

### 5.內容
1. 如果內容包含程式碼，使用程式碼區塊，並加上複製按鈕
2. 程式碼包含註解
3. 在程式碼前，增加以流程圖解說程式邏輯

## 📝 提示詞歷史與變更記錄 (Prompt Archive)

### 🔹 [YYYY-MM-DD HH:mm:ss] [Agent/Model Name] 變更紀錄
- **模型/Agent**: [Model Name / Subagent Role]
- **Prompt 原文**:
  ```text
  [使用者或上游 Agent 傳遞的原始 Prompt 內容]
  ```
- **變更摘要**: [簡述本次建立或增修的內容]
```

---

### 6.index.md與index.html 自動更新
- 在git commit 之前先進行index檔案索引更新
- git commit 與 git push 後檢查檔案索引是否正確
---

## 📁 跨 Agent 目錄分類原則 (Subject Categorization)

所有新增筆記必須依主題分類歸檔至對應資料夾，不得任意放置於根目錄：
* `AI/`：生成式 AI、機器學習、深度學習、電腦視覺、論文與研究提案
* `ec/`：電子電路、歐姆定律、LED 等硬體實驗與計算筆記
* `ev3/`：樂高 (LEGO EV3 / SPIKE) 機器人與尋跡程式
* `math/`：數學、機率與統計分布筆記
* `temple/`：歷史廟宇、GIS 地圖導覽與地方文化統計報告
* `dance/`：舞蹈美學與藝術研究筆記
