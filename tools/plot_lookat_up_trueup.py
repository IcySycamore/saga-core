"""
lookAt 教学图：up（用户给的参考） vs trueUp（算出来的真实头顶）
用 2D 侧视图 + 3D 视图展示 Gram-Schmidt 正交化过程：
  1. forward f = normalize(target - eye)
  2. right s  = normalize(cross(f, up))   ← 用传入 up 叉乘得到 right
  3. trueUp u = cross(s, f)               ← 用 right 叉乘 forward 得到真正的头顶
关键：传入 up 不用精确垂直，会被"投影/重算"成 trueUp
"""
import math
import matplotlib
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

matplotlib.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei", "SimSun"]
matplotlib.rcParams["axes.unicode_minus"] = False

# ===== 设置：相机看向斜下前方，up 故意传个"不太垂直"的方向 =====
eye = np.array([0.0, 0.0, 0.0])
target = np.array([1.0, 0.3, 0.0])          # 前方略偏上
up_given = np.array([0.0, 0.8, 0.6])        # 用户给的 up：不垂直于 forward！

def normalize(v):
    n = np.linalg.norm(v)
    return v / n if n > 1e-8 else v

f = normalize(target - eye)                   # forward
s_raw = np.cross(f, up_given)                 # right（未归一化）
s = normalize(s_raw)                          # right（归一化）
u = normalize(np.cross(s, f))                 # trueUp（真正的头顶）
up_proj = normalize(up_given - np.dot(up_given, f) * f)  # up 投影到垂直面（对比用）

print("forward f :", np.round(f, 3))
print("up(用户给):", np.round(up_given, 3), " norm=", round(np.linalg.norm(up_given),3))
print("right s   :", np.round(s, 3))
print("trueUp u  :", np.round(u, 3))
print("f·up(用户给)=", round(np.dot(f, up_given),3), "(非0=不垂直)")
print("f·trueUp  =", round(np.dot(f, u),3), "(0=精确垂直)")

# ===== 绘图 =====
fig = plt.figure(figsize=(14, 6))

def draw_arrow(ax, start, vec, color, label, lw=2.5, ls="-"):
    ax.quiver(start[0], start[1], start[2],
              vec[0], vec[1], vec[2],
              color=color, arrow_length_ratio=0.12, linewidth=lw, linestyle=ls)
    tip = start + vec * 1.15
    ax.text(tip[0], tip[1], tip[2], label, color=color, fontsize=11)

# ---- 左图：3D 视角 ----
ax1 = fig.add_subplot(1, 2, 1, projection="3d")
# 相机
ax1.scatter(*eye, color="black", s=80, marker="o")
ax1.text(*eye, " 相机 eye", color="black", fontsize=11)
ax1.scatter(*target, color="gray", s=50, marker="x")
ax1.text(*target, " target", color="gray", fontsize=10)
# forward
draw_arrow(ax1, eye, f*2.0, "tab:blue", "forward f")
# 用户给的 up（灰色虚线，明显不垂直 forward）
draw_arrow(ax1, eye, up_given*1.6, "gray", "用户给的 up", lw=2, ls="--")
# right
draw_arrow(ax1, eye, s*1.8, "tab:orange", "right s")
# trueUp
draw_arrow(ax1, eye, u*1.8, "tab:green", "trueUp u")
# 辅助：垂直面（up 投影）
draw_arrow(ax1, eye, up_proj*1.6, "lightgreen", "up投影(垂直f)", lw=1.2, ls=":")
ax1.set_xlim(-2.5, 2.5); ax1.set_ylim(-2.5, 2.5); ax1.set_zlim(-2.5, 2.5)
ax1.set_xlabel("X"); ax1.set_ylabel("Y"); ax1.set_zlabel("Z")
ax1.view_init(elev=20, azim=-60)
ax1.set_title("① 3D：up(灰) vs trueUp(绿)\nright=normalize(cross(f, up))")

# ---- 右图：垂直 forward 的切面（从前方看，只看 right/up 平面）----
# 建立以 f 为 z 轴的局部坐标系，投影 right 和 up 到这个"相机屏幕平面"
ax2 = fig.add_subplot(1, 2, 2)
# 屏幕平面 = 由 s（右）和 u（上）张成
for name, vec, col in [("right s", s, "tab:orange"), ("trueUp u", u, "tab:green"),
                        ("用户up投影", up_proj, "gray")]:
    # 分解到 (s, u) 基
    xs = np.dot(vec, s)
    ys = np.dot(vec, u)
    ax2.annotate("", xy=(xs, ys), xytext=(0, 0),
                 arrowprops=dict(arrowstyle="->", color=col, lw=2.5))
    ax2.text(xs*1.1, ys*1.1, name, color=col, fontsize=11)
ax2.add_patch(plt.Circle((0,0), 1.0, fill=False, color="lightgray", lw=1))
ax2.axhline(0, color="gray", lw=0.8); ax2.axvline(0, color="gray", lw=0.8)
ax2.text(1.1, -0.1, "right(+s)", color="tab:orange", fontsize=10)
ax2.text(-0.1, 1.1, "up(+u)", color="tab:green", fontsize=10)
ax2.set_xlim(-1.5, 1.5); ax2.set_ylim(-1.5, 1.5)
ax2.set_aspect("equal"); ax2.grid(True, alpha=0.3)
ax2.set_title("② 相机屏幕平面（垂直 forward）\n用户 up 投影(灰) → trueUp(绿) 补正")

plt.tight_layout()
plt.savefig("docs/design/fig_lookat_up_trueup.png", dpi=130, bbox_inches="tight")
print("\nsaved: docs/design/fig_lookat_up_trueup.png")
