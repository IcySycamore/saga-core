# M5 — Matrix4x4（四阶矩阵）

**领域**：数学库（变换/投影）

## 关联

- **依赖**：M1-Math.h、M2-Vector3
- **被依赖**：M7-TransformTools、Camera 投影（#14）、渲染管线（#13/#16）

## 目标

提供 4x4 矩阵，覆盖模型/视图/投影变换与齐次坐标，是渲染与空间变换的核心。

## 验收标准

- [x] `struct Matrix4x4 { float m[16]; }`（**列主序 column-major，2026-08-06 确定**）
- [ ] 默认单位矩阵构造
- [ ] 运算符：`*`(矩阵乘) `*`(乘 Vector4/Vector3，齐次处理) `==`
- [ ] `identity()` 静态常量
- [ ] `transpose()` / `inverse()`（一般 4x4 逆，`determinant` 支持）
- [ ] `determinant()`
- [ ] 构造辅助：`translation(v)` / `scale(v)` / `rotation(q)`
- [ ] `orthographic(left,right,bottom,top,near,far)`
- [ ] `perspective(fovY, aspect, near, far)`（右手系）
- [ ] `lookAt(eye, target, up)`
- [ ] 数据访问：`operator[]`(行/列，约定明确) + `data()`（裸数组，供 GPU）
- [ ] 命名空间 `lCYC::math::`

## 存储约定（2026-08-06 拍板：列主序）

**列主序 column-major**：`m[col * 4 + row]`，即内存连续按列存放。

- 元素 `(row, col)` 位于 `m[col*4 + row]`
- 平移分量在 `m[12], m[13], m[14]`（最后一列）
- 匹配 OpenGL 习惯；`data()` 可直接传给 GPU（`glUniformMatrix4fv` 的 transpose=GL_FALSE）
- 与 M6（Matrix3x3）保持一致；`lookRotation` 已按此布局
- 矩阵乘语义：`A * B` = "先应用 B 再应用 A"（列向量约定：`v' = M * v`）

```
内存布局 (m[16]):
m[0] m[4] m[8]  m[12]   列0   列1   列2   列3
m[1] m[5] m[9]  m[13]   =    col0  col1  col2  col3
m[2] m[6] m[10] m[14]      (row, col) → m[col*4+row]
m[3] m[7] m[11] m[15]
```

## 性能

- 矩阵乘用循环展开或 `constexpr` 内联，避免运行时分支
- `inverse` 用伴随矩阵法（4x4 无 SIMD 也可接受），文档标注复杂度
- 结构体 64 字节，`data()` 返回连续内存（GPU 直接消费）

## 扩展性

- 列主序/行主序以 `static_assert` 或文档强制，切换后端（OpenGL 列主序 / D3D 行主序）只需转换函数
- 可加 `Matrix4x4d`（double 版）用于高精度模拟

## 安全

- `inverse()` 对不可逆矩阵（det≈0）防御：返回单位矩阵 + assert，避免除零
- 透视矩阵 near/far 非零验证
- 矩阵乘顺序约定明确（`A*B` = "先 B 后 A"，文档记录，防混淆）
- `operator[]` 边界 assert
