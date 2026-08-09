# M1 — 常量与标量工具（Math.h）

**领域**：数学库基础层

## 关联

- **依赖**：无（最底层）
- **被依赖**：M2-Vector3、M3-Vector2、M4-Quaternion、M5-Matrix4x4、M6-Matrix3x3、M7-TransformTools

## 目标

提供数学库共享的常量（π、ε、角度转换）和标量工具（clamp/lerp/平滑），所有其他数学类型复用，避免重复定义。

## 验收标准

- [ ] `constexpr double PI = 3.14159265358979323846;` 及 `TWO_PI`/`HALF_PI`
- [ ] `constexpr float EPSILON = 1e-6f;`
- [ ] `constexpr double deg2rad(deg)` / `rad2deg(rad)`（constexpr）
- [ ] `T clamp(T value, T min, T max)`（模板，constexpr）
- [ ] `T lerp(T a, T b, float t)`（模板，constexpr）
- [ ] `bool approxEqual(a, b, eps)`（浮点比较，默认 EPSILON）
- [ ] 命名空间 `lCYC::math::`，无 Boost 依赖
- [ ] 全部 `constexpr` 可在编译期求值（static_assert 验证）

## 性能

- 全部 `constexpr` + `inline`，零运行时开销
- 无堆分配、无虚函数

## 扩展性

- 模板化 clamp/lerp 支持任意数值类型（int/float/double）
- 新增常量只需在 Math.h 一处添加

## 安全

- clamp 处理 min>max 的防御（clamp 结果在 [min,max] 内）
- 浮点比较用 epsilon 而非 `==`，避免 NaN 传播问题
