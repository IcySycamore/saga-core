# 序列化使用外部分支（dynamic_cast），Component 保持纯数据

Component 基类与序列化格式解耦：不引入虚序列化方法，而是通过外部分支（`dynamic_cast` 到具体组件类型）完成 JSON ↔ Component 转换。

引擎层默认映射规则（机械映射，不理解语义）：JSON int64→ValLabel/Counter、string→StrLabel、array→ValVec/CounterVec。未知 semantic 键：静默跳过 + 日志 warning。

**Considered Options**: 组件内部虚方法 serialize（组件依赖格式，违反纯数据原则）；type-erased 注册表（过度设计）。

**Consequences**: Component 保持纯数据可复用于任意格式；dynamic_cast 代价可接受（游戏帧内序列化不频繁）。
