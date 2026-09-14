"""
LEGO SPIKE Prime MicroPython 圓圈尋跡程式 (Line Following on Circular Track)
適用環境：LEGO Education SPIKE App (MicroPython)
"""

from spike import PrimeHub, ColorSensor, MotorPair, Port
import time

# ==========================================
# 1. 硬體初始化 (Hardware Initialization)
# ==========================================
hub = PrimeHub()

# 請根據實際機器人接線修改 Port 埠口
# 假設光感應器接在 Port E，左右輪馬達分別接在 Port A 與 Port B
color_sensor = ColorSensor(Port.E)
wheels = MotorPair(Port.A, Port.B)

# ==========================================
# 2. 參數校正與設定 (Calibration & Parameters)
# ==========================================
# 請先實測現場地面的反射光強度 (Reflected Light Intensity)
BLACK_LIGHT = 10  # 黑色線條反射光讀值 (%)
WHITE_LIGHT = 70  # 白色地面反射光讀值 (%)

# 計算黑白邊界門檻值 (Threshold)
THRESHOLD = (BLACK_LIGHT + WHITE_LIGHT) / 2  # 預設為 40%

# 比例控制 P-Controller 參數
TARGET_LIGHT = THRESHOLD  # 目標黑白邊界讀值
KP = 0.8                  # 比例增益 (Kp 越大轉向越敏捷，過大會震盪)
BASE_SPEED = 35           # 巡航基準速度 (%)


# ==========================================
# 模式 A：兩態切換尋跡 (On-Off / Switch Line Follower)
# 適合初學者，原理簡單直觀
# ==========================================
def run_two_state_line_follower():
    print("開始執行：兩態切換尋跡模式...")
    BASE_POWER = 30
    TURN_POWER = 10
    
    while True:
        # 讀取當前反射光強度 (0 ~ 100)
        light_value = color_sensor.get_reflected_light()
        
        if light_value < THRESHOLD:
            # 壓在黑色線上 (偏向圓內)：左輪快、右輪慢 -> 往圓外(白區)修正
            wheels.start_tank(BASE_POWER, TURN_POWER)
        else:
            # 壓在白色地面 (偏向圓外)：左輪慢、右輪快 -> 往圓內(黑區)修正
            wheels.start_tank(TURN_POWER, BASE_POWER)


# ==========================================
# 模式 B：比例控制尋跡 (P-Control Line Follower)
# 適合圓圈軌跡，行駛平滑不蛇行抖動 (推薦)
# ==========================================
def run_p_control_line_follower():
    print("開始執行：比例控制 (P-Control) 尋跡模式...")
    
    while True:
        # 讀取當前反射光強度
        current_light = color_sensor.get_reflected_light()
        
        # 1. 計算偏離目標門檻值的誤差 (Error)
        error = current_light - TARGET_LIGHT
        
        # 2. 計算轉向修正量 (Turn)
        turn = error * KP
        
        # 3. 計算左右馬達個別功率
        left_power = int(BASE_SPEED + turn)
        right_power = int(BASE_SPEED - turn)
        
        # 4. 安全限制功率在 -100 ~ 100 之間
        left_power = max(-100, min(100, left_power))
        right_power = max(-100, min(100, right_power))
        
        # 5. 輸出給驅動馬達
        wheels.start_tank(left_power, right_power)


# ==========================================
# 主程式進入點 (Main Execution)
# ==========================================
if __name__ == "__main__":
    # 提示音效：準備就緒
    hub.speaker.beep(60, 0.2)
    
    # 預設執行平滑的「比例控制尋跡」 (若要改用兩態切換，請取消下一行的註解)
    # run_two_state_line_follower()
    
    run_p_control_line_follower()
