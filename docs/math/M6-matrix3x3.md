# M6 — Matrix3x3（三阶矩阵）

**领域**：数学库（旋转/法线/2D 变换）

## 关联

- **依赖**：M1-Math.h、M2-Vector3、M3-Vector2
- **被依赖**：M7-TransformTools、法线变换（渲染）、2D 变换

## 目标

提供 3x3 矩阵，用于旋转矩阵、法线变换（逆转置）和 2D 仿射变换，是 4x4 的轻量补充。

## 验收标准

- [ ] `struct Matrix3x3 { float m[9]; }`（列主序或行主序，与 M5 约定一致）
- [ ] 默认单位矩阵构造
- [ ] 运算符：`*`(矩阵乘) `*`(乘 Vector3) `==`
- [ ] `identity()` 静态常量
- [ ] `transpose()` / `inverse()` / `determinant()`
- [ ] 构造辅助：`rotationX/Y/Z(angle)` / `rotation(q)` / `scale(v)`
- [ ] `fromMatrix4x4()` / `toMatrix4x4()`（提取旋转/缩放部分，去平移）
- [ ] `normalMatrix()`（= 逆转置，用于法线变换）
- [ ] 2D：`rotation2D(angle)` / `translation2D(x,y)`（3x3 齐次，与 M3 配合）
- [ ] 命名空间 `lCYC::math::`

## 性能

- 3x3 运算比 4x4 便宜（少 25% 数据），法线变换热路径用 3x3
- 全部 `constexpr` + `inline`，无堆分配

## 扩展性

- 与 Matrix4x4 双向转换（统一约定）
- 可加 `Matrix2x2`（若需要，2D 纯旋转场景）

## 安全

- `inverse()` 对 det≈0 防御（返回单位 + assert）
- `normalMatrix` 明确约定只作用于法线（不含平移），文档警示
- `operator[]` 边界 assert
- 2D 齐次坐标第三行约定明确（`{0,0,1}`）
