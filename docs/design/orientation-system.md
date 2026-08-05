# 姿态系统设计（Orientation System）

基于 ADR-0009。引擎姿态核心架构：**内部 S³，边界用户友好，连续性缝合**。

## 分层架构

```
┌─────────────────────────────────────────┐
│  用户/脚本层                             │
│  欧拉角、LookAt、FromTo 等友好接口       │
├─────────────────────────────────────────┤
│  连续性约束层 (Continuity Layer)         │
│  欧拉角序列 → S³ 最短路径反解            │
│  符号一致性保证 / quat→euler 输出约定    │
├─────────────────────────────────────────┤
│  S³ 流形运算层                           │
│  四元数、Slerp、指数映射、切空间工具     │
├─────────────────────────────────────────┤
│  物理模拟层                              │
│  指数映射积分 / 归一化投影 RK4           │
├─────────────────────────────────────────┤
│  SO(3) 输出层                            │
│  旋转矩阵、变换矩阵、法线矩阵            │
│  供渲染管线使用                          │
└─────────────────────────────────────────┘
```

## 核心数据结构

```cpp
struct Transform {
    Quaternion rotation;   // S³，随时保证 ||q|| = 1
    Vector3    position;   // R³，不受流形约束
    Vector3    scale;      // R³，不受流形约束
};

struct RotationCurve {
    std::vector<Quaternion> keyframes;  // 已在 S³，最短路径
    std::vector<float> times;
    EasingFunction easing;              // 节奏控制（S³ 外）
};

struct RigidBody {
    Quaternion orientation;   // S³ 当前位置
    Vector3    angularVelocity; // 切空间 so(3) 坐标（3 分量，永远良好定义）
};
```

## 关键算法

### 动画导入：欧拉关键帧 → S³ 连续性缝合（修正审计 Bug 1）

```cpp
std::vector<Quaternion> fixEulerCurve(const std::vector<Euler>& eulers) {
    std::vector<Quaternion> result;
    result.reserve(eulers.size());
    result.push_back(eulerToQuat(eulers[0]));
    for (size_t i = 1; i < eulers.size(); ++i) {
        Quaternion q1 = eulerToQuat(eulers[i]);
        Quaternion q2 = Quaternion(-q1.x, -q1.y, -q1.z, -q1.w);
        // 连续性：选离上一帧更近的符号（q 与 -q 同旋转）
        result.push_back(dot(result[i-1], q1) >= dot(result[i-1], q2) ? q1 : q2);
    }
    return result;
}
```

> **审计修正**：原方案有一段 `angle > M_PI` 的"跳变检测"——因 `2·acos(|dot|) ∈ [0, π]`，该分支**永远不触发**（死代码），且与符号选择逻辑矛盾，已删除。符号选择已保证最短路径。

### 物理积分：指数映射（修正审计 Bug 2）

```cpp
// 一阶指数映射（严格李群积分）
Quaternion integrate(Quaternion q, Vector3 omega, float dt) {
    float halfAngle = length(omega) * dt * 0.5f;
    if (halfAngle < 1e-6f) return q;
    Vector3 axis = normalized(omega);
    Quaternion dq = axisAngle(axis, halfAngle * 2.0f); // 指数映射 exp(0.5·ω·dt)
    return (dq * q).normalized();
}
```

> **审计修正**：原方案把"欧拉半步 + 归一化"标榜为 RK4——四元数不是向量空间，不能直接 `q + dq·dt` 当 RK4。正确做法：
>
> - **方案 A（严格）**：切空间做经典 RK4（k1..k4 是 ω 向量），再用指数映射 `exp(0.5·Ω·dt) ⊗ q`
> - **方案 B（工程常用）**：向量空间 RK4 + 归一化投影（精度略低但合法）
>   高阶版本落实现时用方案 A 或 B，不能混称。

### FPS 相机：四元数累加器（无奇点）

```cpp
class FirstPersonCamera {
    Quaternion orientation;
public:
    void rotate(float mouseDX, float mouseDY, float sensitivity) {
        Quaternion dYaw   = axisAngle(Vector3::up(), -mouseDX * sensitivity);
        Vector3 localRight = orientation * Vector3::right();
        Quaternion dPitch = axisAngle(localRight, -mouseDY * sensitivity);
        orientation = (dYaw * orientation).normalized();
        orientation = (dPitch * orientation).normalized();
    }
};
```

> 增量旋转永远小、永远在 S³ 上累积，看天/看地/倒飞全程无奇点。

### 网络同步：3 分量 + 连续性重建（含丢包兜底）

```cpp
struct NetRot { float x, y, z; };

Quaternion reconstruct(const NetRot& net, const Quaternion& prev) {
    float wSq = 1.0f - net.x*net.x - net.y*net.y - net.z*net.z;
    float w = wSq > 0.0f ? sqrt(wSq) : 0.0f;
    Quaternion q1(net.x, net.y, net.z,  w);
    Quaternion q2(net.x, net.y, net.z, -w);
    return (dot(prev, q1) >= dot(prev, q2)) ? q1 : q2;
}
```

> **审计补充（丢包兜底）**：符号选择依赖 prev 连续性，丢包/乱序会选错符号（跳 360°）。兜底策略：
>
> 1. **增量编码**：只发相对上一帧的 delta 旋转，周期性发全量帧
> 2. **w 接近 0 的数值兜底**：旋转接近 180° 时 wSq 可能为负（量化误差平方放大），已用 `wSq > 0 ? sqrt : 0` 防崩溃
> 3. 高精度场景可退化为发 4 分量量化

### quat → 欧拉角输出（审计补充：缺失项）

反向转换在 pitch=±90° 同样奇异（yaw/roll 不可唯一分解）。输出约定：

- 固定提取顺序 ZYX
- 奇点处用连续性锚定（上一帧欧拉角作参考选最近解）
- 仅供编辑器/调试/日志，永不作为内部状态

```cpp
// 约定：ZYX 顺序提取
Euler quatToEuler(const Quaternion& q); // 实现见 TransformTools（M7）
```

## 用户接口

```cpp
// 游戏脚本（Lua 绑定）
actor:setEulerAngles(0, 90, 0)            // 欧拉输入（边界转换）
actor:rotateTowards(target, maxAngle)     // S³ 最短路径
camera:lookAt(position, up)
quat = Quat.slerp(a, b, t, easing.quadOut)

// 编辑器：显示欧拉曲线，内部已是 S³ 最短路径（连续性层已修复）
```

## 分层归属

| 能力                                | 模块                               | 状态      |
| ----------------------------------- | ---------------------------------- | --------- |
| 四元数/slerp/axisAngle/lookRotation | `core/math/Quaternion.h`（M4）     | ✅ 已实现 |
| 旋转矩阵/变换矩阵                   | `core/math/Matrix4x4.h`（M5）      | ✅ 已实现 |
| 法线矩阵/2D 变换                    | `core/math/Matrix3x3.h`（M6）      | 🔄 待实现 |
| quat↔euler 输出、TRS、lookAt 封装   | `core/math/TransformTools.h`（M7） | 🔄 待实现 |
| 欧拉→S³ 连续性缝合                  | 连续性层（游戏层/动画模块）        | 📋 规划   |
| 指数映射物理积分                    | 物理模块                           | 📋 规划   |
| 网络 3 分量重建                     | 网络模块                           | 📋 规划   |
| Transform 组件                      | ECS（#11）                         | 📋 规划   |
