#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
課堂載具紅綠燈與專注力管理看板 (Classroom Smartphone Traffic-Light & Focus Dashboard)
功能：
1. 視覺化切換「紅燈(禁用) / 黃燈(待命) / 綠燈(互動)」三種載具使用狀態，投影於教室大螢幕。
2. 內建分段教學計時器（15分鐘講述、5分鐘小組任務、60秒合法看手機喘息 Tech Break）。
3. 內建隨機座號抽籤器（保持學生適度警覺，減少低頭滑手機機率）。
4. 內建小組專注榮譽加分板，並支援匯出課堂紀錄檔 (.txt)。
"""

import ctypes
import random
import datetime
import tkinter as tk
from tkinter import messagebox, filedialog


class ClassroomFocusDashboard:
    """課堂手機管理與專注力互動看板主類別"""

    # 定義紅綠燈三種狀態的視覺與行為規範設定
    MODES = {
        "RED": {
            "title": "🔴 紅燈模式：無載具專注區 (Device-Free Zone)",
            "desc": "【行為規範】手機請關機或靜音放入「手機旅館／書包底層」，桌面保持淨空，專注聽講與思考！",
            "bg": "#dc2626",
            "panel_bg": "#fef2f2",
            "text_color": "#991b1b"
        },
        "YELLOW": {
            "title": "🟡 黃燈模式：條件式待命區 (Standby / Conditional Use)",
            "desc": "【行為規範】手機請螢幕朝下置於桌角「停車格」，僅在老師指定查資料或紀錄時拿起，用畢立即蓋回。",
            "bg": "#d97706",
            "panel_bg": "#fffbeb",
            "text_color": "#92400e"
        },
        "GREEN": {
            "title": "🟢 綠燈模式：數位互動學習區 (Active BYOD Mode)",
            "desc": "【行為規範】請拿起手機參與課堂即時互動（Kahoot! / Slido / 查資料），嚴禁切換至無關遊戲或社群軟體！",
            "bg": "#059669",
            "panel_bg": "#ecfdf5",
            "text_color": "#065f46"
        }
    }

    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("📱 課堂載具紅綠燈與專注力管理看板 (Classroom Focus Dashboard)")
        self.root.geometry("1080x720")
        self.root.configure(bg="#f8fafc")

        # 狀態變數初始化
        self.current_mode = "RED"
        self.remaining_seconds = 15 * 60  # 預設 15 分鐘分段講述
        self.timer_running = False
        self.timer_job = None
        self.group_scores = {f"第 {i} 組": 0 for i in range(1, 7)}
        self.action_logs = []

        self._build_ui()
        self.set_mode("RED")
        self._log_event("系統啟動：預設進入【紅燈模式（無載具專注區）】")
        self.root.after(100, self._force_foreground_window)

    def _force_foreground_window(self):
        """
        強制將 Tkinter 視窗顯示於最上層（解決由背景無視窗終端機啟動時，
        Windows STARTUPINFO 預設繼承 SW_HIDE 導致視窗隱藏或在背景的問題）
        """
        self.root.update_idletasks()
        self.root.deiconify()
        self.root.state("normal")
        self.root.lift()
        self.root.attributes("-topmost", True)
        self.root.focus_force()
        try:
            hwnd = ctypes.windll.user32.GetParent(self.root.winfo_id()) or self.root.winfo_id()
            # SW_SHOWNORMAL = 1, SW_RESTORE = 9
            ctypes.windll.user32.ShowWindow(hwnd, 1)
            ctypes.windll.user32.ShowWindow(hwnd, 9)
            ctypes.windll.user32.SetForegroundWindow(hwnd)
        except Exception:
            pass
        # 1.5 秒後取消強制置頂，讓使用者仍可正常切換其他視窗
        self.root.after(1500, lambda: self.root.attributes("-topmost", False))

    def _build_ui(self):
        """建立大螢幕投影最佳化的圖形介面"""
        # 1. 頂部巨型狀態橫幅 (Banner)
        self.banner_frame = tk.Frame(self.root, bg="#dc2626", pady=18, padx=20)
        self.banner_frame.pack(fill=tk.X)

        self.lbl_mode_title = tk.Label(
            self.banner_frame,
            text="",
            font=("Microsoft JhengHei", 24, "bold"),
            fg="white",
            bg="#dc2626"
        )
        self.lbl_mode_title.pack()

        self.lbl_mode_desc = tk.Label(
            self.banner_frame,
            text="",
            font=("Microsoft JhengHei", 13),
            fg="#fef2f2",
            bg="#dc2626",
            pady=6
        )
        self.lbl_mode_desc.pack()

        # 2. 主內容區（左：紅綠燈與計時器；右：抽籤與小組專注積分）
        main_frame = tk.Frame(self.root, bg="#f8fafc", padx=20, pady=15)
        main_frame.pack(fill=tk.BOTH, expand=True)

        left_col = tk.Frame(main_frame, bg="#ffffff", bd=1, relief=tk.SOLID, padx=20, pady=15)
        left_col.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 10))

        right_col = tk.Frame(main_frame, bg="#ffffff", bd=1, relief=tk.SOLID, padx=20, pady=15)
        right_col.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=(10, 0))

        # --- 左欄：紅綠燈切換按鈕區 ---
        tk.Label(
            left_col, text="🚦 1. 切換課堂載具紅綠燈狀態",
            font=("Microsoft JhengHei", 15, "bold"), bg="white", fg="#1e293b"
        ).pack(anchor="w", pady=(0, 10))

        btn_frame = tk.Frame(left_col, bg="white")
        btn_frame.pack(fill=tk.X, pady=5)

        tk.Button(
            btn_frame, text="🔴 紅燈：完全收起", font=("Microsoft JhengHei", 12, "bold"),
            bg="#ef4444", fg="white", activebackground="#dc2626", activeforeground="white",
            padx=12, pady=8, cursor="hand2", command=lambda: self.set_mode("RED")
        ).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=4)

        tk.Button(
            btn_frame, text="🟡 黃燈：桌角待命", font=("Microsoft JhengHei", 12, "bold"),
            bg="#f59e0b", fg="white", activebackground="#d97706", activeforeground="white",
            padx=12, pady=8, cursor="hand2", command=lambda: self.set_mode("YELLOW")
        ).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=4)

        tk.Button(
            btn_frame, text="🟢 綠燈：開放互動", font=("Microsoft JhengHei", 12, "bold"),
            bg="#10b981", fg="white", activebackground="#059669", activeforeground="white",
            padx=12, pady=8, cursor="hand2", command=lambda: self.set_mode("GREEN")
        ).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=4)

        # --- 左欄：課堂節奏與專注倒數計時器 ---
        tk.Label(
            left_col, text="⏱️ 2. 分段教學與科技喘息計時器 (Chunking Timer)",
            font=("Microsoft JhengHei", 15, "bold"), bg="white", fg="#1e293b"
        ).pack(anchor="w", pady=(25, 5))

        self.lbl_timer = tk.Label(
            left_col, text="15:00",
            font=("Consolas", 56, "bold"), bg="#f1f5f9", fg="#0f172a",
            pady=10, relief=tk.FLAT
        )
        self.lbl_timer.pack(fill=tk.X, pady=8)

        preset_frame = tk.Frame(left_col, bg="white")
        preset_frame.pack(fill=tk.X, pady=4)

        tk.Button(
            preset_frame, text="📖 15分 精講(紅燈)", font=("Microsoft JhengHei", 10),
            command=lambda: self.set_preset_timer(15 * 60, "RED")
        ).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=2)

        tk.Button(
            preset_frame, text="💬 5分 討論(黃燈)", font=("Microsoft JhengHei", 10),
            command=lambda: self.set_preset_timer(5 * 60, "YELLOW")
        ).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=2)

        tk.Button(
            preset_frame, text="🎮 3分 搶答(綠燈)", font=("Microsoft JhengHei", 10),
            command=lambda: self.set_preset_timer(3 * 60, "GREEN")
        ).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=2)

        tk.Button(
            preset_frame, text="☕ 60秒 Tech Break", font=("Microsoft JhengHei", 10, "bold"),
            fg="#0369a1", command=lambda: self.set_preset_timer(60, "GREEN")
        ).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=2)

        ctrl_frame = tk.Frame(left_col, bg="white")
        ctrl_frame.pack(fill=tk.X, pady=10)

        self.btn_start_pause = tk.Button(
            ctrl_frame, text="▶ 開始計時", font=("Microsoft JhengHei", 12, "bold"),
            bg="#2563eb", fg="white", pady=6, command=self.toggle_timer
        )
        self.btn_start_pause.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=4)

        tk.Button(
            ctrl_frame, text="↺ 重設為 10:00", font=("Microsoft JhengHei", 12),
            bg="#e2e8f0", fg="#334155", pady=6, command=lambda: self.set_preset_timer(600, self.current_mode)
        ).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=4)

        # --- 右欄：防分心隨機座號抽籤器 ---
        tk.Label(
            right_col, text="🎲 3. 課堂隨機互動抽籤（保持專注警覺）",
            font=("Microsoft JhengHei", 15, "bold"), bg="white", fg="#1e293b"
        ).pack(anchor="w", pady=(0, 8))

        picker_frame = tk.Frame(right_col, bg="#eff6ff", padx=12, pady=10)
        picker_frame.pack(fill=tk.X, pady=5)

        tk.Label(picker_frame, text="班級總人數：", font=("Microsoft JhengHei", 11), bg="#eff6ff").pack(side=tk.LEFT)
        self.spin_students = tk.Spinbox(picker_frame, from_=1, to=60, width=5, font=("Consolas", 12))
        self.spin_students.delete(0, tk.END)
        self.spin_students.insert(0, "35")
        self.spin_students.pack(side=tk.LEFT, padx=6)

        tk.Button(
            picker_frame, text="🎯 隨機抽一位同學回答", font=("Microsoft JhengHei", 11, "bold"),
            bg="#4f46e5", fg="white", padx=10, pady=4, command=self.pick_random_student
        ).pack(side=tk.LEFT, padx=10)

        self.lbl_picked = tk.Label(
            right_col, text="等待抽籤...",
            font=("Microsoft JhengHei", 18, "bold"), bg="#eef2ff", fg="#312e81", pady=10
        )
        self.lbl_picked.pack(fill=tk.X, pady=6)

        # --- 右欄：PBIS 小組專注榮譽加分板 ---
        tk.Label(
            right_col, text="🏆 4. PBIS 小組專注與零違規榮譽積分",
            font=("Microsoft JhengHei", 15, "bold"), bg="white", fg="#1e293b"
        ).pack(anchor="w", pady=(18, 8))

        self.score_labels = {}
        score_grid = tk.Frame(right_col, bg="white")
        score_grid.pack(fill=tk.X)

        for idx, group_name in enumerate(self.group_scores.keys()):
            row, col = divmod(idx, 2)
            cell = tk.Frame(score_grid, bg="#f8fafc", bd=1, relief=tk.RIDGE, padx=10, pady=8)
            cell.grid(row=row, column=col, sticky="ew", padx=4, pady=4)
            score_grid.grid_columnconfigure(col, weight=1)

            tk.Label(cell, text=group_name, font=("Microsoft JhengHei", 12, "bold"), bg="#f8fafc").pack(side=tk.LEFT)
            lbl_pts = tk.Label(cell, text="0 分", font=("Consolas", 14, "bold"), fg="#0d9488", bg="#f8fafc", padx=8)
            lbl_pts.pack(side=tk.LEFT)
            self.score_labels[group_name] = lbl_pts

            tk.Button(
                cell, text="+1 專注", font=("Microsoft JhengHei", 10, "bold"),
                bg="#14b8a6", fg="white", command=lambda g=group_name: self.add_score(g, 1)
            ).pack(side=tk.RIGHT, padx=2)

        # 匯出課堂紀錄按鈕
        tk.Button(
            right_col, text="💾 匯出本節課堂專注與加分紀錄 (.txt)",
            font=("Microsoft JhengHei", 11, "bold"), bg="#334155", fg="white", pady=8,
            command=self.export_log
        ).pack(fill=tk.X, pady=(18, 0))

    def set_mode(self, mode_key: str):
        """切換紅綠燈模式並更新頂部巨型橫幅顏色與說明文字"""
        self.current_mode = mode_key
        cfg = self.MODES[mode_key]
        self.banner_frame.configure(bg=cfg["bg"])
        self.lbl_mode_title.configure(text=cfg["title"], bg=cfg["bg"])
        self.lbl_mode_desc.configure(text=cfg["desc"], bg=cfg["bg"])
        self._log_event(f"切換載具狀態為：{cfg['title']}")

    def set_preset_timer(self, seconds: int, auto_mode: str):
        """設定快速教學階段計時並自動切換對應紅綠燈"""
        if self.timer_running and self.timer_job:
            self.root.after_cancel(self.timer_job)
            self.timer_running = False
            self.btn_start_pause.configure(text="▶ 開始計時", bg="#2563eb")

        self.remaining_seconds = seconds
        self._update_timer_display()
        self.set_mode(auto_mode)

    def toggle_timer(self):
        """啟動或暫停倒數計時器"""
        if self.timer_running:
            self.timer_running = False
            if self.timer_job:
                self.root.after_cancel(self.timer_job)
            self.btn_start_pause.configure(text="▶ 繼續計時", bg="#2563eb")
        else:
            self.timer_running = True
            self.btn_start_pause.configure(text="⏸ 暫停計時", bg="#dc2626")
            self._tick()

    def _tick(self):
        """每秒遞減計時器"""
        if not self.timer_running:
            return
        if self.remaining_seconds > 0:
            self.remaining_seconds -= 1
            self._update_timer_display()
            self.timer_job = self.root.after(1000, self._tick)
        else:
            self.timer_running = False
            self.btn_start_pause.configure(text="▶ 開始計時", bg="#2563eb")
            self._log_event("計時結束！提醒學生切換課堂階段。")
            messagebox.showinfo("時間到！", "本階段時間結束！請依老師指示切換載具狀態。")

    def _update_timer_display(self):
        """將剩餘秒數格式化為 MM:SS 顯示"""
        mins, secs = divmod(self.remaining_seconds, 60)
        self.lbl_timer.configure(text=f"{mins:02d}:{secs:02d}")

    def pick_random_student(self):
        """隨機抽取座號，提升課堂正向緊張感"""
        try:
            total = int(self.spin_students.get())
        except ValueError:
            total = 35
        picked = random.randint(1, max(1, total))
        self.lbl_picked.configure(text=f"🎉 抽中座號：【 {picked:02d} 號 】同學！")
        self._log_event(f"課堂隨機抽籤：抽中 {picked:02d} 號同學")

    def add_score(self, group_name: str, delta: int):
        """為專注表現良好的小組加分"""
        self.group_scores[group_name] += delta
        new_score = self.group_scores[group_name]
        self.score_labels[group_name].configure(text=f"{new_score} 分")
        self._log_event(f"PBIS 正向獎勵：{group_name} 獲得 +{delta} 分（目前 {new_score} 分）")

    def _log_event(self, message: str):
        """記錄課堂事件與時間戳記"""
        now_str = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.action_logs.append(f"[{now_str}] {message}")

    def export_log(self):
        """匯出本節課堂專注紀錄與各組得分至文字檔"""
        file_path = filedialog.asksaveasfilename(
            defaultextension=".txt",
            initialfile=f"classroom_focus_log_{datetime.datetime.now().strftime('%Y%m%d_%H%M')}.txt",
            filetypes=[("Text Files", "*.txt")]
        )
        if not file_path:
            return
        with open(file_path, "w", encoding="utf-8") as f:
            f.write("=== 課堂載具管理與小組專注積分紀錄 ===\n\n")
            f.write("【各組最終榮譽積分】\n")
            for g, s in self.group_scores.items():
                f.write(f"  - {g}: {s} 分\n")
            f.write("\n【課堂歷程紀錄】\n")
            for entry in self.action_logs:
                f.write(f"  {entry}\n")
        messagebox.showinfo("匯出成功", f"課堂紀錄已儲存至：\n{file_path}")


if __name__ == "__main__":
    root = tk.Tk()
    app = ClassroomFocusDashboard(root)
    root.mainloop()
