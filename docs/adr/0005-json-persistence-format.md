# 持久化使用可读 JSON（未来切二进制）

持久化格式当前选择可读 JSON（boost::json），理由：调试友好、人类可读、开发期迭代快。这是**权宜之计**，未来切换到二进制格式。

两层持久化分离：EntityManager 存全量实例池 + 代表物注册表（单文件 `{ "instances": [...], "representatives": {...} }`，代表物不存 components 字段），Inventory 存槽（uuid+count，代表物和普通物品统一处理）。

加载策略：ArcheType 全部或零（双缓冲 temp map + swap）；Instance 尽力而为（跳过损坏条目）。API：`bool saveInstances(path)` / `bool loadInstances(path)`。

**Consequences**: JSON 可读性优先，格式与 Component 类通过外部分支（dynamic_cast）解耦，未来切换二进制只影响序列化层。
