"""
投影教学图：fov / 视锥 / 深度 / 近大远小（俯视图）
- 相机在原点，看向 +x（即深度方向）
- 视锥：上下两条边张开 fov 角
- near/far 平面：两条竖线
- 近物体(深色)与远物体(浅色)：同样高度，投影到 near 平面上的高度不同 → 近大远小
"""
import math
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon

matplotlib.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei", "SimSun"]
matplotlib.rcParams["axes.unicode_minus"] = False

FOV = math.radians(60.0)   # fov（垂直视角 60°）
NEAR = 2.0
FAR = 10.0
H = 1.5                     # 物体半高
D_NEAR = 3.5                # 近物体距离
D_FAR = 8.0                 # 远物体距离

tan = math.tan(FOV / 2.0)

fig, ax = plt.subplots(figsize=(9, 5.5))
fig.suptitle("透视投影：fov 视锥 + 深度 + 近大远小（俯视图）", fontsize=13)

# ---- 视锥（三角形区域）----
ax.add_patch(Polygon([(0, 0), (FAR, FAR * tan), (FAR, -FAR * tan), (0, 0)],
                     closed=True, color="lightblue", alpha=0.35))
# 视锥上下边界线
ax.plot([0, FAR], [0, FAR * tan], color="steelblue", lw=1.4)
ax.plot([0, FAR], [0, -FAR * tan], color="steelblue", lw=1.4)

# ---- near / far 平面 ----
ax.axvline(NEAR, color="green", ls="--", lw=1.2)
ax.axvline(FAR, color="orange", ls="--", lw=1.2)
ax.text(NEAR, FAR * tan + 0.35, f"near 平面\n(屏幕在此, z={NEAR})", color="green",
        ha="center", fontsize=9)
ax.text(FAR, FAR * tan + 0.35, f"far 平面\n(z={FAR})", color="orange",
        ha="center", fontsize=9)

# ---- 相机 ----
ax.plot(0, 0, marker="*", markersize=18, color="blue", zorder=6)
ax.text(0, -0.7, "相机\n(原点)", color="blue", ha="center", fontsize=10)

# ---- 深度轴 ----
ax.annotate("", xy=(FAR + 0.8, 0), xytext=(-0.5, 0),
            arrowprops=dict(arrowstyle="->", color="gray", lw=1.5))
ax.text(FAR + 0.9, 0.15, "深度 d = 沿视线到相机的距离", color="gray", fontsize=9)

# ---- 两个物体 + 投影 ----
for d, h, col, label, off in [(D_NEAR, H, "darkred", "近物体", 0.5),
                              (D_FAR, H, "salmon", "远物体", 0.5)]:
    # 物体：竖线（顶 y=+h, 底 y=-h）
    ax.plot([d, d], [-h, h], color=col, lw=3, zorder=5)
    ax.plot(d, h, marker="o", color=col, zorder=5)
    ax.plot(d, -h, marker="o", color=col, zorder=5)
    ax.text(d, h + 0.4, label, color=col, ha="center", fontsize=10)
    # 从物体顶部/底部到相机的光线，交 near 平面于 y = ±h * NEAR / d
    y_top = h * NEAR / d
    y_bot = -h * NEAR / d
    ax.plot([0, d], [0, h], color=col, lw=1.0, ls=":", alpha=0.6)
    ax.plot([0, d], [0, -h], color=col, lw=1.0, ls=":", alpha=0.6)
    # near 平面上的投影高度（屏幕高度）
    ax.plot([NEAR, NEAR], [y_bot, y_top], color=col, lw=4, zorder=5)
    ax.text(NEAR, y_top + 0.25, f"投影高 = h×near/d\n= {y_top:.2f}", color=col,
            ha="center", fontsize=8)

ax.set_xlim(-0.8, FAR + 2.0)
ax.set_ylim(-4.2, 4.6)
ax.set_aspect("equal")
ax.set_xlabel("沿视线方向（俯视图，相机在原点看向右）")
ax.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig("docs/design/fig_projection_fov.png", dpi=130, bbox_inches="tight")
print("saved: docs/design/fig_projection_fov.png")
print(f"近物体 d={D_NEAR}: 投影高 = {H*NEAR/D_NEAR:.2f}")
print(f"远物体 d={D_FAR}: 投影高 = {H*NEAR/D_FAR:.2f}")
print(f"fov={math.degrees(FOV):.0f}°, 半角 tan={tan:.3f}")
