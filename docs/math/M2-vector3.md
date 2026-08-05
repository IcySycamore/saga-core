# M2 — Vector3（三维向量）

**领域**：数学库核心

## 关联

- **依赖**：M1-Math.h
- **被依赖**：M4-Quaternion、M5-Matrix4x4、M7-TransformTools、ECS TransformComponent（#11）

## 目标

提供完整的 3D 向量类型，覆盖位置/方向/距离计算，是连续三维空间的基础原语。

## 验收标准

- [ ] `struct Vector3 { float x, y, z; }`（`constexpr` 构造，默认 `{0,0,0}`）
- [ ] 运算符：`+` `-` `*`(标量) `/`(标量) `+=` `-=` `*=` `/=` `-`(取反) `==` `!=`
- [ ] `dot(a,b)` 点积、`cross(a,b)` 叉积
- [ ] `length()` / `lengthSquared()` / `distance(a,b)`
- [ ] `normalized()` / `normalize()`（零向量安全：返回零，不产生 NaN）
- [ ] `clampMagnitude(v, maxLen)`
- [ ] `angle(a,b)`（弧度）、`reflect(v, normal)`
- [ ] 静态常量：`zero`/`one`/`up`/`down`/`left`/`right`/`forward`/`back`
- [ ] **语义澄清**：`one()` = 全 1 向量（非单位向量，长度 √3）；单位向量用 `up`/`down`/`left`/`right`/`forward`/`back`
- [ ] `toString()` 便于日志调试
- [ ] 命名空间 `math::`

## 性能

- 所有方法 `constexpr` + `inline`，无堆分配
- 热路径（dot/cross/length）无分支或最少分支
- 结构体大小 = 12 字节（3×float），可 SIMD 友好对齐

## 扩展性

- 提供 `float` 版本（渲染精度足够）；未来如需 `double` 可模板化 `Vector<T>`
- 组件（Transform）通过语义枚举引用 Vector3，不绑定

## 安全

- 归一化零向量 → 返回 `{0,0,0}`（不产生 NaN/Inf），并在 debug 下 assert
- 索引访问 `operator[]` 用 `assert(index < 3)`
- 所有运算避免未定义行为；叉积结果与输入正交（数学正确性测试）
