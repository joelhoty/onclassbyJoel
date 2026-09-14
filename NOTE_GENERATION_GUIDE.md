# 📝 筆記生成與更新標準指引 (Note Generation Guidelines)

> **生成時間**: 2026-09-10 14:51:01 (UTC+8)  
> **模型代號**: Gemini 3.6 Flash  
> **適用範圍**: 本工作區 (`D:\CLASS_NOTE`) 所有筆記、教學文件與研究提案之新建與更新

---

## 📌 一、 指引三大核心規範

### 1. 自動增加時間戳記與模型代號 (Metadata Tracking)
每次新建或編輯筆記時，必須於文件最頂部（標題下方）包含標準中元資料（Metadata）：
* **生成/更新時間**: 使用 ISO 格式或 `YYYY-MM-DD HH:mm:ss (UTC+8)` 時區標記。
* **模型代號**: 明確標註當前執行的 AI 模型名稱（例如 `Gemini 3.6 Flash`）。

#### 範例格式：
```markdown
# [筆記主題名稱]

> **最後更新時間**: 2026-09-10 14:51:01 (UTC+8)
> **使用模型**: Gemini 3.6 Flash
> **文件版本**: v1.0
```

---

### 2. 關鍵字自動概念擴充機制 (Keyword Expansion Framework)
當使用者提示詞 (Prompt) 包含核心關鍵字（如特定演算法、物理定律、程式架構或專業術語）時，筆記不得僅進行簡答，必須包含以下三大擴充區塊：

1. **核心概念剖析 (Core Concepts)**：
   * 術語定義與基本原理。
   * 必要的數學公式、物理定律或邏輯架構。
2. **實例與應用範例 (Examples & Code/Applications)**：
   * 具體的程式碼範例 (Code Snippet)、數據範例或實際生活應用場景。
   * 搭配邊界條件或經典範例說明。
3. **延伸思考與進階應用 (Extensions & Advanced Topics)**：
   * 該技術/概念的優缺點與效能瓶頸。
   * 延伸相關技術（例如：從 CNN 延伸至 Transformer/Attention 機制）。
   * 未來發展或跨領域整合應用。

---

### 3. 提示詞原文整理與記錄區塊 (Prompt Archive)
每次新建或更新檔案時，必須於**文件底部**更新「提示詞歷史與變更記錄 (Prompt Archive)」區塊：
* 每次對話的提示詞原文需以區塊代碼（Code block）完整保留。
* 保留時間戳記與模型代號，便於後續追蹤提示詞演進（Prompt Lineage）。

#### 範例格式：
```markdown
---

## 📝 提示詞歷史與變更記錄 (Prompt Archive)

### 🔹 [2026-09-10 14:51:01] 初始建立
* **模型**: Gemini 3.6 Flash
* **Prompt 原文**:
  ```text
  [使用者輸入的原始 Prompt 內容]
  ```
* **變更摘要**: 建立筆記基礎架構與核心概念。

### 🔹 [2026-09-10 15:30:00] 內容擴充
* **模型**: Gemini 3.6 Flash
* **Prompt 原文**:
  ```text
  [後續增補或修改的 Prompt 內容]
  ```
* **變更摘要**: 補充實作範例與延伸思考區塊。
```

---

## 📄 二、 標準筆記 Markdown 範本 (Template)

```markdown
# 📘 [主題名稱]

> **最後更新時間**: YYYY-MM-DD HH:mm:ss (UTC+8)
> **使用模型**: [Model Name]
> **主題分類**: [AI / ec / ev3 / math / temple / dance / ...]

---

## 🎯 摘要 (Summary)
[簡述本筆記的核心內容與學習目標]

---

## 💡 1. 核心概念剖析 (Core Concepts)
### 1.1 基本原理
[術語與原理詳細說明]

### 1.2 數學/邏輯推導 (如適用)
$$ [數學公式] $$

---

## 🛠️ 2. 實作與應用範例 (Practical Examples)
### 2.1 範例說明 / 程式碼實作
```python
# [範例程式碼或步驟]
```

---

## 🚀 3. 延伸思考與進階主題 (Extensions)
* **優缺點分析**: ...
* **相關技術比較**: ...
* **進階延伸學習**: ...

---

## 📝 提示詞歷史與變更記錄 (Prompt Archive)

### 🔹 [YYYY-MM-DD HH:mm:ss] 版本記錄
* **模型**: [Model Name]
* **Prompt 原文**:
  ```text
  [Prompt 原文]
  ```
* **變更摘要**: [異動說明]
```
