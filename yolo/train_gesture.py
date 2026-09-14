import os
import sys
import time
import argparse
import cv2
import yaml

try:
    from ultralytics import YOLO
except ImportError:
    YOLO = None

def collect_dataset(class_name, count=50, camera_id=0):
    """
    Collect gesture image dataset from webcam feed.
    """
    print(f"\n[資料集收集] 準備收集類別: '{class_name}' ({count} 張照片)")
    print("按 [空白鍵 (SPACE)] 拍照記錄一張")
    print("按 [Q] 結束照片收集\n")

    base_dir = os.path.join(os.path.dirname(__file__), "dataset", "images", "train", class_name)
    os.makedirs(base_dir, exist_ok=True)

    cap = cv2.VideoCapture(camera_id)
    if not cap.isOpened():
        print(f"[錯誤] 無法開啟攝影機 ID {camera_id}")
        return

    captured = 0
    while captured < count:
        ret, frame = cap.read()
        if not ret:
            continue

        frame = cv2.flip(frame, 1)
        display = frame.copy()

        # Visual info
        text = f"類別: {class_name} | 已收集: {captured}/{count} | 按 [SPACE] 拍照"
        cv2.putText(display, text, (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        cv2.imshow("Dataset Collector", display)

        key = cv2.waitKey(1) & 0xFF
        if key == 32: # SPACE
            img_name = f"{class_name}_{int(time.time()*1000)}.jpg"
            img_path = os.path.join(base_dir, img_name)
            cv2.imwrite(img_path, frame)
            captured += 1
            print(f"  [{captured}/{count}] 儲存圖片: {img_path}")
            # Flash effect
            cv2.rectangle(display, (0,0), (display.shape[1], display.shape[0]), (255,255,255), -1)
            cv2.imshow("Dataset Collector", display)
            cv2.waitKey(50)
        elif key in [ord('q'), ord('Q'), 27]:
            print("取消收集。")
            break

    cap.release()
    cv2.destroyAllWindows()
    print(f"[完成] 已成功收集 {captured} 張 '{class_name}' 照片於: {base_dir}\n")

def create_yaml(classes):
    """Generate YOLO data.yaml configuration."""
    dataset_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "dataset"))
    data = {
        "path": dataset_dir,
        "train": "images/train",
        "val": "images/val",
        "names": {i: name for i, name in enumerate(classes)}
    }
    yaml_path = os.path.join(dataset_dir, "data.yaml")
    with open(yaml_path, "w", encoding="utf-8") as f:
        yaml.dump(data, f, default_flow_style=False)
    print(f"[Yaml] 已建立配置文件: {yaml_path}")
    return yaml_path

def train_yolo(data_yaml, epochs=50, model_type="yolov8n.pt"):
    """Launch YOLO training pipeline."""
    if YOLO is None:
        print("[錯誤] 請先安裝 ultralytics: pip install ultralytics")
        return

    print(f"\n[YOLO 訓練] 開始訓練模型 (Base: {model_type}, Epochs: {epochs})...")
    model = YOLO(model_type)
    
    results = model.train(
        data=data_yaml,
        epochs=epochs,
        imgsz=640,
        batch=16,
        name="hand_gesture_yolo",
        project=os.path.join(os.path.dirname(__file__), "runs")
    )

    best_weights = os.path.join(os.path.dirname(__file__), "runs", "hand_gesture_yolo", "weights", "best.pt")
    target_dir = os.path.join(os.path.dirname(__file__), "models")
    os.makedirs(target_dir, exist_ok=True)
    target_path = os.path.join(target_dir, "best.pt")

    if os.path.exists(best_weights):
        import shutil
        shutil.copy(best_weights, target_path)
        print(f"\n[訓練成功] 最佳權重已匯出至: {target_path}")
        print(f"你可以使用以下命令執行實時辨識:")
        print(f"  python main.py --model models/best.pt")

def main():
    parser = argparse.ArgumentParser(description="YOLO Hand Gesture Dataset Collector & Custom Model Trainer")
    parser.add_argument("--collect", action="store_true", help="開啟鏡頭收集資料集照片")
    parser.add_argument("--class-name", type=str, default="custom_gesture", help="要收集的手勢類別名稱 (如: fist, palm, peace)")
    parser.add_argument("--count", type=int, default=30, help="收集的照片數量 (預設: 30)")
    parser.add_argument("--train", action="store_true", help="執行 YOLO 自訂手勢模型訓練")
    parser.add_argument("--epochs", type=int, default=30, help="訓練輪數 (預設: 30)")
    args = parser.parse_args()

    if args.collect:
        collect_dataset(args.class_name, count=args.count)

    elif args.train:
        # Detect classes in dataset/images/train/
        train_dir = os.path.join(os.path.dirname(__file__), "dataset", "images", "train")
        if not os.path.exists(train_dir):
            print(f"[錯誤] 未找到資料集目錄 {train_dir}，請先執行 --collect 收集照片。")
            return

        classes = [d for d in os.listdir(train_dir) if os.path.isdir(os.path.join(train_dir, d))]
        if not classes:
            print("[錯誤] 資料集中沒有任何類別資料夾。")
            return

        print(f"[偵測到手勢類別]: {classes}")
        yaml_path = create_yaml(classes)
        train_yolo(yaml_path, epochs=args.epochs)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
