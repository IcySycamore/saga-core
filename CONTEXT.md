# TRPG 游戏引擎

一个 TRPG 网络游戏服务器项目（C++20），目标是未来复用为通用游戏引擎。核心原则：引擎层（可复用）与游戏层（具体逻辑）分离。

## Language

**EntityInstance**:
一个实体的运行时实例，持有一组动态组件，通过 uuid 唯一标识，由 EntityManager 唯一拥有。适用于物品、生物等所有实体。
_Avoid_: ItemEntity、Item、Entity（单指实例时）

**EntityArcheType**:
实体的类型模板（静态定义），持有静态组件和 defaults 初始值。实例通过 type*id 引用其模板。
\_Avoid*: ItemTemplate、ItemDef

**EntityManager**:
实例池的唯一所有者，管理类型模板、实例池、代表物注册表与逻辑分发。
_Avoid_: ItemFactory、ItemRegistry

**StaticComponent**:
只在类型模板上存在、所有该类型实例共享的不可变组件（值、标签、向量）。
_Avoid_: Static part

**DynamicComponent**:
只在实例上存在、每个实例独立可变状态的组件（计数器、计数器向量）。
_Avoid_: Dynamic part

**ItemSlot**:
轻量槽对象，持有裸指针 + 堆叠数，不参与生命周期，通过 onAttach/onDetach 维护引用计数。
_Avoid_: InventorySlot、BagSlot

**Inventory**:
游戏层容器，持有一组 ItemSlot，负责物品的存取与堆叠逻辑。

**ItemSemantic**:
游戏层物品分类（消耗/堆叠语义：UNC/SNC/SUC/MUC），定义于 `Inventory.h`。不属于引擎层。
_Avoid_: Semantic（当指物品分类时）

**代表物 (Representative)**:
一个 type*id 的共享实例，所有该类型的非唯一物品共享它，uuid 稳定可持久化。判断方法：实例的组件容器为空。生物等有运行时组件的类型不会命中该优化。
\_Avoid*: Prototype、Default instance

**Handler**:
唯一处理者，1:1 绑定某个语义/逻辑点，同一时间只有一个 handler 生效。
_Avoid_: Listener、Callback

**回调/信号 (Signal)**:
多播通知，1:N 广播给所有订阅者。与 handler 不同，回调不提供返回值语义。
_Avoid_: Event（指事件对象时）、Handler

**Connection**:
信号与槽之间的独立连接对象，1:1 连接线（编译期绑定）。全局/命名空间作用域声明，生命周期比所有绑定它的 Subscriber 长。信号函数与槽互不引用对方，只通过 Connection 中转。
_Avoid_: Bus、Channel、Signal（当指连接线时）

**Subscriber（槽）**:
接收方持有的可调用对象，构造时绑定到 Connection，析构时自动解绑（connection 失效）。槽逻辑 = 本类成员函数（`this->` 显式调用）。
_Avoid_: Listener、Slot（当指成员函数名时）

**PUBLISHER / SUBSCRIBER 宏**:
回调组件宏（`lCYC::callback`，定义于 `core/CallBack/`）。PUBLISHER 生成信号函数（转发到 Connection.emit，恒 void）；SUBSCRIBER 声明槽成员并绑定 Connection。多对一 = 多个 PUBLISHER 信号 → 同一 Connection → 一个槽。
_Avoid_: 带前缀的宏名（宏名永远不加前缀）

> **Connection 无宏**：Connection 用直接类型声明（`lCYC::callback::Connection<Sig>`，全局作用域），不提供 CONNECTION 宏。

**uuid**:
实例的稳定唯一标识，使用 boost::uuids::random*generator 生成，持久化后保持稳定。
\_Avoid*: id、index、key

**type_id**:
类型模板的 int32 标识，EntityManager 用它索引 EntityArcheType 与代表物。
_Avoid_: type、item_type

**semantic 键**:
组件在 map 中的 int32 键，表示组件语义。静态键与动态键使用不同的数值空间，实例查询用动态键。
_Avoid_: component id、slot

**Persistable**:
可被 JSON 序列化/反序列化的对象。当前使用可读 JSON（权宜之计），未来切换二进制格式。
_Avoid_: Serializable（当强调未来二进制时）
