# EntityManager 使用 unordered_map<uuid, unique_ptr> 存储实例池

EntityManager 持有全量实例池：`unordered_map<uuid, unique_ptr<EntityInstance>>`，uuid 是唯一键，map 是实例的唯一所有者。

**Considered Options**: vector + index（紧凑、迭代快，但删除需要 swap-and-pop 破坏引用稳定性）；unordered_map（点查 O(1)、删除 O(1) 且稳定）。

**Consequences**: 游戏场景以点查（按 uuid 找实例）为主，删除频繁，选择 map。ItemSlot 只持裸指针，生命周期完全由 EntityManager 保证。
