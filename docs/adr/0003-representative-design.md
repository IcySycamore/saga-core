# 代表物（Representative）设计

每个 type_id 存在一个共享实例（代表物），所有该类型的非唯一物品共享它。`createItemInstance(type_id)` 是统一入口，内部先查 `m_rep_type_2_uuid` 快速索引；若创建后实例的组件容器为空（`m_component.empty()`），判定为代表物并注册到 `m_rep_type_2_uuid`。

代表物 uuid 稳定且持久化，Inventory 槽统一存 uuid+count（不存 type_id）。

**Considered Options**: 为每个实例深拷贝组件（内存浪费，代表物存在的意义就是消除重复）；按 type_id 懒加载（需要额外生命周期管理）。

**Consequences**: 代表物不设防——把代表物 uuid 传给 destroyItem 属于调用方 bug（设计契约由调用者保证），不做运行时防护。
