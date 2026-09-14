import os
import sys
import time
import argparse
import cv2
from gesture_detector import HandGestureDetector

def parse_args():
    parser = argparse.ArgumentParser(description="YOLO Webcam Real-time Hand Gesture Recognition")
    parser.add_argument("--model", type=str, default="yolov8n-pose.pt", help="Path to YOLO model (.pt)")
    parser.add_argument("--camera", type=int, default=0, help="Webcam device ID (default: 0)")
    parser.add_argument("--conf", type=float, default=0.4, help="Confidence threshold (default: 0.4)")
    parser.add_argument("--width", type=int, default=1280, help="Camera frame width (default: 1280)")
    parser.add_argument("--height", type=int, default=720, help="Camera frame height (default: 720)")
    return parser.parse_args()

def main():
    args = parse_args()

    print("=" * 60)
    print("      YOLO 結合視訊鏡頭手勢辨識系統 (Hand Gesture Recognition)")
    print("=" * 60)
    print(f"[設定] 模型路徑: {args.model}")
    print(f"[設定] 鏡頭 ID: {args.camera}")
    print(f"[設定] 置信度門檻: {args.conf}")
    print("[操作說明]")
    print("  - 按 'Q' 或 'ESC': 結束程式")
    print("  - 按 'S': 儲存當前畫面截圖 (Screenshots)")
    print("  - 按 'C': 切換攝影機鏡頭 (0 -> 1 -> 0)")
    print("  - 按 'H': 開啟/關閉手部骨架繪製 (Skeleton)")
    print("  - 按 'P': 暫停/繼續視訊推論")
    print("=" * 60)

    # Initialize Detector
    detector = HandGestureDetector(model_path=args.model, conf_thresh=args.conf)

    # Initialize Camera
    cam_id = args.camera
    cap = cv2.VideoCapture(cam_id, cv2.CAP_DSHOW if sys.platform.startswith('win') else cv2.CAP_ANY)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, args.width)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, args.height)

    if not cap.isOpened():
        print(f"[警告] 無法開啟 ID {cam_id} 的攝影機！嘗試預設 CAP_ANY...")
        cap = cv2.VideoCapture(cam_id)

    if not cap.isOpened():
        print(f"[錯誤] 無法讀取任何攝影機。請檢查視訊鏡頭連接狀態！")
        return

    # Screen Capture Directory
    screenshot_dir = os.path.join(os.path.dirname(__file__), "screenshots")
    os.makedirs(screenshot_dir, exist_ok=True)

    # State flags
    show_skeleton = True
    is_paused = False
    saved_msg_timer = 0
    saved_msg = ""

    # FPS Calculation variables
    prev_time = time.time()
    fps = 0.0

    window_name = "YOLO Hand Gesture Recognition System"
    cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)
    cv2.resizeWindow(window_name, args.width, args.height)

    frame_count = 0

    try:
        while True:
            if not is_paused:
                ret, frame = cap.read()
                if not ret or frame is None:
                    print("[警告] 無法取得攝影機畫面，重新嘗試中...")
                    time.sleep(0.1)
                    continue

                # Mirror frame horizontally for intuitive camera experience
                frame = cv2.flip(frame, 1)

            # Measure FPS
            curr_time = time.time()
            dt = curr_time - prev_time
            prev_time = curr_time
            if dt > 0:
                fps = 0.9 * fps + 0.1 * (1.0 / dt)

            # Perform detection if not paused
            if not is_paused:
                detections = detector.detect(frame)
                annotated_frame = detector.draw_overlay(
                    frame,
                    detections,
                    show_skeleton=show_skeleton,
                    show_fps=True,
                    fps=fps
                )
            else:
                annotated_frame = detector.draw_chinese_text(
                    annotated_frame,
                    text="【 已暫停 PAUSED 】",
                    pos=(40, 60),
                    font_size=28,
                    color=(0, 255, 255),
                    bg_color=(0, 0, 0)
                )

            # Display screenshot notification banner if active
            if time.time() < saved_msg_timer:
                annotated_frame = detector.draw_chinese_text(
                    annotated_frame,
                    text=saved_msg,
                    pos=(30, annotated_frame.shape[0] - 50),
                    font_size=22,
                    color=(0, 255, 0),
                    bg_color=(20, 20, 20)
                )

            cv2.imshow(window_name, annotated_frame)
            key = cv2.waitKey(1) & 0xFF

            # Key Controls
            if key in [ord('q'), ord('Q'), 27]: # Q or ESC
                print("[系統] 正在退出程式...")
                break

            elif key in [ord('s'), ord('S')]: # Save snapshot
                timestamp = time.strftime("%Y%m%d_%H%M%S")
                filename = os.path.join(screenshot_dir, f"gesture_{timestamp}.jpg")
                cv2.imwrite(filename, annotated_frame)
                saved_msg = f"已截圖儲存至: {filename}"
                saved_msg_timer = time.time() + 2.5
                print(f"[截圖] {saved_msg}")

            elif key in [ord('c'), ord('C')]: # Switch Camera
                cam_id = (cam_id + 1) % 3
                print(f"[攝影機] 切換至 Camera ID: {cam_id}...")
                cap.release()
                cap = cv2.VideoCapture(cam_id)
                cap.set(cv2.CAP_PROP_FRAME_WIDTH, args.width)
                cap.set(cv2.CAP_PROP_FRAME_HEIGHT, args.height)

            elif key in [ord('h'), ord('H')]: # Toggle Skeleton
                show_skeleton = not show_skeleton
                state_str = "開啟" if show_skeleton else "關閉"
                print(f"[顯示] 骨架繪製已: {state_str}")

            elif key in [ord('p'), ord('P')]: # Pause
                is_paused = not is_paused
                print(f"[狀態] 推論暫停: {is_paused}")

    finally:
        cap.release()
        cv2.destroyAllWindows()
        print("[系統] 程式已安全結束。")

if __name__ == "__main__":
    main()
