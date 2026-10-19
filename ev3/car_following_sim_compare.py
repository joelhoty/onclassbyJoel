"""EV3 跟車模擬比較：第一寫法、第四寫法、單一 clamp 公式、建議公式（含停車線與起步遲滯）。
前車情境：等速 0.30 m/s -> 8~11 s 煞停 -> 靜止 -> 20 s 後重新起步。
假設：超音波雜訊 σ=1 cm、馬達一階延遲 τ=0.25 s、100% 功率 ≈ 0.5 m/s（皆為估計值，非實機量測）。
執行：python car_following_sim_compare.py
"""
import random
random.seed(1)
DT=0.05; MAXV=0.50
def lead_v(t):
    if t<8: return 0.30
    if t<11: return max(0.0,0.30-0.15*(t-8))
    if t<20: return 0.0
    return min(0.35,0.10*(t-20))
clamp=lambda x,lo,hi:max(lo,min(hi,x))
def first(d):  # 1019-1 four zones
    return 0 if d<20 else d if d<50 else 50 if d<70 else min(100,1.2*d)
def fourth(d):
    if d<10: return 0
    if d<50: return 0.5*(d-35)
    if d<70: return 50
    return min(100,d)
def single(d): return clamp(1.5*(d-10),0,60)
D_STOP,D_REST,K,PMIN,PMAX=15,20,1.3,15,60
class Rec:
    stopped=True
    def __call__(s,d):
        sp=clamp(PMIN+K*(d-D_STOP),PMIN,PMAX)
        if d<D_STOP: sp=0; s.stopped=True
        elif s.stopped and d<D_REST: sp=0
        else: s.stopped=False
        return sp
def run(f):
    x_lead,x,v=0.8,0.0,0.0; mg=9; var=0; last=0; t=0; maxback=0
    while t<35:
        x_lead+=lead_v(t)*DT
        gap=x_lead-x
        d=(gap+random.gauss(0,0.01))*100
        p=f(d)
        tgt=p/100*MAXV
        v+=(tgt-v)*DT/0.25
        x+=v*DT; mg=min(mg,gap); var+=abs(v-last); last=v; maxback=min(maxback,v); t+=DT
    return mg*100,var,maxback
for name,f in (("1 first",first),("4 fourth",fourth),("single clamp",single),("recommended",Rec())):
    mg,var,mb=run(f)
    print(f"{name:13s} min_gap={mg:6.1f} cm  speed_variation={var:5.2f} m/s  max_reverse={mb:6.3f} m/s")
r=Rec()
print([(d,round(r(d),2)) for d in (5,10,12,20,30,40,50,80)])
