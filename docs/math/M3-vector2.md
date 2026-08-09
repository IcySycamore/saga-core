# M3 — Vector2（二维向量）

**领域**：数学库（2D 映射/UI/网格）

## 关联

- **依赖**：M1-Math.h
- **被依赖**：M6-Matrix3x3（2D 变换）、渲染的 2D 投影阶段（#9 之后）

## 目标

提供 2D 向量类型，支持 2D 映射、UI 坐标、纹理/网格 UV 等场景，与 Vector3 共享一致的 API 风格。

## 验收标准

- [ ] `struct Vector2 { float x, y; }`（`constexpr` 构造，默认 `{0,0}`）
- [ ] 运算符：`+` `-` `*`(标量) `/`(标量) `+=` `-=` `*=` `/=` `-`(取反) `==` `!=`
- [ ] `dot(a,b)`、`cross(a,b)`（返回标量，2D 叉积 = z 分量）
- [ ] `length()` / `lengthSquared()` / `distance(a,b)`
- [ ] `normalized()` / `normalize()`（零向量安全）
- [ ] `rotate(v, angleRad)`（2D 旋转）
- [ ] `perp(v)`（垂直向量，旋转 90°）
- [ ] 静态常量：`zero`/`one`/`up`/`down`/`left`/`right`
- [ ] **语义澄清**：`one()` = 全 1 向量（非单位向量，长度 √2）；单位向量用 `up`/`down`/`left`/`right`
- [ ] `Vector3` ↔ `Vector2` 转换（`toVector3(z)` / 显式构造）
- [ ] 命名空间 `lCYC::math::`

## 性能

- 全部 `constexpr` + `inline`，结构体 8 字节
- 无堆分配、无虚函数

## 扩展性

- 与 Vector3 保持同构 API（统一命名），后续 2D 渲染直接复用
- 可扩展为 `Vector2i`（整数版，网格/像素坐标）

## 安全

- 归一化零向量安全返回 `{0,0}`
- 与 Vector3 的转换显式（`toVector3(z)`），避免隐式丢失 z 的静默错误
