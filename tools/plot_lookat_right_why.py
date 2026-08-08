"""
lookAt 关键性质教学图：为什么 right 是"你想要的"？
核心：叉乘自动丢弃 up 中平行于 f 的分量，只保留垂直分量。
  up = up_par + up_perp
  cross(f, up) = cross(f, up_par) + cross(f, up_perp) = 0 + cross(f, up_perp)
所以：
  - right 只由 up 的"垂直分量方向"决定（= 你对"头顶朝哪"的意图）
  - 两个 up 若垂直分量方向相同（平行分量不同）→ 产生完全相同的 right
子图1：up 分解为 平行/垂直 分量
子图2：两个不同 up（平行分量不同）→ 相同 right（证明确定性）
"""
import math
import matplotlib
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

matplotlib.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei", "SimSun"]
matplotlib.rcParams["axes.unicode_minus"] = False

def normalize(v):
    n = np.linalg.norm(v)
    return v / n if n > 1e-8 else v

def right_from(f, up):
    """lookAt 的 right 计算"""
    s = np.cross(f, up)
    n = np.linalg.norm(s)
    if n < 1e-8:
        return None
    return s / n

# 固定 forward（斜向上看，带一点偏航）
f = normalize(np.array([1.0, 0.35, 0.2]))

# 两个"意图相同"的 up：垂直分量方向相同，只差平行分量
# up_a = 标准世界向上 (0,1,0)（可能不完全垂直 f）
up_a = np.array([0.0, 1.0, 0.0])
# up_b = 在 up_a 基础上加一个平行于 f 的偏移（"乱"一点，但头顶意图一样）
up_par_extra = f * 0.5
up_b = up_a + up_par_extra

# 分解 up_a
up_par_a = np.dot(up_a, f) * f
up_perp_a = up_a - up_par_a
up_perp_a_n = normalize(up_perp_a)

# right 计算
r_a = right_from(f, up_a)
r_b = right_from(f, up_b)
print("forward f    :", np.round(f, 3))
print("up_a (0,1,0) :", up_a, " 平行分量=", np.round(up_par_a,3), " 垂直分量=", np.round(up_perp_a_n,3))
print("up_b (加偏移):", np.round(up_b,3))
print("right_a      :", np.round(r_a,3))
print("right_b      :", np.round(r_b,3))
print("right_a == right_b ? ", np.allclose(r_a, r_b))

# ===== 绘图 =====
fig = plt.figure(figsize=(14, 6))

def draw_arrow(ax, start, vec, color, label, lw=2.5, ls="-"):
    ax.quiver(start[0], start[1], start[2],
              vec[0], vec[1], vec[2],
              color=color, arrow_length_ratio=0.10, linewidth=lw, linestyle=ls)
    tip = start + vec * 1.18
    ax.text(tip[0], tip[1], tip[2], label, color=color, fontsize=10)

# ---- 子图1：up 分解 ----
ax1 = fig.add_subplot(1, 2, 1, projection="3d")
draw_arrow(ax1, (0,0,0), f*2.0, "tab:blue", "forward f")
draw_arrow(ax1, (0,0,0), up_a*1.8, "gray", "up(用户给的)", lw=2, ls="--")
draw_arrow(ax1, (0,0,0), up_par_a*2.2, "crimson", "up_par(平行f)", lw=1.8, ls=":")
draw_arrow(ax1, (0,0,0), up_perp_a_n*1.8, "tab:orange", "up_perp(垂直f)", lw=2.5)
draw_arrow(ax1, (0,0,0), r_a*1.8, "tab:green", "right", lw=3)
ax1.scatter(0,0,0, color="black", s=60)
ax1.set_xlim(-2.5,2.5); ax1.set_ylim(-2.5,2.5); ax1.set_zlim(-2.5,2.5)
ax1.set_xlabel("X"); ax1.set_ylabel("Y"); ax1.set_zlabel("Z")
ax1.view_init(elev=18, azim=-55)
ax1.set_title("① up 分解：cross(f,up) 只吃 up_perp\n(平行分量被叉乘消掉: cross(f,up_par)=0)")

# ---- 子图2：两个 up → 相同 right ----
ax2 = fig.add_subplot(1, 2, 2, projection="3d")
draw_arrow(ax2, (0,0,0), f*2.0, "tab:blue", "forward f", lw=2)
# up_a
draw_arrow(ax2, (0,0,0), up_a*1.6, "gray", "up_a(0,1,0)", lw=2, ls="--")
# up_b（带平行偏移）
draw_arrow(ax2, (0,0,0), up_b*1.6, "purple", "up_b(+f×0.5)", lw=2, ls="--")
# 两个 right（应重合）
draw_arrow(ax2, (0,0,0), r_a*1.9, "tab:green", "right_a", lw=3.5)
if np.allclose(r_a, r_b):
    ax2.text(0,0,0.1, "", fontsize=8)
else:
    draw_arrow(ax2, (0,0,0), r_b*1.9, "crimson", "right_b", lw=3, ls=":")
ax2.scatter(0,0,0, color="black", s=60)
ax2.set_xlim(-2.5,2.5); ax2.set_ylim(-2.5,2.5); ax2.set_zlim(-2.5,2.5)
ax2.set_xlabel("X"); ax2.set_ylabel("Y"); ax2.set_zlabel("Z")
ax2.view_init(elev=18, azim=-55)
ax2.set_title("② up_a 与 up_b（仅差平行分量）\n→ 产生相同的 right（绿）")

plt.tight_layout()
plt.savefig("docs/design/fig_lookat_right_why.png", dpi=130, bbox_inches="tight")
print("\nsaved: docs/design/fig_lookat_right_why.png")
