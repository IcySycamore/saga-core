"""
深度映射教学图：z_ndc = z'/w' 的非线性曲线（near=0.1, far=100）
- 实线：真实透视映射（非线性，近处陡峭=精度高，远处平坦=精度低）
- 虚线：线性映射参考（如果是均匀的会怎样）
- 标注：几何中点 z_ndc≈0.998（几乎贴到 far）→ 证明非线性
"""
import math
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

matplotlib.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei", "SimSun"]
matplotlib.rcParams["axes.unicode_minus"] = False

NEAR, FAR = 0.1, 100.0

# z_ndc(d) 其中 d = 视图空间深度（正数，从 near 到 far）
def z_ndc(d):
    return ((FAR + NEAR) * d - 2 * FAR * NEAR) / (d * (FAR - NEAR))

def z_ndc_linear(d):
    return 2.0 * (d - NEAR) / (FAR - NEAR) - 1.0

d = np.linspace(NEAR, FAR, 4000)
zn = [z_ndc(x) for x in d]
zl = [z_ndc_linear(x) for x in d]

mid = (NEAR + FAR) / 2.0  # 几何中点
zn_mid = z_ndc(mid)

fig, ax = plt.subplots(figsize=(9, 5))
fig.suptitle("归一化深度 z_ndc = z' / w'：非线性映射（near=0.1, far=100）", fontsize=13)

ax.plot(d, zn, color="crimson", lw=2.5, label="真实透视映射（非线性）")
ax.plot(d, zl, color="gray", lw=1.5, ls="--", label="线性映射参考（若均匀）")

# 边界与中点标注
ax.axhline(-1, color="green", ls=":", lw=1.0)
ax.axhline(1, color="green", ls=":", lw=1.0)
ax.axhline(0, color="lightgray", lw=0.8)
ax.text(NEAR + 0.5, -1.08, "z_ndc = -1  ← 近平面 (d=near)", color="green", fontsize=9)
ax.text(FAR - 8, 1.03, "z_ndc = +1  ← 远平面 (d=far)", color="green", fontsize=9)

ax.plot(mid, zn_mid, marker="o", color="blue", zorder=6)
ax.annotate(f"几何中点 d={mid:.1f}\n→ z_ndc ≈ {zn_mid:.3f}（几乎贴到 far！）",
            xy=(mid, zn_mid), xytext=(mid + 10, -0.55),
            arrowprops=dict(arrowstyle="->", color="blue"),
            color="blue", fontsize=10)

# 近处陡峭/远处平坦 标注
ax.annotate("近处：曲线陡峭\n（z_ndc 变化快 → 精度高）",
            xy=(1.0, z_ndc(1.0)), xytext=(12, -0.9),
            arrowprops=dict(arrowstyle="->", color="crimson"),
            color="crimson", fontsize=9)
ax.annotate("远处：曲线平坦\n（z_ndc 几乎不变 → 精度低 → z-fighting）",
            xy=(60, z_ndc(60)), xytext=(35, 0.55),
            arrowprops=dict(arrowstyle="->", color="crimson"),
            color="crimson", fontsize=9)

ax.set_xlabel("视图空间深度 d = -z_view（近 → 远）")
ax.set_ylabel("z_ndc = z'/w'")
ax.set_xlim(NEAR, FAR)
ax.set_ylim(-1.3, 1.4)
ax.grid(True, alpha=0.3)
ax.legend(loc="center left", fontsize=9)

plt.tight_layout()
plt.savefig("docs/design/fig_depth_mapping.png", dpi=130, bbox_inches="tight")
print("saved: docs/design/fig_depth_mapping.png")
print(f"几何中点 d={mid:.2f}: z_ndc = {zn_mid:.4f}（线性应为 0）")
print(f"near 处 z_ndc = {z_ndc(NEAR):.3f}, far 处 z_ndc = {z_ndc(FAR):.3f}")
