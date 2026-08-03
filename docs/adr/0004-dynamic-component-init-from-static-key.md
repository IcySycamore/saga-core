# 动态组件初始值来自静态键引用

ItemArcheType 新增 `"dynamic"` 字段，值为 `{ 动态键 → 静态键 }` 映射，例如 `"dynamic": {"21": "121"}` 表示动态键 21 使用静态键 121 的类型和值作为默认值。

映射规则：ValLabel→Counter、ValVec→CounterVec；StrLabel 禁止映射（报错跳过）。

**Considered Options**: 实例创建时深拷贝静态组件（重复存储、语义混淆）；JSON 内直接写初始值（值重复，类型漂移）。

**Consequences**: 初始值属于类型配置域（arche），不属于实例域；ItemInstance 不持有静态组件，通过 getArche 读取。静态键/动态键数值空间分离，语义不重复，实例查询统一用动态键。
