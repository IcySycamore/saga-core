# CounterArrComponent 废弃，统一 CounterVecComponent

动态数组组件统一使用 `CounterVecComponent`（std::vector 版本）。早期固定 size=5 的 `CounterArrComponent`（std::array 版本）保留文件但标记 `[[deprecated]]`，加载时打日志 warning。

**Consequences**: 新代码一律使用 CounterVecComponent；旧 CounterArr 仅用于兼容既有存档读取。
