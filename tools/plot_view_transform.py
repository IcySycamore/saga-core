"""
视图变换教学图：平移 + 旋转（2D 简化版）
展示 V = R * T(-eye) 的两步：
  图1 世界坐标系（相机在 (3,2)，forward 朝 45°，物体在 (10,5)）
  图2 平移后：相机到原点，物体到 (7,3) —— 以相机为原点的相对坐标
  图3 旋转后：相机 forward 对齐 +y，物体到旋转后的位置
"""
import math
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyArrow

# 中文字体支持（Windows 常见中文字体）
matplotlib.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei", "SimSun"]
matplotlib.rcParams["axes.unicode_minus"] = False  # 负号正常显示

CAM = (3.0, 2.0)   # 相机位置
OBJ = (10.0, 5.0)  # 物体位置
ANG = math.radians(45.0)  # 相机朝向：朝右上 45°

def rot(p, a):
    """绕原点旋转角度 a"""
    c, s = math.cos(a), math.sin(a)
    return (p[0]*c - p[1]*s, p[0]*s + p[1]*c)

def draw_axes(ax, origin, scale=1.0, label="", color_alpha=0.9):
    """画一组坐标轴（x 红、y 绿）"""
    o = origin
    ax.annotate("", xy=(o[0]+1.2*scale, o[1]), xytext=o,
                arrowprops=dict(arrowstyle="->", color="red", lw=1.6, alpha=color_alpha))
    ax.annotate("", xy=(o[0], o[1]+1.2*scale), xytext=o,
                arrowprops=dict(arrowstyle="->", color="green", lw=1.6, alpha=color_alpha))
    ax.text(o[0]+1.25*scale, o[1], "x", color="red", fontsize=11, alpha=color_alpha)
    ax.text(o[0], o[1]+1.25*scale, "y", color="green", fontsize=11, alpha=color_alpha)

def draw_cam(ax, pos, ang, label, color="blue"):
    """画相机：位置 + forward（蓝）+ right（橙）本地轴"""
    ax.plot(*pos, marker="o", markersize=10, color=color, zorder=5)
    ax.text(pos[0], pos[1]-0.55, label, color=color, fontsize=11, ha="center", zorder=6)
    fwd = rot((0, 1.5), ang)   # forward：沿朝向 45°
    rgt = rot((1.2, 0), ang)   # right：垂直 forward
    ax.annotate("", xy=(pos[0]+fwd[0], pos[1]+fwd[1]), xytext=pos,
                arrowprops=dict(arrowstyle="->", color=color, lw=2.2))
    ax.annotate("", xy=(pos[0]+rgt[0], pos[1]+rgt[1]), xytext=pos,
                arrowprops=dict(arrowstyle="->", color="orange", lw=1.6, alpha=0.8))
    ax.text(pos[0]+fwd[0], pos[1]+fwd[1]+0.15, "forward", color=color, fontsize=9)
    ax.text(pos[0]+rgt[0], pos[1]+rgt[1]+0.15, "right", color="orange", fontsize=9)

def draw_obj(ax, pos, label, color="purple"):
    ax.plot(*pos, marker="s", markersize=11, color=color, zorder=5)
    ax.text(pos[0], pos[1]-0.55, label, color=color, fontsize=11, ha="center", zorder=6)

# 计算各阶段
obj_translated = (OBJ[0]-CAM[0], OBJ[1]-CAM[1])   # 平移后 (7,3)
cam_fwd_aligned = -ANG                              # 旋转 -45°：forward 45°→0°... 不，对齐 +y 需 -45°
# 注意：相机 forward 原本 45°（右上），要对齐 +y（90°）→ 旋转角度 = 90-45 = 45°
# 但我们是"旋转坐标系让 forward 对齐 +y"，即把整个坐标系转 (90-45)=45°
rot_angle = math.radians(90) - ANG   # = 45°（forward 45° 转到 90°）
obj_rotated = rot(obj_translated, rot_angle)  # 物体随坐标系旋转

fig, axes = plt.subplots(1, 3, figsize=(15, 4.6))
fig.suptitle("视图变换 V = R × T(-eye)：先平移（相机到原点），再旋转（forward 对齐 +y）",
             fontsize=13, y=1.02)

# ---- 图1：世界坐标系 ----
ax = axes[0]
draw_axes(ax, (0, 0), label="world")
draw_cam(ax, CAM, ANG, "相机 (3,2)")
draw_obj(ax, OBJ, "物体 (10,5)")
# 虚线：从世界轴连到相机，表示"待平移"
ax.plot([CAM[0], CAM[0]], [0, CAM[1]], ls=":", color="gray", alpha=0.6)
ax.plot([0, CAM[0]], [CAM[1], CAM[1]], ls=":", color="gray", alpha=0.6)
ax.set_title("① 世界坐标系\n(相机在 (3,2)，forward 朝 45°)")
ax.set_xlim(-1, 12); ax.set_ylim(-1, 8); ax.set_aspect("equal")
ax.grid(True, alpha=0.3)

# ---- 图2：平移后 ----
ax = axes[1]
draw_axes(ax, (0, 0), label="(相机为原点)")
draw_cam(ax, (0, 0), ANG, "相机 (0,0)")
draw_obj(ax, obj_translated, "物体 (7,3)")
ax.annotate("", xy=obj_translated, xytext=(0, 0),
            arrowprops=dict(arrowstyle="->", color="gray", lw=1.2, ls="--"))
ax.set_title("② 平移 T(-eye)\n世界整体搬到相机脚下\n→ 以相机为原点的相对坐标")
ax.set_xlim(-1, 10); ax.set_ylim(-4, 8); ax.set_aspect("equal")
ax.grid(True, alpha=0.3)

# ---- 图3：旋转后 ----
ax = axes[2]
draw_axes(ax, (0, 0), label="(屏幕坐标)")
draw_cam(ax, (0, 0), math.radians(90), "相机 (0,0)")
draw_obj(ax, obj_rotated, f"物体 ({obj_rotated[0]:.2f}, {obj_rotated[1]:.2f})")
# 用灰虚线标出旋转前的 forward 45° 方向（从相机出发）
old_fwd = rot((0, 2.0), ANG)
ax.annotate("", xy=old_fwd, xytext=(0, 0),
            arrowprops=dict(arrowstyle="->", color="gray", lw=1.4, ls="--"))
ax.text(old_fwd[0]+0.2, old_fwd[1]+0.2, "旋转前 forward 45°", color="gray", fontsize=9)
ax.annotate("", xy=obj_rotated, xytext=(0, 0),
            arrowprops=dict(arrowstyle="->", color="gray", lw=1.2, ls=":"))
ax.set_title("③ 旋转 R(+45°)\n坐标系旋转 → forward 对齐 +y\n→ 物体变成屏幕坐标")
ax.set_xlim(-1, 10); ax.set_ylim(-4, 8); ax.set_aspect("equal")
ax.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig("docs/design/fig_view_transform.png", dpi=130, bbox_inches="tight")
print("saved: docs/design/fig_view_transform.png")
print(f"平移后物体: {obj_translated}")
print(f"旋转角: {math.degrees(rot_angle):.1f}°（使 forward 45° 对齐 +y=90°）")
print(f"旋转后物体: ({obj_rotated[0]:.3f}, {obj_rotated[1]:.3f})")
