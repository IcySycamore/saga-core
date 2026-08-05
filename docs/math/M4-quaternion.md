# M4 — Quaternion（四元数）

**领域**：数学库（旋转）

## 关联

- **依赖**：M1-Math.h、M2-Vector3
- **被依赖**：M7-TransformTools、TransformComponent（#11）、Camera（#14）

## 目标

提供四元数旋转原语，避免欧拉角万向锁，用于实体朝向、相机旋转、插值旋转。

## 验收标准

- [ ] `struct Quaternion { float x, y, z, w; }`（默认单位四元数 `{0,0,0,1}`）
- [ ] 运算符：`*`(四元数乘，旋转合成) `*`(乘 Vector3，旋转向量) `==` `!=`
- [ ] `identity()` 静态常量
- [ ] `axisAngle(axis, angleRad)` 构造（归一化 axis，零轴安全）
- [ ] `euler(pitch, yaw, roll)` 构造（弧度）
- [ ] `toEuler()` / `toMatrix4x4()`（转换，在 M5 完成前可声明）
- [ ] `normalized()` / `normalize()`（零四元数安全）
- [ ] `inverse()` / `conjugate()`
- [ ] `dot(a,b)`（球面插值用）
- [ ] `slerp(a, b, t)`（球面线性插值，处理最短路径 + 双倍角）
- [ ] `lookRotation(forward, up)`（朝向，供 M7/Camera 用）
- [ ] 命名空间 `math::`

## 性能

- 全部 `constexpr` + `inline`，结构体 16 字节
- `slerp` 用 `std::acos` 但避免分支热点；normalize 在构造时保证
- 无堆分配

## 扩展性

- `slerp` 支持 `shortestPath` 参数扩展
- 与欧拉角/矩阵互转的完整闭环（M7 组合）

## 安全

- 归一化零四元数 → 返回单位四元数（避免 NaN）
- `axisAngle` 对零向量 axis 防御（assert + 返回单位）
- `lookRotation` 对 forward/up 平行（万向锁）情况的防御
- 所有旋转运算保持单位长度（内部 normalize 或断言）
