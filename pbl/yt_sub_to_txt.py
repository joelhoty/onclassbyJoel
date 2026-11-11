#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
YouTube CC 字幕提取與文字檔轉換工具 (yt_sub_to_txt.py)
======================================================
功能特色：
1. 支援各種 YouTube 網址格式 (youtu.be 短網址、watch?v=、shorts/、embed/ 或 11 碼 Video ID)。
2. 自動透過 YouTube oEmbed API 取得影片標題與頻道名稱，並自動清理為合法檔案名稱。
3. 優先選擇「手動建立的 CC 字幕 (Manually Created)」，若無則自動退回「自動產生字幕 (Generated)」。
4. 支援多格式文字檔輸出：
   - 純文字段落版 (.txt)：自動清除換行破碎感，依句號/問號合併成易讀段落。
   - 時間戳記對照版 (_timestamped.txt)：每行保留 [HH:MM:SS] 時間碼，方便回查影片。
   - 繁體中文翻譯 / 雙語對照版 (_zh-Hant.txt / _bilingual.txt)：若原片為外文 CC 字幕，可自動呼叫 YouTube 翻譯軌道轉為繁體中文，並產生中英雙語對照檔。
   - 標準字幕檔 (.srt)：可供影音播放器或剪輯軟體載入。
5. 提供三種重複使用方式：
   - 命令列直接執行：python yt_sub_to_txt.py https://youtu.be/oIk3R-sMX5o
   - 圖形化視窗介面：python yt_sub_to_txt.py --gui
   - 互動式循環模式：python yt_sub_to_txt.py (不帶參數時啟動)
"""

import argparse
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple
import requests
from youtube_transcript_api import YouTubeTranscriptApi
from youtube_transcript_api.formatters import SRTFormatter


def extract_video_id(url_or_id: str) -> str:
    """
    從各種 YouTube 網址格式提取 11 碼 Video ID。
    支援：
    - https://youtu.be/oIk3R-sMX5o
    - https://www.youtube.com/watch?v=oIk3R-sMX5o
    - https://www.youtube.com/shorts/oIk3R-sMX5o
    - 直接傳入 11 碼 ID: oIk3R-sMX5o
    """
    text = url_or_id.strip()
    pattern = r"(?:v=|\/videos\/|embed\/|youtu\.be\/|\/v\/|\/e\/|watch\?v=|&v=|\/shorts\/)([a-zA-Z0-9_-]{11})"
    match = re.search(pattern, text)
    if match:
        return match.group(1)
    # 若本身就是 11 碼 ID
    if re.fullmatch(r"[a-zA-Z0-9_-]{11}", text):
        return text
    raise ValueError(f"無法從輸入字串解析出有效的 YouTube Video ID: {url_or_id}")


def fetch_video_metadata(video_id: str) -> Dict[str, str]:
    """
    透過 YouTube 官方 oEmbed API 取得影片標題與作者名稱（免 API Key）。
    """
    watch_url = f"https://www.youtube.com/watch?v={video_id}"
    try:
        resp = requests.get(
            "https://www.youtube.com/oembed",
            params={"url": watch_url, "format": "json"},
            timeout=10,
        )
        if resp.status_code == 200:
            data = resp.json()
            return {
                "title": data.get("title", video_id),
                "author": data.get("author_name", "Unknown"),
                "url": watch_url,
            }
    except Exception:
        pass
    return {"title": video_id, "author": "Unknown", "url": watch_url}


def sanitize_filename(name: str, max_len: int = 60) -> str:
    """移除 Windows / macOS / Linux 檔名非法字元，避免存檔失敗。"""
    cleaned = re.sub(r'[\\/*?:"<>|\r\n\t]+', "_", name).strip(" ._")
    cleaned = re.sub(r"\s+", " ", cleaned)
    return cleaned[:max_len].strip() or "youtube_transcript"


def format_seconds_to_hms(seconds: float) -> str:
    """將秒數轉為 [HH:MM:SS] 或 [MM:SS] 格式字串。"""
    total_sec = int(seconds)
    hrs = total_sec // 3600
    mins = (total_sec % 3600) // 60
    secs = total_sec % 60
    if hrs > 0:
        return f"[{hrs:02d}:{mins:02d}:{secs:02d}]"
    return f"[{mins:02d}:{secs:02d}]"


def merge_snippets_to_paragraphs(entries: List[Dict], pause_threshold: float = 2.0) -> str:
    """
    將破碎的 CC 字幕片段合併為自然閱讀段落：
    - 移除片段內部多餘的強制換行
    - 當句子以句號/問號/驚嘆號結尾且累積一定長度，或兩句間隔超過 pause_threshold 秒時自動分段
    """
    paragraphs = []
    current_chunk = []
    current_len = 0
    prev_end = None

    for item in entries:
        text = re.sub(r"\s+", " ", item["text"]).strip()
        if not text:
            continue

        start = float(item.get("start", 0.0))
        duration = float(item.get("duration", 0.0))

        # 判斷是否因為長停頓而分段
        long_pause = (prev_end is not None) and ((start - prev_end) > pause_threshold)
        prev_end = start + duration

        if long_pause and current_chunk:
            paragraphs.append(" ".join(current_chunk))
            current_chunk = []
            current_len = 0

        current_chunk.append(text)
        current_len += len(text)

        # 若句尾為完整標點符號且段落已達一定長度，則換段以利閱讀
        if text.endswith((".", "?", "!", "。", "？", "！")) and current_len >= 280:
            paragraphs.append(" ".join(current_chunk))
            current_chunk = []
            current_len = 0

    if current_chunk:
        paragraphs.append(" ".join(current_chunk))

    return "\n\n".join(paragraphs)


def select_best_transcript(transcript_list, preferred_langs: List[str]):
    """
    智慧選擇最佳字幕軌道：
    1. 優先尋找 preferred_langs 中的「手動建立 CC 字幕 (Manually Created)」
    2. 其次尋找影片中任何語言的「手動建立 CC 字幕」（例如原片提供的英文手動 CC 字幕）
    3. 再次尋找 preferred_langs 中的「自動產生字幕 (Generated)」
    4. 最後回傳第一個可用的字幕軌道
    """
    # 1. 優先找指定語言的手動 CC 字幕
    try:
        return transcript_list.find_manually_created_transcript(preferred_langs)
    except Exception:
        pass

    # 2. 找任何手動建立的 CC 字幕 (例如該影片有高品質英文手動 CC)
    manual_tracks = [t for t in transcript_list if not t.is_generated]
    if manual_tracks:
        return manual_tracks[0]

    # 3. 找指定語言的自動生成字幕
    try:
        return transcript_list.find_generated_transcript(preferred_langs)
    except Exception:
        pass

    # 4. 回傳任意第一個可用軌道
    for t in transcript_list:
        return t
    raise RuntimeError("此影片沒有任何可用的字幕軌道。")


def process_youtube_to_files(
    url_or_id: str,
    output_dir: str = ".",
    preferred_langs: Optional[List[str]] = None,
    translate_to_zh: bool = False,
    export_srt: bool = True,
) -> Dict[str, Path]:
    """
    核心轉換函式：輸入 YouTube 網址，下載 CC 字幕並轉換為多種文字檔。
    回傳產生的檔案路徑字典。
    """
    if preferred_langs is None:
        preferred_langs = ["zh-TW", "zh-Hant", "zh", "en"]

    video_id = extract_video_id(url_or_id)
    meta = fetch_video_metadata(video_id)
    title = meta["title"]
    author = meta["author"]

    out_dir_path = Path(output_dir)
    out_dir_path.mkdir(parents=True, exist_ok=True)

    safe_title = sanitize_filename(title)
    base_name = f"{video_id}_{safe_title}"

    print("=" * 70)
    print(f"🎬 影片標題：{title}")
    print(f"👤 頻道作者：{author}")
    print(f"🔗 影片網址：{meta['url']} (ID: {video_id})")

    ytt_api = YouTubeTranscriptApi()
    transcript_list = ytt_api.list(video_id)

    # 選擇最佳主字幕軌道 (優先手動建立的 CC 字幕)
    primary_track = select_best_transcript(transcript_list, preferred_langs)
    track_type = "手動建立 CC 字幕" if not primary_track.is_generated else "自動產生字幕"
    print(f"🎯 選用字幕：{primary_track.language} ({primary_track.language_code}) [{track_type}]")

    fetched_primary = primary_track.fetch()
    raw_primary = fetched_primary.to_raw_data()

    saved_files: Dict[str, Path] = {}

    # 檔案 1：純文字段落版 (.txt) —— 最易讀、適合筆記與 LLM 摘要
    header_info = (
        f"影片標題：{title}\n"
        f"頻道作者：{author}\n"
        f"影片網址：{meta['url']}\n"
        f"字幕語言：{primary_track.language} ({primary_track.language_code}) [{track_type}]\n"
        f"{'=' * 60}\n\n"
    )
    paragraph_text = merge_snippets_to_paragraphs(raw_primary)
    txt_path = out_dir_path / f"{base_name}.txt"
    txt_path.write_text(header_info + paragraph_text + "\n", encoding="utf-8")
    saved_files["純文字段落檔 (.txt)"] = txt_path

    # 檔案 2：含時間戳記文字檔 (_timestamped.txt) —— 方便對照影片秒數
    ts_lines = [header_info.rstrip()]
    for item in raw_primary:
        ts_str = format_seconds_to_hms(item["start"])
        clean_line = re.sub(r"\s+", " ", item["text"]).strip()
        ts_lines.append(f"{ts_str} {clean_line}")
    ts_path = out_dir_path / f"{base_name}_timestamped.txt"
    ts_path.write_text("\n".join(ts_lines) + "\n", encoding="utf-8")
    saved_files["時間戳記文字檔 (_timestamped.txt)"] = ts_path

    # 檔案 3：若原字幕非繁體中文且支援翻譯，自動產生「繁體中文翻譯與雙語對照文字檔」
    if translate_to_zh and not primary_track.language_code.startswith("zh") and primary_track.is_translatable:
        try:
            zh_track = primary_track.translate("zh-Hant")
            fetched_zh = zh_track.fetch()
            raw_zh = fetched_zh.to_raw_data()

            # 3A: 繁體中文純文字段落版
            zh_header = (
                f"影片標題：{title}\n"
                f"頻道作者：{author}\n"
                f"影片網址：{meta['url']}\n"
                f"字幕語言：繁體中文 (由 {primary_track.language_code} {track_type} 自動翻譯)\n"
                f"{'=' * 60}\n\n"
            )
            zh_paragraphs = merge_snippets_to_paragraphs(raw_zh)
            zh_txt_path = out_dir_path / f"{base_name}_zh-Hant.txt"
            zh_txt_path.write_text(zh_header + zh_paragraphs + "\n", encoding="utf-8")
            saved_files["繁體中文段落檔 (_zh-Hant.txt)"] = zh_txt_path

            # 3B: 中英雙語時間軸對照版
            bilingual_lines = [
                f"影片標題：{title} (中英雙語 CC 字幕對照)",
                f"頻道作者：{author}",
                f"影片網址：{meta['url']}",
                "=" * 60,
                "",
            ]
            # 建立時間索引對照
            zh_by_index = {i: item["text"] for i, item in enumerate(raw_zh)}
            for idx, en_item in enumerate(raw_primary):
                ts_str = format_seconds_to_hms(en_item["start"])
                en_line = re.sub(r"\s+", " ", en_item["text"]).strip()
                zh_line = re.sub(r"\s+", " ", zh_by_index.get(idx, "")).strip()
                bilingual_lines.append(f"{ts_str} {en_line}")
                if zh_line:
                    bilingual_lines.append(f"         {zh_line}")
            bilingual_path = out_dir_path / f"{base_name}_bilingual.txt"
            bilingual_path.write_text("\n".join(bilingual_lines) + "\n", encoding="utf-8")
            saved_files["中英雙語對照檔 (_bilingual.txt)"] = bilingual_path
        except Exception as e:
            print(f"⚠️ 繁體中文翻譯軌道轉換略過：{e}")

    # 檔案 4：可選輸出標準 .srt 字幕檔
    if export_srt:
        srt_formatter = SRTFormatter()
        srt_content = srt_formatter.format_transcript(fetched_primary)
        srt_path = out_dir_path / f"{base_name}.srt"
        srt_path.write_text(srt_content, encoding="utf-8")
        saved_files["SRT 字幕檔 (.srt)"] = srt_path

    print("✅ 轉換完成！產出檔案如下：")
    for label, path in saved_files.items():
        print(f"   📄 {label}: {path.resolve()}")
    print("=" * 70)
    return saved_files


def launch_gui(default_output_dir: str = ".") -> None:
    """啟動 Tkinter 圖形化視窗介面，讓使用者可重複貼上網址一鍵轉檔。"""
    import tkinter as tk
    from tkinter import filedialog, messagebox, scrolledtext

    root = tk.Tk()
    root.title("🎬 YouTube CC 字幕轉文字檔工具 (可重複使用)")
    root.geometry("760x560")
    root.configure(padx=16, pady=16)

    # 網址輸入列
    tk.Label(root, text="YouTube 影片網址 (或 11 碼 Video ID)：", font=("Microsoft JhengHei", 11, "bold")).pack(anchor="w")
    url_frame = tk.Frame(root)
    url_frame.pack(fill="x", pady=(4, 10))

    url_var = tk.StringVar(value="https://youtu.be/oIk3R-sMX5o")
    url_entry = tk.Entry(url_frame, textvariable=url_var, font=("Consolas", 11))
    url_entry.pack(side="left", fill="x", expand=True, ipady=4)

    def clear_url():
        url_var.set("")
        url_entry.focus_set()

    tk.Button(url_frame, text="清除", command=clear_url, width=8).pack(side="left", padx=(8, 0))

    # 輸出資料夾設定
    tk.Label(root, text="儲存資料夾路徑：", font=("Microsoft JhengHei", 10, "bold")).pack(anchor="w")
    dir_frame = tk.Frame(root)
    dir_frame.pack(fill="x", pady=(4, 10))

    dir_var = tk.StringVar(value=str(Path(default_output_dir).resolve()))
    dir_entry = tk.Entry(dir_frame, textvariable=dir_var, font=("Consolas", 10))
    dir_entry.pack(side="left", fill="x", expand=True, ipady=3)

    def choose_dir():
        selected = filedialog.askdirectory(initialdir=dir_var.get())
        if selected:
            dir_var.set(selected)

    tk.Button(dir_frame, text="瀏覽...", command=choose_dir, width=8).pack(side="left", padx=(8, 0))

    # 選項勾選列
    opt_frame = tk.Frame(root)
    opt_frame.pack(fill="x", pady=(0, 12))
    zh_var = tk.BooleanVar(value=True)
    srt_var = tk.BooleanVar(value=True)
    tk.Checkbutton(opt_frame, text="若為外文影片，自動額外產生「繁體中文翻譯」與「中英雙語對照」文字檔", variable=zh_var, font=("Microsoft JhengHei", 10)).pack(anchor="w")
    tk.Checkbutton(opt_frame, text="同時匯出 .srt 時間軸字幕檔", variable=srt_var, font=("Microsoft JhengHei", 10)).pack(anchor="w")

    # 紀錄輸出視窗
    log_box = scrolledtext.ScrolledText(root, height=16, font=("Consolas", 10), bg="#0f172a", fg="#e2e8f0")
    log_box.pack(fill="both", expand=True, pady=(8, 0))

    def append_log(msg: str):
        log_box.insert("end", msg + "\n")
        log_box.see("end")
        root.update_idletasks()

    def run_conversion():
        target = url_var.get().strip()
        if not target:
            messagebox.showwarning("提示", "請先輸入 YouTube 影片網址！")
            return
        try:
            append_log(f"⏳ 開始處理：{target} ...")
            saved = process_youtube_to_files(
                url_or_id=target,
                output_dir=dir_var.get().strip() or ".",
                translate_to_zh=zh_var.get(),
                export_srt=srt_var.get(),
            )
            append_log("✅ 轉換成功！已儲存以下檔案：")
            for k, p in saved.items():
                append_log(f"   • {k}: {p.name}")
            append_log("-" * 60)
            messagebox.showinfo("轉換成功", f"已成功轉換 {len(saved)} 個文字檔至：\n{dir_var.get()}")
        except Exception as exc:
            append_log(f"❌ 發生錯誤：{exc}")
            messagebox.showerror("轉換失敗", str(exc))

    btn_run = tk.Button(
        root,
        text="🚀 取得 CC 字幕並轉換成文字檔",
        command=run_conversion,
        bg="#2563eb",
        fg="white",
        font=("Microsoft JhengHei", 11, "bold"),
        pady=6,
    )
    btn_run.pack(fill="x", pady=(4, 8))

    append_log("💡 歡迎使用 YouTube CC 字幕轉文字檔工具！貼上網址後點擊按鈕即可重複轉檔。")
    root.mainloop()


def interactive_cli_loop(output_dir: str, translate_to_zh: bool, export_srt: bool) -> None:
    """終端機互動式循環模式：可連續貼上多個網址重複轉檔，輸入 q 離開。"""
    print("\n" + "=" * 70)
    print("🔁 進入【YouTube CC 字幕轉文字檔 — 互動式重複轉檔模式】")
    print("   • 直接貼上 YouTube 網址並按 Enter 即可轉換成文字檔")
    print("   • 輸入 'gui' 可切換開啟圖形化視窗介面")
    print("   • 輸入 'q' 或 'exit' 結束程式")
    print("=" * 70)

    while True:
        try:
            user_input = input("\n👉 請輸入 YouTube 影片網址 (或輸入 q 離開): ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\n👋 感謝使用，再見！")
            break

        if not user_input:
            continue
        if user_input.lower() in {"q", "quit", "exit"}:
            print("👋 感謝使用，再見！")
            break
        if user_input.lower() == "gui":
            launch_gui(default_output_dir=output_dir)
            break

        try:
            process_youtube_to_files(
                url_or_id=user_input,
                output_dir=output_dir,
                translate_to_zh=translate_to_zh,
                export_srt=export_srt,
            )
        except Exception as exc:
            print(f"❌ 轉換失敗：{exc}")


def main():
    parser = argparse.ArgumentParser(
        description="YouTube CC 字幕取得與文字檔 (.txt / .srt) 轉換工具（支援單次、批次、互動式與 GUI 模式）"
    )
    parser.add_argument(
        "urls",
        nargs="*",
        help="一個或多個 YouTube 影片網址或 11 碼 Video ID (若不填則進入互動式重複輸入模式)",
    )
    parser.add_argument(
        "-o",
        "--output-dir",
        default=".",
        help="輸出文字檔的資料夾路徑 (預設為當前目錄)",
    )
    parser.add_argument(
        "--zh",
        action="store_true",
        help="若原字幕為外文，嘗試透過 YouTube 自動翻譯額外輸出繁體中文與雙語對照檔",
    )
    parser.add_argument(
        "--no-srt",
        action="store_true",
        help="不額外輸出 .srt 標準時間軸字幕檔",
    )
    parser.add_argument(
        "--gui",
        action="store_true",
        help="啟動 Tkinter 圖形化視窗介面",
    )

    args = parser.parse_args()

    if args.gui:
        launch_gui(default_output_dir=args.output_dir)
        return

    if args.urls:
        for url in args.urls:
            try:
                process_youtube_to_files(
                    url_or_id=url,
                    output_dir=args.output_dir,
                    translate_to_zh=args.zh,
                    export_srt=not args.no_srt,
                )
            except Exception as exc:
                print(f"❌ 處理 [{url}] 時發生錯誤：{exc}", file=sys.stderr)
    else:
        interactive_cli_loop(
            output_dir=args.output_dir,
            translate_to_zh=args.zh,
            export_srt=not args.no_srt,
        )


if __name__ == "__main__":
    main()
