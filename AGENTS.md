# AGENTS.md

Agent 约定文件：本仓库是 TRPG 游戏引擎（C++20），目标是未来复用为通用游戏引擎。核心原则：**引擎层（可复用）与游戏层（具体逻辑）分离**。

## 构建与测试

- 构建目录：`build/vscodeBuild/`（VS Code CMake Tools，Ninja），CLI 构建用 `build/`
- 测试可执行文件：`build/vscodeBuild/test_item_type.exe`、`test_item_slot_integration.exe`、`test_item_manager.exe`、`test_callback.exe`、`test_math.exe`、`test_time.exe`、`test_random.exe`、`test_render.exe`（均在 `build/vscodeBuild/`）
- 测试框架：自写轻量断言宏（TEST/EXPECT/EXPECT_EQ/EXPECT_THROWS），不用 gtest
- 测试独立于 trpg_core 库编译（只链接 Boost），避免受未完成代码影响
- Boost 1.91.0（json、uuid），`Boost_ROOT=C:/msys64/clang64`
- CMake 3.21+，策略 CMP0144/CMP0167 已置 NEW

## Agent skills

### Issue tracker

Issues and PRDs live as GitHub issues, managed via the `gh` CLI. See `docs/agents/issue-tracker.md`.

### Triage labels

Five canonical roles (`needs-triage` / `needs-info` / `ready-for-agent` / `ready-for-human` / `wontfix`), used verbatim. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `CONTEXT.md` at the repo root + `docs/adr/` for architecture decisions. See `docs/agents/domain.md`.

## 项目约定

- **分层**：`core/` 下引擎层（Component、EntityInstance、EntityArcheType、EntityManager、ItemSlot、引用计数）与游戏层（Inventory、Handler、代表物规则、ItemSemantic 物品分类）分离
- **命名**：EntityInstance/EntityArcheType/EntityManager（泛化自 Item 系，适用于物品/生物等所有实体）；CounterVecComponent 统一动态数组；CounterArrComponent 已废弃
- **所有权**：EntityManager 的 `unordered_map<uuid, unique_ptr<EntityInstance>>` 是唯一所有者；ItemSlot 只持裸指针 + 引用计数
- **信号语义**：handler = 1:1 唯一处理者；回调/信号 = 1:N 多播；不引入 boost::signals2
- **持久化**：JSON 可读格式（未来切二进制）；EntityManager 存实例池 + 代表物注册表，Inventory 存槽（uuid+count）
- **物品域概念**：ItemSemantic（消耗/堆叠分类）属于游戏层，定义于 `Inventory.h`；语义枚举（Static/DynamicComponentSemantic）是引擎层通用概念
- **返回值约定**：`void` = 总是成功（原子操作），`bool` = 可能失败
- 测试用词须与 `CONTEXT.md` 术语一致；改动前先读相关 ADR
