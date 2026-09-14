# 📝 Antigravity Workspace Rule: Note Generation Guidelines

When generating or updating any notes or documentation files in this workspace (`D:\CLASS_NOTE`), ALWAYS adhere strictly to the following three core rules:

## 1. Automatic Timestamp and Model Identifier
At the very top of the note (below the title), always include:
- `最後更新時間`: Current time in `YYYY-MM-DD HH:mm:ss (UTC+8)` format.
- `使用模型`: Model name (e.g. `Gemini 3.6 Flash`).

## 2. Keyword Concept Expansion
When the user's prompt mentions key terms or concepts (e.g., algorithms, physics laws, software frameworks, specific techniques), expand the note to cover:
- **Core Concepts**: Definitions, underlying mechanisms, formulas, and fundamental principles.
- **Practical Examples**: Real-world use cases, code snippets, or worked numeric examples.
- **Extensions**: Performance tradeoffs, related advanced concepts, edge cases, and future directions.

## 3. Prompt Archive Logging
At the bottom of every note file, maintain a `## 📝 提示詞歷史與變更記錄 (Prompt Archive)` section.
- Append a timestamped entry for every create or update action.
- Log the exact verbatim user prompt inside a fenced code block (`text`).
- Summarize the change made.
