# 引擎层与游戏层分离

项目目标是未来复用为通用游戏引擎，因此代码严格分为两层：引擎层（可复用的 Component 基类、EntityInstance、ItemSlot、引用计数）与游戏层（EntityArcheType、Inventory、EntityManager、Handler、代表物规则）。

游戏层依赖引擎层，引擎层不得反向依赖游戏层。引擎层只做机械映射（JSON 值类型 → Component 类），不理解语义含义。
