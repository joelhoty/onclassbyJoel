# classroom_offtask_triage_labeler.py
# 獨立小工具：依線索粗分「娛樂分心」與「他科學術分流」
# 不依賴、不修改 classroom_cross_subject_analyzer.py

from dataclasses import dataclass


@dataclass
class Clue:
    planned: bool  # 課前就帶好他科講義／檔案？
    exam_week: bool  # 是否考試週或大截止日前？
    content: str  # "other_course" | "social" | "game" | "mixed"
    low_course_value: bool  # 學生自述／觀察：當堂低興趣低利害？


def label(clue: Clue) -> dict:
    """回傳粗分類與建議；僅供討論，非心理診斷。"""
    if clue.content in {"social", "game"} and not clue.planned:
        kind = "entertainment_distraction"
        levers = ["裝置結構化使用", "短講+活動循環", "同儕常模與延遲通知"]
    elif clue.content == "other_course" and (
        clue.planned or clue.exam_week or clue.low_course_value
    ):
        kind = "academic_triage_other_course"
        levers = [
            "提高當堂工具價值與產出責任",
            "內插／抽問提高切走成本",
            "跨課評量時程協調",
        ]
    else:
        kind = "mixed_or_unclear"
        levers = ["先低干擾近距離提示", "課後短談釐清動機", "避免一開始就道德化"]
    return {"label": kind, "suggested_levers": levers}


if __name__ == "__main__":
    demo = Clue(
        planned=True,
        exam_week=True,
        content="other_course",
        low_course_value=True,
    )
    print(label(demo))
