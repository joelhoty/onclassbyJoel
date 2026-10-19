"""EV3 跟車模擬：比較「原始四區間邏輯」與「時距 (time-gap) ACC」。
前車情境：等速 0.30 m/s -> 8~11 s 煞停 -> 靜止 -> 20 s 後重新起步。
假設：超音波雜訊 σ=1 cm、馬達一階延遲 τ=0.25 s、100% 功率 ≈ 0.5 m/s（皆為估計值，非實機量測）。
執行：python car_following_sim.py
"""
import random
random.seed(1)
DT = 0.05
MAXV = 0.50          # m/s at 100% power (EV3 medium wheels approx)

def lead_v(t):
    if t < 8: return 0.30
    if t < 11: return max(0.0, 0.30 - 0.15 * (t - 8))   # brake
    if t < 20: return 0.0
    return min(0.35, 0.10 * (t - 20))                    # restart

def original(d_cm):
    if d_cm < 20: return 0
    if d_cm < 50: return d_cm
    if d_cm < 70: return 50
    return min(100, d_cm * 1.2)

class ACC:
    def __init__(s):
        s.d0, s.h, s.kp, s.kd = 0.15, 1.0, 0.8, 1.2
        s.ema = None; s.prev = None; s.vrel = 0.0
    def step(s, d_meas, v_own):
        s.ema = d_meas if s.ema is None else 0.5 * s.ema + 0.5 * d_meas   # low-pass the raw range
        if s.prev is not None:
            raw = (s.ema - s.prev) / DT
            s.vrel = 0.8 * s.vrel + 0.2 * raw                              # filtered closing speed
        s.prev = s.ema
        d_des = s.d0 + s.h * v_own
        v_lead_est = v_own + s.vrel                  # lead speed = own speed + closing rate
        cmd = v_lead_est + s.kp * (s.ema - d_des)    # match lead speed, then correct the gap error
        return max(0.0, min(MAXV, cmd)), s.ema

def run(mode):
    x_lead, x, v = 0.8, 0.0, 0.0
    acc = ACC(); min_gap = 9; jerk = 0; last = 0; vmax = 0
    t = 0
    while t < 35:
        vl = lead_v(t); x_lead += vl * DT
        gap = x_lead - x
        meas = gap + random.gauss(0, 0.01)          # ultrasonic noise ~1 cm
        if mode == "orig":
            cmd = original(meas * 100) / 100 * MAXV
            tgt = cmd
        else:
            tgt, _ = acc.step(meas, v)
        v += (tgt - v) * DT / 0.25                  # motor first-order lag
        x += v * DT
        min_gap = min(min_gap, gap)
        jerk += abs(v - last) ; last = v
        t += DT
    return min_gap, jerk

for m in ("orig", "acc"):
    g, j = run(m)
    print(f"{m:5s} min_gap={g*100:6.1f} cm  total_speed_variation={j:.2f} m/s")
print("orig speed table (cm -> %):", [(d, round(original(d))) for d in (10, 19, 20, 35, 49, 50, 69, 70, 84, 120)])
