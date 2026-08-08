"""
透视投影矩阵 P 逐格标注图：5 个非零项的含义
代入具体值：fov=60°, aspect=16/9, near=0.1, far=100
- 蓝：x/y 缩放（fov + aspect 控制视野）
- 红：z 改造（缩放+平移，把深度压到 [-1,1]）
- 绿：w = -z（装深度，供透视除法）
"""
import math
import matplotlib
import matplotlib.pyplot as plt

matplotlib.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei", "SimSun"]
matplotlib.rcParams["axes.unicode_minus"] = False

FOV = math.radians(60.0)
ASPECT = 16.0 / 9.0
N, F = 0.1, 100.0
tan = math.tan(FOV / 2.0)

# 5 个非零项： (row, col, 值, 颜色, 说明)
cells = [
    (0, 0, 1.0/(ASPECT*tan), "tab:blue",
     f"x缩放\n1/(aspect·tan) = {1.0/(ASPECT*tan):.3f}\n(视野宽 + 屏幕宽高比修正)"),
    (1, 1, 1.0/tan, "tab:blue",
     f"y缩放\n1/tan(fov/2) = {1.0/tan:.3f}\n(视野高)"),
    (2, 2, -(F+N)/(F-N), "tab:red",
     f"z缩放\n-(f+n)/(f-n) = {-(F+N)/(F-N):.3f}\n(配合平移→近-1远+1)"),
    (2, 3, -2.0*F*N/(F-N), "tab:red",
     f"z平移\n-2fn/(f-n) = {-2.0*F*N/(F-N):.3f}\n(常数项，让近平面=-1)"),
    (3, 2, -1.0, "tab:green",
     f"w=-z\n-1\n(深度装进w→透视除法)"),
]

fig, ax = plt.subplots(figsize=(10, 7.5))
fig.suptitle("透视投影矩阵 P 的 5 个非零项（fov=60°, aspect=16/9, near=0.1, far=100）",
             fontsize=13)

# 画 4x4 网格
for r in range(4):
    for c in range(4):
        x, y = c, 3 - r
        color = "white"
        for (cr, cc, val, col, txt) in cells:
            if cr == r and cc == c:
                color = col
                break
        ax.add_patch(plt.Rectangle((x, y), 1, 1, facecolor=color, alpha=0.25,
                                   edgecolor="black", lw=1.2))
        ax.text(x+0.5, y+0.5, f"({r},{c})", ha="center", va="center",
                fontsize=11, color="dimgray" if color == "white" else "black")

# 每格填值
for (r, c, val, col, txt) in cells:
    x, y = c, 3 - r
    ax.text(x+0.5, y+0.78, f"{val:.3f}", ha="center", va="center",
            fontsize=14, fontweight="bold", color=col)
    ax.text(x+0.5, y+0.06, txt.split("\n")[0], ha="center", va="center",
            fontsize=8.5, color=col)

# 图例说明（在右侧）
legend = ("\n".join(
    ["图例：",
     "■ 蓝 (0,0)(1,1): x/y 缩放 —— 控制视野大小（fov+aspect）",
     "■ 红 (2,2)(2,3): z 改造 —— 把深度压到[-1,1]（近=-1 远=+1）",
     "■ 绿 (3,2): w=-z —— 深度装进w，供透视除法",
     "□ 白: 0（矩阵乘法的零项）"]))
ax.text(4.15, 2.0, legend, va="center", fontsize=10,
        bbox=dict(boxstyle="round", facecolor="lightyellow", alpha=0.9))

# 行列标签
for c in range(4):
    ax.text(c+0.5, -0.35, f"列{c}", ha="center", fontsize=10)
for r in range(4):
    ax.text(-0.35, 3-r+0.5, f"行{r}", ha="center", va="center", fontsize=10)

ax.set_xlim(-0.8, 7.2)
ax.set_ylim(-0.9, 4.2)
ax.set_aspect("equal")
ax.axis("off")

plt.tight_layout()
plt.savefig("docs/design/fig_perspective_matrix.png", dpi=130, bbox_inches="tight")
print("saved: docs/design/fig_perspective_matrix.png")
for (r, c, val, col, txt) in cells:
    print(f"at({r},{c}) = {val:.4f}")
