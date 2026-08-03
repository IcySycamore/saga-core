# Handler 与回调分离，自实现 Signal

信号语义明确二分：**handler = 唯一处理者（1:1）**，同一逻辑点同时只有一个 handler 生效，用于逻辑分发；**回调/信号 = 多播通知（1:N）**，广播给所有订阅者，用于事件通知。

**Considered Options**: boost::signals2（引入重型依赖）；统一为单一 handler 模型（无法表达多播需求）。

**Consequences**: 自实现约 30 行 `Signal<T>`（`vector<std::function>` 封装），不引入 boost::signals2。延迟型回调走 EventManager 队列（规划中）。
