"""
渲染讲解图：drawGrid + drawSegment 三步流水线（真实矩阵运算复现）
复现 render_demo 的临时正交视图：
  vp = orthographic(-1,1,-1,1, 0.1, 10)
  drawGrid(2.0, 8, vp)  →  18 条 XZ 平面线（y=0）
面板1：3D 世界空间（XZ 平面网格，18 条线）
面板2：NDC 空间（vp 变换后）—— 观察正交投影的效果
面板3：屏幕空间（最终 SDL_RenderLine 结果）
"""
import math
import matplotlib
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

matplotlib.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei", "SimSun"]
matplotlib.rcParams["axes.unicode_minus"] = False

# ============ 复现 C++ math::orthographic（列主序，但这里用行主序等价计算） ============
# orthographic(left=-1, right=1, bottom=-1, top=1, near=0.1, far=10)
L, R, B, T, N, F = -1.0, 1.0, -1.0, 1.0, 0.1, 10.0
# 列主序矩阵（与 C++ 一致）：m[col*4+row]
m = [0.0] * 16
m[0] = 2.0 / (R - L)          # at(0,0) x 缩放
m[5] = 2.0 / (T - B)          # at(1,1) y 缩放
m[10] = -2.0 / (F - N)        # at(2,2) z 缩放
m[12] = -(R + L) / (R - L)    # at(0,3) x 平移 = 0
m[13] = -(T + B) / (T - B)    # at(1,3) y 平移 = 0
m[14] = -(F + N) / (F - N)    # at(2,3) z 平移
m[15] = 1.0

def apply_mvp(v):
    """C++ Matrix4x4::operator*(Vector3)：M*v 含齐次除法"""
    x = m[0]*v[0] + m[4]*v[1] + m[8]*v[2] + m[12]
    y = m[1]*v[0] + m[5]*v[1] + m[9]*v[2] + m[13]
    z = m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14]
    w = m[3]*v[0] + m[7]*v[1] + m[11]*v[2] + m[15]
    if w != 1.0 and w != 0.0:
        inv = 1.0/w
        return (x*inv, y*inv, z*inv)
    return (x, y, z)

def is_outside_ndc(p):
    return p[0] < -1 or p[0] > 1 or p[1] < -1 or p[1] > 1 or p[2] < -1 or p[2] > 1

# ============ 生成网格线（与 drawGrid 完全一致） ============
SIZE, DIVS = 2.0, 8
half, step = SIZE*0.5, SIZE/DIVS
lines_3d = []
for i in range(DIVS+1):
    coord = -half + step*i
    lines_3d.append(((coord, 0.0, -half), (coord, 0.0, half)))  # 沿X
    lines_3d.append(((-half, 0.0, coord), (half, 0.0, coord)))  # 沿Z

# ============ 变换到 NDC ============
lines_ndc = [((apply_mvp(a)), (apply_mvp(b))) for a, b in lines_3d]

# ============ 变换到屏幕（width=1280, height=720） ============
W, H = 1280, 720
def to_screen(p):
    return (int((p[0]+1.0)*0.5*W), int((1.0-p[1])*0.5*H))
lines_scr = [(to_screen(a), to_screen(b)) for a, b in lines_ndc]

# ============ 统计 ============
print("网格线总数:", len(lines_3d))
print("NDC 中 y 值集合（应全为 0，因网格在 y=0 平面 + 正交投影）:")
print("  ", sorted(set(round(a[1],3) for a,b in lines_ndc)))
print("NDC 中 z 值范围:", min(a[2] for a,b in lines_ndc), "~", max(a[2] for a,b in lines_ndc))
print("屏幕 y 坐标集合:", sorted(set(a[1] for a,b in lines_scr)))

# ============ 画图 ============
fig = plt.figure(figsize=(16, 5.2))
fig.suptitle("drawGrid + drawSegment 三步流水线（复现 render_demo 真实矩阵运算）", fontsize=14)

# ---- 面板1：3D 世界空间 ----
ax1 = fig.add_subplot(1, 3, 1, projection="3d")
for (a, b) in lines_3d:
    ax1.plot([a[0], b[0]], [a[1], b[1]], [a[2], b[2]], color="steelblue", lw=1.2)
# 高亮第一条线（x=-1 竖线）
a0, b0 = lines_3d[0]
ax1.plot([a0[0], b0[0]], [a0[1], b0[1]], [a0[2], b0[2]], color="crimson", lw=3)
ax1.scatter(*a0, color="crimson", s=40)
ax1.scatter(*b0, color="crimson", s=40)
ax1.text(a0[0], a0[1], a0[2]+0.15, "A(-1,0,-1)", color="crimson", fontsize=10)
ax1.text(b0[0], b0[1], b0[2]+0.15, "B(-1,0,+1)", color="crimson", fontsize=10)
ax1.set_xlabel("X"); ax1.set_ylabel("Y"); ax1.set_zlabel("Z")
ax1.set_title("① 世界空间：XZ 平面网格\n(y=0, 8×8格, 18条线)")
ax1.set_xlim(-1.4, 1.4); ax1.set_ylim(-1.4, 1.4); ax1.set_zlim(-1.4, 1.4)
ax1.view_init(elev=25, azim=-60)

# ---- 面板2：NDC 空间 ----
ax2 = fig.add_subplot(1, 3, 2)
# NDC 可见框 [-1,1]²
ax2.add_patch(plt.Rectangle((-1, -1), 2, 2, fill=False, edgecolor="green", lw=2))
ax2.text(-1.05, -1.15, "[-1,1]² 可见区", color="green", fontsize=9)
# 变换后的线（全部压在 y=0 一条线上）
for (a, b) in lines_ndc:
    ax2.plot([a[0], b[0]], [a[1], b[1]], color="steelblue", lw=1.0, alpha=0.8)
a0n, b0n = lines_ndc[0]
ax2.plot([a0n[0], b0n[0]], [a0n[1], b0n[1]], color="crimson", lw=3)
ax2.scatter(*a0n[:2], color="crimson", s=50)
ax2.scatter(*b0n[:2], color="crimson", s=50)
ax2.text(a0n[0]+0.03, a0n[1]+0.06, f"A'({a0n[0]:.2f},{a0n[1]:.2f})", color="crimson", fontsize=9)
ax2.text(b0n[0]+0.03, b0n[1]-0.15, f"B'({b0n[0]:.2f},{b0n[1]:.2f})", color="crimson", fontsize=9)
ax2.set_xlim(-1.6, 1.6); ax2.set_ylim(-1.6, 1.6)
ax2.set_aspect("equal"); ax2.grid(True, alpha=0.3)
ax2.set_title("② NDC：vp 变换后\n⚠ 全部线被压到 y=0 一条水平线！")

# ---- 面板3：屏幕空间 ----
ax3 = fig.add_subplot(1, 3, 3)
# 深蓝背景
ax3.add_patch(plt.Rectangle((0, 0), W, H, color=(0x10/255, 0x20/255, 0x40/255)))
# 所有线（实际渲染：全是一条水平线）
for (a, b) in lines_scr:
    ax3.plot([a[0], b[0]], [a[1], b[1]], color=(0xC8/255, 0xD0/255, 0xE0/255), lw=2)
ax3.set_xlim(0, W); ax3.set_ylim(H, 0)
ax3.set_title(f"③ 屏幕 {W}×{H}\nSDL_RenderLine 结果：只画出一条水平亮线")
ax3.set_xlabel("x 像素"); ax3.set_ylabel("y 像素")

plt.tight_layout()
plt.savefig("docs/design/fig_drawgrid_pipeline.png", dpi=130, bbox_inches="tight")
print("\nsaved: docs/design/fig_drawgrid_pipeline.png")
print(f"屏幕 y 坐标: {sorted(set(a[1] for a,b in lines_scr))}")
