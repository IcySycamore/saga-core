# M7 — 组合工具（TransformTools）

**领域**：数学库收尾（组合/转换）

## 关联

- **依赖**：M1-Math.h、M2-Vector3、M4-Quaternion、M5-Matrix4x4、M6-Matrix3x3
- **被依赖**：TransformComponent（#11）、Camera（#14）、场景图（#15）

## 目标

提供高层组合工具：TRS 矩阵合成、欧拉角↔四元数↔矩阵互转、朝向计算，作为数学库的"应用层"入口，供 ECS 和渲染直接消费。

## 验收标准

- [ ] `Matrix4x4 trs(pos, rot, scale)`（TRS 合成，顺序 T*R*S 约定明确）
- [ ] `Matrix4x4 trs(pos, rot)`（默认 scale=1）
- [ ] 分解：`decomposeTRS(mat, pos, rot, scale)`（从矩阵还原，文档标注非唯一性）
- [ ] 欧拉↔四元数：`Quaternion eulerToQuat(x,y,z)` / `eulerToQuat(Vector3)` / `quatToEuler(q)`
- [ ] 欧拉↔矩阵：`Matrix4x4 eulerToMatrix(...)` / 从矩阵提取欧拉
- [ ] `Matrix4x4 lookAt(eye, target, up)` 封装（调用 M5）
- [ ] `Vector3 transformPoint(mat, v)` / `transformDirection(mat, v)`（明确区分：point 带平移，direction 不带）
- [ ] `Vector3 rotateVector(q, v)` 便捷封装
- [ ] 一致性测试：trs → decompose → 还原（容差内相等）
- [ ] 命名空间 `lCYC::math::`

## 性能

- 组合工具为"组装/分解"用途，非每帧热路径，但保持 `inline`
- `transformPoint`/`transformDirection` 是热路径（渲染每顶点），用内联 + 无分配

## 扩展性

- 未来 `Transform`（含父子层级）可直接用 trs/decompose 作为数学底座
- 可加惯性/插值工具（`slerp` 组合、`lerpTransform`）

## 安全

- `decomposeTRS` 对不可逆/奇异矩阵防御（返回 false 或 assert）
- `transformPoint` vs `transformDirection` 的语义在文档强约束（混用是经典 bug）
- 欧拉↔四元数转换明确旋转顺序约定（文档：如 ZYX），防混淆
- 所有转换往返测试（forward↔inverse 容差内一致）作为验收强制项
