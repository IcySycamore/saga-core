# Handler 与回调分离，自实现 Signal

信号语义明确二分：**handler = 唯一处理者（1:1）**，同一逻辑点同时只有一个 handler 生效，用于逻辑分发；**回调/信号 = 多播通知（1:N）**，广播给所有订阅者，用于事件通知。

**Considered Options**: boost::signals2（引入重型依赖）；统一为单一 handler 模型（无法表达多播需求）。

**Consequences**: 自实现约 30 行 `Signal<T>`（`vector<std::function>` 封装），不引入 boost::signals2。延迟型回调走 EventManager 队列（规划中）。

## 补充（2026-08-14）：Connection 中转设计

落地时（`core/CallBack/`）进一步将"连接"独立为 `Connection` 对象：

- **Connection** = 信号与槽之间的 1:1 连接线，全局/命名空间作用域声明，生命周期比所有绑定它的 `Subscriber` 长
- **Subscriber（槽）** = 接收方持有的可调用对象，构造时绑定 Connection、析构时自动解绑（connection 失效），**绝不 delete Connection**（借用者不销毁拥有者）
- **多对一** = 多个 `PUBLISHER` 信号函数 → 同一 Connection → 一个槽；间接 1:1 由"一个槽只挂一个信号"实现
- 信号链单调递减（信号 → 连接 → 槽），不可能成环
- **编译期绑定**：无运行时 connect / 全局注册表，所有连接在编译期确定（与 Qt 的运行时 connect 不同，这是有意的取舍）
- 跨系统/跨线程/延迟广播仍留给 EventManager 队列
