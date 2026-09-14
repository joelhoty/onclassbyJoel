import os
import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import torch

try:
    from ultralytics import YOLO
except ImportError:
    YOLO = None

class HandGestureDetector:
    """
    YOLO-based Hand Gesture Recognition Detector.
    Supports YOLO pose keypoint evaluation and custom YOLO gesture classification models.
    """

    # Hand Skeleton Connections for 21 Keypoint model
    HAND_CONNECTIONS = [
        # Thumb
        (0, 1), (1, 2), (2, 3), (3, 4),
        # Index
        (0, 5), (5, 6), (6, 7), (7, 8),
        # Middle
        (0, 9), (9, 10), (10, 11), (11, 12),
        # Ring
        (0, 13), (13, 14), (14, 15), (15, 16),
        # Pinky
        (0, 17), (17, 18), (18, 19), (19, 20),
        # Palm connections
        (5, 9), (9, 13), (13, 17)
    ]

    # Finger colors (BGR format)
    FINGER_COLORS = {
        "thumb": (255, 100, 100),   # Light Blue
        "index": (100, 255, 100),   # Light Green
        "middle": (100, 255, 255),  # Yellow
        "ring": (255, 150, 100),    # Orange
        "pinky": (255, 100, 255),   # Purple
        "palm": (200, 200, 200)     # Gray
    }

    def __init__(self, model_path="yolov8n-pose.pt", conf_thresh=0.4):
        self.model_path = model_path
        self.conf_thresh = conf_thresh
        self.model = None
        self.font_path = self._find_chinese_font()
        self._load_model()

    def _find_chinese_font(self):
        """Find available system Chinese font for OpenCV Pillow overlay."""
        candidates = [
            "C:\\Windows\\Fonts\\msjh.ttc",     # Microsoft JhengHei
            "C:\\Windows\\Fonts\\msjhbd.ttc",   # Microsoft JhengHei Bold
            "C:\\Windows\\Fonts\\simhei.ttf",   # SimHei
            "C:\\Windows\\Fonts\\arial.ttf"     # Fallback
        ]
        for path in candidates:
            if os.path.exists(path):
                return path
        return None

    def _load_model(self):
        """Load YOLO model."""
        if YOLO is None:
            print("[Warning] ultralytics package not installed yet.")
            return

        print(f"[YOLO] Loading model from '{self.model_path}'...")
        try:
            self.model = YOLO(self.model_path)
            print("[YOLO] Model loaded successfully.")
        except Exception as e:
            print(f"[Error] Failed to load YOLO model: {e}")
            print("[YOLO] Falling back to default 'yolov8n-pose.pt'...")
            try:
                self.model = YOLO("yolov8n-pose.pt")
            except Exception as ex:
                print(f"[Error] Fallback failed: {ex}")

    def detect(self, frame):
        """
        Process a single image frame and return gesture detections.
        
        Returns:
            detections: List of dicts with keys:
                - 'box': [x1, y1, x2, y2]
                - 'label_en': English gesture name
                - 'label_zh': Traditional Chinese gesture name with emoji
                - 'conf': confidence score
                - 'keypoints': 2D/3D keypoints numpy array or None
        """
        if self.model is None or frame is None:
            return []

        results = self.model(frame, conf=self.conf_thresh, verbose=False)
        detections = []

        if not results:
            return detections

        res = results[0]

        # Case A: Model predicts pose keypoints
        if res.keypoints is not None and len(res.keypoints) > 0:
            boxes = res.boxes.xyxy.cpu().numpy() if res.boxes is not None else []
            confs = res.boxes.conf.cpu().numpy() if res.boxes is not None else []
            kp_data = res.keypoints.data.cpu().numpy()  # shape: (N, K, 2 or 3)

            for idx, kps in enumerate(kp_data):
                box = boxes[idx] if idx < len(boxes) else [0, 0, 0, 0]
                conf = confs[idx] if idx < len(confs) else 0.8

                if len(kps) >= 21:
                    # 21 hand keypoints standard
                    gesture_en, gesture_zh = self._classify_hand_keypoints_21(kps)
                elif len(kps) == 17:
                    # Body pose keypoints (wrist/hand position heuristic)
                    gesture_en, gesture_zh = self._classify_body_pose_17(kps)
                else:
                    gesture_en, gesture_zh = "Hand Detected", "偵測到手部 ✋"

                detections.append({
                    "box": [int(b) for b in box],
                    "label_en": gesture_en,
                    "label_zh": gesture_zh,
                    "conf": float(conf),
                    "keypoints": kps
                })

        # Case B: Standard object detection or classification model
        elif res.boxes is not None and len(res.boxes) > 0:
            boxes = res.boxes.xyxy.cpu().numpy()
            confs = res.boxes.conf.cpu().numpy()
            cls_ids = res.boxes.cls.cpu().numpy().astype(int)
            names = res.names

            for idx, box in enumerate(boxes):
                cls_id = cls_ids[idx]
                class_name = names.get(cls_id, f"Class {cls_id}")
                conf = confs[idx]
                gesture_zh = self._translate_label(class_name)

                detections.append({
                    "box": [int(b) for b in box],
                    "label_en": class_name,
                    "label_zh": gesture_zh,
                    "conf": float(conf),
                    "keypoints": None
                })

        return detections

    def _classify_hand_keypoints_21(self, kps):
        """
        Evaluate 21 hand landmarks to classify gesture.
        Landmark indexing:
          0: Wrist
          1-4: Thumb
          5-8: Index
          9-12: Middle
          13-16: Ring
          17-20: Pinky
        """
        pts = kps[:, :2] # (21, 2)
        wrist = pts[0]

        # Calculate Euclidean distances from wrist to fingertips & PIP joints
        def dist(p1, p2):
            return np.linalg.norm(p1 - p2)

        # Check finger extensions
        # Index
        index_open = dist(wrist, pts[8]) > dist(wrist, pts[6]) * 1.15
        # Middle
        middle_open = dist(wrist, pts[12]) > dist(wrist, pts[10]) * 1.15
        # Ring
        ring_open = dist(wrist, pts[16]) > dist(wrist, pts[14]) * 1.15
        # Pinky
        pinky_open = dist(wrist, pts[20]) > dist(wrist, pts[18]) * 1.15
        # Thumb (distance to pinky MCP joint vs thumb MCP joint)
        pinky_mcp = pts[17]
        thumb_open = dist(pts[4], pinky_mcp) > dist(pts[2], pinky_mcp) * 1.2

        # OK Sign Check (thumb tip & index tip very close)
        palm_size = dist(wrist, pts[9])
        thumb_index_dist = dist(pts[4], pts[8])
        is_ok = (thumb_index_dist < palm_size * 0.35) and middle_open and ring_open and pinky_open

        if is_ok:
            return "OK Sign", "OK 手勢 👌"

        open_state = [thumb_open, index_open, middle_open, ring_open, pinky_open]

        # Pattern Matching
        if not any(open_state):
            return "Fist", "握拳 ✊"
        
        if all(open_state):
            return "Open Palm", "張開手掌 🖐️"

        if open_state == [False, True, True, False, False] or open_state == [True, True, True, False, False]:
            return "Victory / Peace", "勝利手勢 ✌️"

        if open_state == [False, True, False, False, False]:
            return "Pointing / One", "比 1 (指點) ☝️"

        if open_state == [False, True, True, True, False]:
            return "Three Fingers", "比 3 3️⃣"

        if open_state == [False, True, True, True, True]:
            return "Four Fingers", "比 4 4️⃣"

        if open_state == [True, True, False, False, True]:
            return "I Love You", "愛心手勢 🤟"

        # Thumb up vs down
        if open_state == [True, False, False, False, False]:
            thumb_tip_y = pts[4][1]
            thumb_mcp_y = pts[2][1]
            if thumb_tip_y < thumb_mcp_y:
                return "Thumbs Up", "豎大拇指 (讚) 👍"
            else:
                return "Thumbs Down", "拇指向下 (爛) 👎"

        # Count open fingers as fallback
        count = sum(open_state)
        return f"{count} Fingers", f"伸出 {count} 隻手指 🖐️"

    def _classify_body_pose_17(self, kps):
        """Fallback for 17 body pose keypoints (wrist/hand position)."""
        # Keypoints: 9: left wrist, 10: right wrist, 7: left elbow, 8: right elbow
        return "Hand Detected", "偵測到手部位置 ✋"

    def _translate_label(self, label):
        """Translate generic label into Chinese representation."""
        translations = {
            "fist": "握拳 ✊",
            "palm": "張開手掌 🖐️",
            "peace": "勝利手勢 ✌️",
            "ok": "OK 手勢 👌",
            "like": "豎大拇指 👍",
            "dislike": "拇指向下 👎",
            "one": "比 1 ☝️",
            "person": "人像 👤",
            "hand": "手部 ✋"
        }
        lowered = label.lower()
        for k, v in translations.items():
            if k in lowered:
                return v
        return f"手勢: {label}"

    def draw_chinese_text(self, img, text, pos, font_size=24, color=(255, 255, 255), bg_color=(0, 0, 0)):
        """Draw Chinese text cleanly on OpenCV image frame using PIL."""
        if self.font_path is None:
            # OpenCV text fallback
            cv2.putText(img, text, pos, cv2.FONT_HERSHEY_SIMPLEX, 0.8, color, 2)
            return img

        img_pil = Image.fromarray(cv2.cvtColor(img, cv2.COLOR_BGR2RGB))
        draw = ImageDraw.Draw(img_pil)

        try:
            font = ImageFont.truetype(self.font_path, font_size)
        except Exception:
            font = ImageFont.load_default()

        x, y = pos
        # Optional background box for clarity
        try:
            bbox = font.getbbox(text)
            text_w = bbox[2] - bbox[0]
            text_h = bbox[3] - bbox[1]
        except AttributeError:
            text_w, text_h = draw.textsize(text, font=font)

        padding = 4
        if bg_color is not None:
            draw.rectangle([x - padding, y - padding, x + text_w + padding, y + text_h + padding], fill=bg_color)

        draw.text((x, y), text, font=font, fill=color)

        return cv2.cvtColor(np.array(img_pil), cv2.COLOR_RGB2BGR)

    def draw_overlay(self, frame, detections, show_skeleton=True, show_fps=True, fps=0.0):
        """
        Draw rich visual HUD, bounding boxes, hand skeletons, and labels on frame.
        """
        annotated = frame.copy()
        h, w, _ = annotated.shape

        for det in detections:
            box = det["box"]
            x1, y1, x2, y2 = box
            label_zh = det["label_zh"]
            label_en = det["label_en"]
            conf = det["conf"]
            kps = det.get("keypoints")

            # 1. Draw stylish bounding box with corner highlights
            box_color = (0, 255, 128) # Cyan Green
            cv2.rectangle(annotated, (x1, y1), (x2, y2), box_color, 2)

            corner_len = 15
            # Top-Left
            cv2.line(annotated, (x1, y1), (x1 + corner_len, y1), (0, 255, 255), 3)
            cv2.line(annotated, (x1, y1), (x1, y1 + corner_len), (0, 255, 255), 3)
            # Top-Right
            cv2.line(annotated, (x2, y1), (x2 - corner_len, y1), (0, 255, 255), 3)
            cv2.line(annotated, (x2, y1), (x2, y1 + corner_len), (0, 255, 255), 3)
            # Bottom-Left
            cv2.line(annotated, (x1, y2), (x1 + corner_len, y2), (0, 255, 255), 3)
            cv2.line(annotated, (x1, y2), (x1, y2 - corner_len), (0, 255, 255), 3)
            # Bottom-Right
            cv2.line(annotated, (x2, y2), (x2 - corner_len, y2), (0, 255, 255), 3)
            cv2.line(annotated, (x2, y2), (x2, y2 - corner_len), (0, 255, 255), 3)

            # 2. Draw 21 Hand Keypoint Skeleton if available
            if show_skeleton and kps is not None and len(kps) >= 21:
                pts = kps[:, :2].astype(int)

                # Draw connection lines
                for conn in self.HAND_CONNECTIONS:
                    p1, p2 = pts[conn[0]], pts[conn[1]]
                    cv2.line(annotated, (p1[0], p1[1]), (p2[0], p2[1]), (255, 200, 100), 2, cv2.LINE_AA)

                # Draw keypoints dots
                for idx, pt in enumerate(pts):
                    # Fingertips (4, 8, 12, 16, 20) in bright yellow
                    if idx in [4, 8, 12, 16, 20]:
                        cv2.circle(annotated, (pt[0], pt[1]), 6, (0, 255, 255), -1)
                        cv2.circle(annotated, (pt[0], pt[1]), 8, (255, 255, 255), 1)
                    else:
                        cv2.circle(annotated, (pt[0], pt[1]), 4, (0, 165, 255), -1)

            # 3. Label Text Header
            display_text = f"{label_zh} ({conf:.0%})"
            text_y = max(y1 - 32, 10)
            annotated = self.draw_chinese_text(
                annotated,
                text=display_text,
                pos=(x1, text_y),
                font_size=22,
                color=(255, 255, 255),
                bg_color=(20, 20, 20)
            )

        # 4. Top Information Bar
        top_bar_height = 40
        overlay = annotated.copy()
        cv2.rectangle(overlay, (0, 0), (w, top_bar_height), (15, 15, 15), -1)
        cv2.addWeighted(overlay, 0.7, annotated, 0.3, 0, annotated)

        info_text = f"YOLO 手勢辨識系統 | FPS: {fps:.1f} | 檢測數量: {len(detections)}"
        annotated = self.draw_chinese_text(
            annotated,
            text=info_text,
            pos=(15, 8),
            font_size=20,
            color=(0, 255, 200),
            bg_color=None
        )

        return annotated
