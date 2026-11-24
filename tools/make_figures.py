"""Vẽ các hình minh họa cho Hướng dẫn và Báo cáo -> tai_lieu/hinh/*.png
Chạy: python tools/make_figures.py   (cần matplotlib)
"""
import csv
import math
import os
import re
import shutil

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from matplotlib.patches import Circle, FancyArrowPatch, FancyBboxPatch, Rectangle

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "tai_lieu", "hinh")
os.makedirs(OUT, exist_ok=True)
plt.rcParams["font.family"] = "Arial"
NAVY, BLUE, LB, GR, GRN, RED, GREY, YEL = "#1F3A5F", "#2E75B6", "#DCE9F5", "#E8F3E8", "#2E7D32", "#C62828", "#F2F2F2", "#FFF2CC"


def box(ax, x, y, w, h, text, fc=LB, fs=8.5, bold=False, tc="black"):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.02,rounding_size=0.8", fc=fc, ec=NAVY, lw=1.3))
    if text:
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=fs, fontweight="bold" if bold else "normal", color=tc)


def arr(ax, x1, y1, x2, y2, text="", c=NAVY, fs=7.5, both=False, rad=0):
    ax.add_patch(FancyArrowPatch((x1, y1), (x2, y2), arrowstyle="<|-|>" if both else "-|>", mutation_scale=11, color=c, lw=1.2,
                                 connectionstyle=f"arc3,rad={rad}"))
    if text:
        ax.text((x1 + x2) / 2, (y1 + y2) / 2, text, fontsize=fs, ha="center", va="center", bbox=dict(fc="white", ec="none", pad=0.6))


def canvas(w, h, xl, yl):
    fig, ax = plt.subplots(figsize=(w, h))
    ax.set_xlim(0, xl); ax.set_ylim(0, yl); ax.axis("off")
    return fig, ax


def save(fig, name):
    fig.savefig(os.path.join(OUT, name), dpi=170, bbox_inches="tight", facecolor="white")
    plt.close(fig)


# ------------------------------------------------------------------ 1. sơ đồ khối phần cứng
fig, ax = canvas(10.5, 7, 105, 70)
box(ax, 40, 26, 25, 18, "STM32F103C8T6\nBlue Pill 72 MHz\n64 KB Flash · 20 KB RAM\nLED PC13 · SWD (ST-Link)", fc="#FFE699", bold=True, fs=8.5)
box(ax, 2, 55, 28, 12, "5 mắt TCRT5000 (bán nguyệt)\nL2 PB11 · L1 PB12 · M PB13\nR1 PB14 · R2 PB15", fs=8)
box(ax, 38, 56, 29, 11, "2 × E18-D80NK (2 bên hông)\nTrái PA8 · Phải PA10\nHIGH = có vật", fs=8)
box(ax, 75, 56, 28, 11, "TCS3200 (bên trái)\nS2 PB0 · S3 PB1 · OUT PB10\nS0 = 5 V, S1 = GND (20%)", fs=8)
box(ax, 75, 34, 28, 11, "TCS34725 (bên phải)\nI2C1: SCL PB6 · SDA PB7\nđịa chỉ 0x29 · LED cam", fs=8)
box(ax, 75, 12, 28, 12, "Còi 5 V qua NPN S8050\nPA1 → R 1 kΩ → B\ndiode 1N4148 chống ngược", fs=8)
box(ax, 2, 30, 28, 14, "L298N (cầu H kép)\nENA PA11 · IN1/IN2 PA5/PA4\nENB PA9 · IN3/IN4 PA3/PA2", fs=8)
box(ax, 2, 6, 28, 12, "4 động cơ DC\n2 trái song song\n2 phải song song", fc=GREY, fs=8)
box(ax, 38, 2, 29, 16, "Pin 3S 18650 (11,1–12,6 V)\n→ SW1 → cầu chì 2 A → +12 V\n→ LM2596 → +5 V (tụ 100–220 µF)", fc="#F8CBAD", fs=8)
arr(ax, 16, 55, 40, 42, "5 tín hiệu số")
arr(ax, 52, 56, 52, 44, "2 tín hiệu số")
arr(ax, 75, 60, 65, 42, "tần số + chọn lọc", both=True)
arr(ax, 75, 39, 65, 36, "I2C", both=True)
arr(ax, 65, 29, 75, 19, "GPIO")
arr(ax, 40, 35, 30, 37, "PWM + chiều")
arr(ax, 16, 30, 16, 18, "công suất")
arr(ax, 40, 14, 30, 33, "+12 V", c=RED)
arr(ax, 53, 18, 53, 26, "+5 V / 3,3 V", c=RED)
ax.text(52.5, 69.5, "Sơ đồ khối phần cứng", ha="center", fontsize=12, fontweight="bold", color=NAVY)
save(fig, "so_do_khoi.png")

# ------------------------------------------------------------------ sân
POS = {"A": (0, 300), "B": (150, 300), "C": (300, 300), "D": (0, 150), "E": (150, 150), "F": (300, 150),
       "G": (0, 0), "H": (150, 0), "I": (300, 0), "V": (150, 345), "S": (150, -45)}
EDGES = ["AB", "BC", "AD", "BE", "CF", "DE", "EF", "DG", "EH", "FI", "GH", "HI"]


def field(ax, numbers=True, flares=True):
    for i, e in enumerate(EDGES):
        (x1, y1), (x2, y2) = POS[e[0]], POS[e[1]]
        ax.plot([x1, x2], [y1, y2], color="black", lw=5, solid_capstyle="round", zorder=1)
        if numbers:
            ax.text((x1 + x2) / 2 + (12 if x1 == x2 else 0), (y1 + y2) / 2 + (12 if y1 == y2 else 0), f"c{i + 1}", fontsize=9, color=BLUE, fontweight="bold", zorder=5)
    for s in ["BV", "HS"]:
        (x1, y1), (x2, y2) = POS[s[0]], POS[s[1]]
        ax.plot([x1, x2], [y1, y2], color="black", lw=5, zorder=1)
    if flares:
        for n in "BEH":
            nx, ny = POS[n]
            for sx in (-1, 1):
                for sy in (-1, 1):
                    cx, cy = nx + sx * 15, ny + sy * 15
                    a1 = math.atan2(0 - sy * 15 + 0, -sx * 15) if False else math.atan2((ny + sy * 15) - cy, nx - cx)
                    a2 = math.atan2(ny - cy, (nx + sx * 15) - cx)
                    d = (a2 - a1 + math.pi) % (2 * math.pi) - math.pi
                    pts = [(cx + 15 * math.cos(a1 + d * k / 20), cy + 15 * math.sin(a1 + d * k / 20)) for k in range(21)]
                    ax.plot([q[0] for q in pts], [q[1] for q in pts], color="black", lw=2.5, zorder=1)
    for n, (x, y) in POS.items():
        if n in "VS":
            continue
        ax.add_patch(Circle((x, y), 6.5, fc="white", ec=NAVY, lw=1.4, zorder=3))
        ax.text(x + (-14 if x == 0 else 14), y + 12, n, ha="center", va="center", fontsize=10, fontweight="bold", color=NAVY, zorder=4)
    ax.text(150, 360, "VÒNG 1 (ngõ cụt)", ha="center", fontsize=8.5, fontweight="bold")
    ax.text(150, -66, "XUẤT PHÁT", ha="center", fontsize=9, fontweight="bold", color="white", bbox=dict(fc=GRN, ec="none"))
    ax.text(-40, 332, "ĐÍCH", ha="center", fontsize=9, fontweight="bold", color="white", bbox=dict(fc=RED, ec="none"))
    ax.set_xlim(-90, 390); ax.set_ylim(-90, 385); ax.set_aspect("equal"); ax.axis("off")


fig, ax = plt.subplots(figsize=(7, 7)); field(ax)
ax.set_title("Sân Vòng 1 dưới dạng đồ thị: 9 nút, 12 cung × 150 cm\n(B, E, H là ngã tư có loa bo)", fontsize=11, fontweight="bold", color=NAVY)
save(fig, "ban_do_san.png")

ROUTE = ["S", "H", "E", "B", "A", "D", "E", "F", "C", "B", "E", "F", "I", "H", "G", "D", "A"]
fig, ax = plt.subplots(figsize=(7, 7)); field(ax, numbers=False, flares=False)
off = {}
for i in range(len(ROUTE) - 1):
    a, b = ROUTE[i], ROUTE[i + 1]
    (x1, y1), (x2, y2) = POS[a], POS[b]
    k = "".join(sorted(a + b)); o = off.get(k, 0); off[k] = o + 1
    sh = -10 if o == 0 else 10
    if x1 == x2: x1 += sh; x2 += sh
    else: y1 += sh; y2 += sh
    c = cm.viridis(i / (len(ROUTE) - 1))
    ax.annotate("", xy=(x2 - (x2 - x1) * .13, y2 - (y2 - y1) * .13), xytext=(x1 + (x2 - x1) * .13, y1 + (y2 - y1) * .13),
                arrowprops=dict(arrowstyle="-|>", color=c, lw=2.2), zorder=6)
    lx = (x1 + x2) / 2 + (13 * (1 if sh > 0 else -1) if x1 == x2 else 0)
    ly = (y1 + y2) / 2 + (0 if x1 == x2 else 13 * (1 if sh > 0 else -1))
    ax.text(lx, ly, str(i + 1), fontsize=8, fontweight="bold", ha="center", va="center", color="white", bbox=dict(boxstyle="circle,pad=0.2", fc=c, ec="none"), zorder=7)
ax.set_title("Lộ trình phủ 12/12 cung (16 đoạn ≈ 23 m), kết thúc tại ĐÍCH", fontsize=11, fontweight="bold", color=NAVY)
save(fig, "lo_trinh_phu.png")

# ------------------------------------------------------------------ kiến trúc phần mềm
fig, ax = canvas(10.5, 6.8, 105, 68)
layers = [("main.cpp – chọn chế độ RUN / CALIB / TEST / XÓA BỘ NHỚ", "đọc E18 khi khởi động · watchdog 500 ms · vòng lặp không delay", NAVY, "white"),
          ("Robot (robot.cpp) – máy trạng thái hành vi", "Ready · Follow · NodeScan · Turn · ScanObject · AlertOn · Grip · Recover · Goal · Calib", BLUE, "white"),
          ("Điều hướng – field_map.cpp", "đồ thị sân · lộ trình phủ tối ưu · BFS về đích · lộ trình tham lam sau restart", BLUE, "white"),
          ("Nhận thức – line_pos · node_detector · color · object_tracker", "vị trí line · phân loại nút · nhận màu đỏ · khử báo lặp theo vị trí", LB, "black"),
          ("Điều khiển & tiện ích – pid · alert · gripper · backup_codec", "PID bám line · hẹn giờ còi · servo gắp · đóng gói bộ nhớ Backup", LB, "black"),
          ("Lớp trừu tượng phần cứng core::Hal (hal.h)", "millis · readLine · obstacle · readColor · setMotors · buzzer · led · log · backup · calib · servo", GR, "black")]
y = 58
for t, d, fc, tc in layers:
    box(ax, 2, y, 66, 8, "", fc=fc)
    ax.text(4, y + 5.2, t, fontsize=9, fontweight="bold", color=tc)
    ax.text(4, y + 1.8, d, fontsize=7.3, color=tc)
    y -= 9.6
box(ax, 72, 34, 31, 18, "ArduinoHal (xe thật)\nSTM32duino: GPIO, PWM,\npulseIn, Wire I2C,\nBackup, Flash, Servo", fc="#FFE699", fs=8)
box(ax, 72, 8, 31, 20, "SimHal (máy tính)\nmô hình sân + động học xe\n+ 5 mắt + E18 + màu\n→ chạy nguyên lượt thi\n+ 50 unit test", fc=YEL, fs=8)
arr(ax, 68, 12, 72, 40)
arr(ax, 68, 12, 72, 18)
ax.text(52, 67.5, "Kiến trúc firmware: cùng một lõi thuật toán chạy trên xe và trên máy tính", ha="center", fontsize=11, fontweight="bold", color=NAVY)
save(fig, "kien_truc.png")

# ------------------------------------------------------------------ FSM
fig, ax = canvas(11.8, 7.2, 118, 72)
S = {"BOOT\n(main.cpp)": (2, 60, 20, 7), "Ready": (34, 60, 20, 7), "Calib": (2, 38, 20, 7), "Follow": (34, 36, 24, 9),
     "NodeScan": (68, 60, 22, 7), "Goal": (96, 60, 20, 7), "Turn": (68, 38, 22, 7),
     "ScanObject": (6, 18, 22, 7), "AlertOn": (6, 2, 22, 7), "Recover": (64, 14, 20, 7), "Grip": (38, 1, 18, 7)}
col = {"AlertOn": "#F8CBAD", "Goal": "#C5E0B4", "Recover": "#FFE699", "Follow": "#BDD7EE", "Grip": "#E2D3F0"}
for k, (x, y, w, h) in S.items():
    box(ax, x, y, w, h, k, fc=col.get(k, LB), fs=8.5, bold=True)


def L(x, y, t):
    ax.text(x, y, t, fontsize=7.2, ha="center", va="center", bbox=dict(fc="white", ec="none", pad=0.5))


arr(ax, 22, 63.5, 34, 63.5); L(28, 66, "chế độ RUN")
arr(ax, 10, 60, 10, 45); L(10, 52.5, "che E18 trái")
arr(ax, 46, 60, 46, 45); L(46, 52.5, "sau 1 giây")
arr(ax, 56, 45, 72, 60); L(62, 54, "mắt ngoài + giữa ≥ 3 mẫu")
arr(ax, 80, 60, 80, 45); L(80, 52.5, "tra lộ trình")
arr(ax, 68, 41.5, 58, 41.5); L(63, 44, "thấy line mới")
arr(ax, 90, 63.5, 96, 63.5); L(103, 69, "Stop = ĐÍCH / hết giờ")
arr(ax, 36, 36, 20, 25); L(22, 32, "E18 kích")
arr(ax, 26, 25, 40, 36); L(38, 29, "không đỏ / đã báo")
arr(ax, 14, 18, 14, 9); L(14, 13.5, "≥ 4 mẫu đỏ, vật mới")
arr(ax, 28, 5, 38, 5); L(33, 7.5, "Vòng 2")
arr(ax, 24, 9, 39, 36); L(29, 16, "2,2 s")
arr(ax, 48, 8, 48, 36); L(51, 22, "gắp xong")
arr(ax, 56, 36, 68, 21); L(64, 30, "mất line")
arr(ax, 74, 21, 58, 38); L(73, 27, "thấy line")
ax.text(100, 24, "Mọi trạng thái có thời hạn:\nquay quá 1,8 s → Recover\nkhông tìm lại line 3 s → quay đầu,\nchuyển chế độ dự phòng\nquá 4:00 → đường ngắn nhất về A\nquá 5:00 → dừng", ha="center", fontsize=7.5, style="italic", color="#444",
        bbox=dict(fc="#FAFAFA", ec="#CCC"))
ax.text(59, 71.5, "Máy trạng thái hành vi của xe", ha="center", fontsize=12, fontweight="bold", color=NAVY)
save(fig, "fsm.png")

# ------------------------------------------------------------------ quét màu
fig, ax = canvas(11, 5.6, 110, 56)
steps = [(1, 40, "E18 bên trái/phải\nHIGH ≥ 3 mẫu\n(vật mới: E18 đã về LOW\nhoặc đi > 20 cm)"), (23, 40, "Dừng 30 ms\nrồi BÒ CHẬM (PWM 75)\nqua vật tối đa 8 cm"),
         (45, 40, "Mỗi 35 ms đọc 1 mẫu\nTCS3200: 3 chu kỳ xung\nTCS34725: 4 kênh I2C"), (67, 40, "Phân loại mẫu\nngưỡng đã thử thật\nhoặc tâm màu (CALIB)"),
         (89, 40, "Đủ 4 mẫu ĐỎ?\nhoặc hết 8 cm /\nE18 về LOW → thôi")]
for x, y, t in steps:
    box(ax, x, y, 20, 13, t, fs=7.6)
for i in range(4):
    arr(ax, steps[i][0] + 20, 46.5, steps[i + 1][0], 46.5)
box(ax, 84, 18, 25, 12, "Khử lặp: khóa = (cung,\nbên của CUNG, vị trí s)\nđã báo trong ±16 cm?", fc=YEL, fs=7.8)
arr(ax, 99, 40, 97, 30, "đỏ")
box(ax, 45, 18, 28, 12, "CẢNH BÁO 2,2 s\ncòi PA1 + LED PC13\nxe đứng yên", fc="#F8CBAD", fs=8.5, bold=True)
arr(ax, 84, 24, 73, 24, "vật mới")
box(ax, 8, 18, 28, 12, "Ghi vào bộ nhớ Backup\n(nhớ qua nút RESET)\n→ Vòng 2: gắp", fc=GR, fs=8)
arr(ax, 45, 24, 36, 24)
box(ax, 84, 2, 25, 10, "Đi tiếp (Follow)", fc=GREY, fs=8)
arr(ax, 96, 18, 96, 12, "đã báo")
ax.text(55, 55.5, "Quy trình nhận diện vật đỏ: quét trượt + bỏ phiếu + khử lặp theo vị trí", ha="center", fontsize=11, fontweight="bold", color=NAVY)
save(fig, "quet_mau.png")

# ------------------------------------------------------------------ PID
fig, ax = canvas(11, 4.2, 110, 42)
ax.add_patch(Circle((14, 22), 3.5, fc="white", ec=NAVY, lw=1.4)); ax.text(14, 22, "Σ", ha="center", va="center", fontsize=12)
ax.text(0.5, 26.5, "mục tiêu = 0\n(line ở giữa)", fontsize=8); arr(ax, 5, 22, 10.5, 22)
box(ax, 24, 15, 20, 14, "PID\nKp·e + Kd·de/dt (+ Ki)\nchu kỳ 4 ms", fs=8)
arr(ax, 17.5, 22, 24, 22, "e")
box(ax, 50, 15, 18, 14, "Trộn vi sai\ntrái = V + u\nphải = V − u", fs=8)
arr(ax, 44, 22, 50, 22, "u")
box(ax, 74, 15, 15, 14, "L298N +\n4 động cơ", fc=GREY, fs=8)
arr(ax, 68, 22, 74, 22, "PWM")
box(ax, 92, 15, 17, 14, "5 mắt line\nvị trí x (cm)\n−4,33 … +4,33", fc=GR, fs=8)
arr(ax, 89, 22, 92, 22)
ax.plot([100, 100, 14], [15, 6, 6], color=NAVY, lw=1.2); arr(ax, 14, 6, 14, 18.5)
ax.text(57, 2.5, "x = trung bình tọa độ các mắt thấy đen; gần nút chỉ dùng 3 mắt giữa; mất line → giữ hướng lần cuối (±6 cm)", ha="center", fontsize=7.8)
box(ax, 50, 33, 18, 7, "V: 140 / 110 gần nút", fc=YEL, fs=7.8); arr(ax, 59, 33, 59, 29)
ax.text(55, 41.5, "Vòng điều khiển PID bám line", ha="center", fontsize=11, fontweight="bold", color=NAVY)
save(fig, "pid.png")

# ------------------------------------------------------------------ mắt bán nguyệt
fig, ax = plt.subplots(figsize=(9, 4.6))
ax.set_aspect("equal"); ax.axis("off")
ax.add_patch(Rectangle((-14, -9), 22, 18, fc="#EEE", ec="#999"))
ax.text(-3, 0, "thân xe", ha="center", va="center", fontsize=9, color="#666")
ax.add_patch(Rectangle((-4, -10.5), 8, 2, fc="#555")); ax.add_patch(Rectangle((-4, 8.5), 8, 2, fc="#555"))
ax.plot([0, 0], [-9, 9], ls=":", color="#888"); ax.text(0.3, -12.5, "trục bánh", fontsize=8, ha="center")
names = ["L2", "L1", "M", "R1", "R2"]
for i, a in enumerate([60, 30, 0, -30, -60]):
    t = math.radians(a)
    x, y = 6 + 5 * math.cos(t), 5 * math.sin(t)
    ax.add_patch(Circle((x, y), 0.8, fc=BLUE, ec=NAVY))
    ax.text(x + 1.2, y, names[i], fontsize=9, va="center", fontweight="bold")
ax.add_patch(matplotlib.patches.Arc((6, 0), 10, 10, theta1=-70, theta2=70, ls="--", color=BLUE))
ax.add_patch(Rectangle((13, -14), 3, 28, fc="black"))
ax.text(14.5, 15, "vạch ngang của nút", ha="center", fontsize=8.5)
ax.annotate("", xy=(12.5, 0), xytext=(19, 0), arrowprops=dict(arrowstyle="<-", color=RED, lw=2))
ax.text(21, 0.5, "xe tiến tới", color=RED, fontsize=9)
ax.text(24, -6, "Thứ tự chạm vạch:\n1) M (nhô xa nhất)\n2) L1 và R1\n3) L2 và R2 (gần trục nhất)\n→ phát hiện nút bằng\n   tích lũy trong cửa sổ thời gian", fontsize=8.5, va="center")
ax.set_xlim(-16, 42); ax.set_ylim(-15, 17)
ax.set_title("Thanh 5 mắt xếp bán nguyệt (bán kính 5 cm, góc 0/±30/±60°)", fontsize=11, fontweight="bold", color=NAVY)
save(fig, "mat_ban_nguyet.png")

# ------------------------------------------------------------------ quét hình học (từ kết quả mô phỏng)
res = os.path.join(ROOT, "docs", "ket_qua_mo_phong", "quet_hinh_hoc.txt")
if os.path.exists(res):
    rows = [l.split() for l in open(res, encoding="utf-8").read().splitlines()[1:] if l.strip()]
    ys = sorted({float(r[0]) for r in rows}); rs = sorted({float(r[1]) for r in rows})
    grid = [[0] * len(rs) for _ in ys]
    for r in rows:
        grid[ys.index(float(r[0]))][rs.index(float(r[1]))] = float(r[2])
    fig, ax = plt.subplots(figsize=(6.2, 4.2))
    im = ax.imshow(grid, cmap="RdYlGn", vmin=40, vmax=100, origin="lower", aspect="auto")
    for i in range(len(ys)):
        for j in range(len(rs)):
            ax.text(j, i, f"{grid[i][j]:.0f}%", ha="center", va="center", fontsize=10, fontweight="bold")
    ax.set_xticks(range(len(rs))); ax.set_xticklabels([f"{r:g} cm" for r in rs])
    ax.set_yticks(range(len(ys))); ax.set_yticklabels([f"{y:g} cm" for y in ys])
    ax.set_xlabel("Tầm đọc màu tối đa của cảm biến"); ax.set_ylabel("Mặt cảm biến cách tâm xe")
    ax.set_title("Tỉ lệ bắt được vật đỏ theo cách lắp cảm biến màu\n(mô phỏng 20 lượt / ô, vật cách mép line 3–7 cm)", fontsize=10, fontweight="bold", color=NAVY)
    fig.colorbar(im, ax=ax, shrink=0.8)
    save(fig, "hinh_hoc_cam_bien.png")

# ------------------------------------------------------------------ quỹ đạo mô phỏng thật
trail = os.path.join(ROOT, "build", "trail3.csv")
if os.path.exists(trail):
    T, O, A = [], [], []
    for row in csv.reader(open(trail)):
        if row[0] == "T": T.append((float(row[1]), float(row[2])))
        elif row[0] == "O": O.append((float(row[1]), float(row[2]), row[3], int(row[4])))
        elif row[0] == "A": A.append((float(row[1]), float(row[2])))
    fig, ax = plt.subplots(figsize=(7.2, 7.2)); field(ax, numbers=False)
    ax.plot([p[0] for p in T], [p[1] for p in T], color=BLUE, lw=1.2, alpha=0.8, zorder=2)
    for x, y, c, r in O:
        ax.add_patch(Circle((x, y), 4, fc=c, ec="black" if r else "none", lw=1.5, zorder=6))
    for x, y in A:
        ax.add_patch(Circle((x, y), 9, fc="none", ec=RED, lw=2, zorder=7))
    ax.set_title("Một lượt mô phỏng (seed 3): quỹ đạo xe (xanh),\nvật đã báo (viền đen), vị trí cảnh báo (vòng đỏ)", fontsize=10.5, fontweight="bold", color=NAVY)
    save(fig, "quy_dao_mo_phong.png")

# ------------------------------------------------------------------ quy trình Superpowers
fig, ax = canvas(11, 3.4, 110, 34)
st = [("1. Brainstorming", "hiểu yêu cầu,\nchọn cách làm"), ("2. Spec", "tài liệu thiết kế\n(docs/superpowers/specs)"), ("3. Plan", "kế hoạch 10 task\n(docs/superpowers/plans)"),
      ("4. TDD", "viết test TRƯỚC,\nthấy FAIL rồi mới code"), ("5. Mô phỏng", "chạy cả lượt thi\ntìm lỗi hệ thống"), ("6. Debug có hệ thống", "tìm nguyên nhân gốc\ntrước khi sửa"), ("7. Review", "rà soát toàn nhánh\ntrước khi bàn giao")]
for i, (t, d) in enumerate(st):
    x = 1 + i * 15.6
    box(ax, x, 8, 14, 18, "", fc=LB if i % 2 == 0 else YEL)
    ax.text(x + 7, 21.5, t, ha="center", fontsize=8.3, fontweight="bold", color=NAVY)
    ax.text(x + 7, 13.5, d, ha="center", va="center", fontsize=7.3)
    if i < len(st) - 1:
        arr(ax, x + 14, 17, x + 15.6, 17)
ax.text(55, 31, "Quy trình phát triển theo Superpowers", ha="center", fontsize=11, fontweight="bold", color=NAVY)
save(fig, "quy_trinh.png")

# ------------------------------------------------------------------ schematic của đội
for f in os.listdir(os.path.dirname(ROOT)):
    if f.lower().endswith(".jpg"):
        shutil.copy(os.path.join(os.path.dirname(ROOT), f), os.path.join(OUT, "schematic_doi.jpg"))
        break
print("Đã vẽ:", sorted(os.listdir(OUT)))
