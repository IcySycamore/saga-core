// test/test_item_manager.cpp
// 编译与运行:
//   cd build && cmake .. -G Ninja && ninja && ctest -V
//
// 依赖: test/test_items.json（自动复制到 build/test/）

#include "core/Entity/Component/Dynamic/CounterComponent.h"
#include "core/Entity/Component/Static/ValLabelComponent.h"
#include "core/Entity/EntityManager.h"
#include "core/Entity/EntityType.h"
#include "core/Entity/ItemSlot.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// ===================== 轻量断言宏 =====================
static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) std::cout << "\n[ RUN ] " << name << std::endl
#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "  FAILED: " << msg << std::endl;                           \
      ++g_failed;                                                              \
    } else {                                                                   \
      ++g_passed;                                                              \
    }                                                                          \
  } while (false)
#define EXPECT_EQ(a, b) EXPECT((a) == (b), #a " == " #b)

// ===================== 测试 1: 加载有效 JSON =====================
static void test_load_valid_json() {
  TEST("load_valid_json");

  auto &mgr = EntityManager::getManager();
  bool ok = mgr.initArche("test/test_items.json");
  EXPECT(ok, "initArche should succeed with valid JSON");

  // 验证 2 个物品都加载了
  const auto *arch = mgr.getArche(4001);
  EXPECT(arch != nullptr, "should find type_id 4001");
  if (arch) {
    EXPECT_EQ(arch->m_type_id, 4001);
    EXPECT_EQ(arch->m_name, "阿司匹林");
    // components[121] → ValLabel(3)
    auto it = arch->m_component.find(121);
    EXPECT(it != arch->m_component.end(), "components should contain key 121");
    if (it != arch->m_component.end()) {
      auto *val = dynamic_cast<ValLabelComponent *>(it->second.get());
      EXPECT(val != nullptr, "components[121] should be ValLabelComponent");
      if (val)
        EXPECT_EQ(val->m_val_label, 3);
    }
    // defaults[30] → ValLabel(3)
    auto dit = arch->m_defaults.find(30);
    EXPECT(dit != arch->m_defaults.end(), "defaults should contain key 30");
    if (dit != arch->m_defaults.end()) {
      auto *val = dynamic_cast<ValLabelComponent *>(dit->second.get());
      EXPECT(val != nullptr, "defaults[30] should be ValLabelComponent");
      if (val)
        EXPECT_EQ(val->m_val_label, 3);
    }
    // #40: max[30] → ValLabel(10)
    auto mit = arch->m_maximums.find(30);
    EXPECT(mit != arch->m_maximums.end(), "max should contain key 30");
    if (mit != arch->m_maximums.end()) {
      auto *val = dynamic_cast<ValLabelComponent *>(mit->second.get());
      EXPECT(val != nullptr, "max[30] should be ValLabelComponent");
      if (val)
        EXPECT_EQ(val->m_val_label, 10);
    }
  }

  arch = mgr.getArche(4002);
  EXPECT(arch != nullptr, "should find type_id 4002");
  if (arch) {
    EXPECT_EQ(arch->m_name, "生命药水");
    // components[122] → ValLabel(30)
    auto it = arch->m_component.find(122);
    EXPECT(it != arch->m_component.end(), "components should contain key 122");
    if (it != arch->m_component.end()) {
      auto *val = dynamic_cast<ValLabelComponent *>(it->second.get());
      EXPECT(val != nullptr, "components[122] should be ValLabelComponent");
      if (val)
        EXPECT_EQ(val->m_val_label, 30);
    }
  }
}

// ===================== 测试 2: 查询不存在的类型 =====================
static void test_get_missing_type() {
  TEST("get_missing_type");

  auto &mgr = EntityManager::getManager();
  const auto *arch = mgr.getArche(99999);
  EXPECT(arch == nullptr, "non-existent type_id should return nullptr");
}

// ===================== 测试 3: reload 清除旧数据 =====================
static void test_reload() {
  TEST("reload_clears_old_data");

  auto &mgr = EntityManager::getManager();
  bool ok = mgr.reload("test/test_items.json");
  EXPECT(ok, "reload should succeed");

  // 之前加载的数据应该还在（同文件 reload）
  EXPECT(mgr.getArche(4001) != nullptr,
         "after reload, 4001 should still exist");
  EXPECT(mgr.getArche(4002) != nullptr,
         "after reload, 4002 should still exist");
}

// ===================== 测试 4: createInstance 动态组件填充
// =====================
static void test_create_instance() {
  TEST("create_item_instance_dynamic_defaults");

  auto &mgr = EntityManager::getManager();
  mgr.reload("test/test_items.json");

  auto inst = mgr.createInstance(4001);
  EXPECT(inst != nullptr,
         "createInstance should return non-null for valid type_id");
  if (inst) {
    EXPECT_EQ(inst->getTypeID(), 4001);
    EXPECT(!inst->getUuid().is_nil(), "new instance should have non-nil UUID");
    // defaults[30] ValLabel(3) → Counter(3)
    auto *counter = inst->getComponent<CounterComponent>(30);
    EXPECT(counter != nullptr, "instance should have CounterComponent at 30");
    if (counter)
      EXPECT_EQ(counter->getCounter(), 3);
    // 实例不应持有静态组件（静态组件属于 arche）
    EXPECT(inst->getComponent<ValLabelComponent>(30) == nullptr,
           "instance should not hold ValLabelComponent");
  }

  // 4002: defaults[30]→Counter(3)、defaults[40]→Counter(30)
  auto inst2 = mgr.createInstance(4002);
  EXPECT(inst2 != nullptr, "create 4002 should succeed");
  if (inst2) {
    auto *c30 = inst2->getComponent<CounterComponent>(30);
    EXPECT(c30 != nullptr, "4002 should have Counter at 30");
    if (c30)
      EXPECT_EQ(c30->getCounter(), 3);
    auto *c40 = inst2->getComponent<CounterComponent>(40);
    EXPECT(c40 != nullptr, "4002 should have Counter at 40");
    if (c40)
      EXPECT_EQ(c40->getCounter(), 30);
  }

  auto inst_null = mgr.createInstance(99999);
  EXPECT(inst_null == nullptr,
         "createInstance should return null for missing type");
}

// ===================== 测试 5: 池内查询与销毁 =====================
static void test_get_and_destroy() {
  TEST("get_and_destroy_instance");

  auto &mgr = EntityManager::getManager();
  mgr.reload("test/test_items.json");

  auto inst = mgr.createInstance(4001);
  EXPECT(inst != nullptr, "create 4001 should succeed");
  if (inst) {
    auto uuid = inst->getUuid();
    auto *found = mgr.getInstance(uuid);
    EXPECT(found == inst, "getInstance should return the same instance");

    bool ok = mgr.destroyInstance(uuid);
    EXPECT(ok, "destroy should succeed");
    EXPECT(mgr.getInstance(uuid) == nullptr,
           "destroyed instance should be gone from pool");
    // 重复销毁失败
    EXPECT(!mgr.destroyInstance(uuid), "double destroy should fail");
  }
}

// ===================== 测试 6: 注册与获取 Handler =====================
static void test_handler_register() {
  TEST("handler_register_and_get");

  auto &mgr = EntityManager::getManager();

  bool called = false;
  mgr.regHandler(4001, [&called](EntityInstance &, const EntityArcheType &) {
    called = true;
  });

  auto handler = mgr.getHandler(4001);
  EXPECT(handler != nullptr, "handler should be registered");

  // 调用一下
  if (handler) {
    EntityInstance inst;
    EntityArcheType arch{};
    handler(inst, arch);
    EXPECT(called, "handler should have been invoked");
  }

  auto no_handler = mgr.getHandler(99999);
  EXPECT(no_handler == nullptr, "unregistered handler should be nullptr");
}

// ===================== 测试 7: 持久化往返（ADR-0005） =====================
static void test_persistence_roundtrip() {
  TEST("persistence_roundtrip");

  auto &mgr = EntityManager::getManager();
  mgr.reload("test/test_items.json");

  auto inst = mgr.createInstance(4001);
  EXPECT(inst != nullptr, "create 4001 should succeed");
  if (!inst)
    return;
  const auto uuid = inst->getUuid();
  // 改默认值验证往返保留
  auto *counter = inst->getComponent<CounterComponent>(30);
  EXPECT(counter != nullptr, "4001 should have Counter at 30");
  if (counter)
    counter->setCounter(7);

  // 创建代表物（4003 无 defaults）→ 应写入 representatives（不存 components）
  auto *rep = mgr.createInstance(4003);
  EXPECT(rep != nullptr, "create representative 4003 should succeed");

  EXPECT(mgr.saveInstances("test/persist_tmp.json"),
         "saveInstances should succeed");

  // 验证代表物注册表已序列化（ADR-0005：representatives 字段含 4003）
  {
    std::ifstream f("test/persist_tmp.json");
    std::stringstream ss;
    ss << f.rdbuf();
    const std::string content = ss.str();
    EXPECT(content.find("\"representatives\"") != std::string::npos,
           "file should contain representatives field");
    EXPECT(content.find("4003") != std::string::npos,
           "representatives should reference type 4003");
  }

  EXPECT(mgr.loadInstances("test/persist_tmp.json"),
         "loadInstances should succeed");

  // 覆盖合并后 uuid 应稳定、组件值保留
  auto *loaded = mgr.getInstance(uuid);
  EXPECT(loaded != nullptr, "instance should be found after roundtrip");
  if (loaded) {
    EXPECT_EQ(loaded->getTypeID(), 4001);
    auto *lc = loaded->getComponent<CounterComponent>(30);
    EXPECT(lc != nullptr, "loaded Counter(30) should exist");
    if (lc)
      EXPECT_EQ(lc->getCounter(), 7);
  }
}

// ===================== 测试 8: 代表物共享（ADR-0003） =====================
// 依赖构建宏 REP_SEMANTIC_OPTIMIZATION（默认 ON）；关闭时每次独立实例
static void test_representative_shared() {
  TEST("representative_shared");

  auto &mgr = EntityManager::getManager();
  mgr.reload("test/test_items.json");

  // 4003 无 defaults（组件容器空）→ 应命中代表物共享
  auto *a = mgr.createInstance(4003);
  auto *b = mgr.createInstance(4003);
  EXPECT(a != nullptr && b != nullptr, "4003 should create instances");
  if (a && b) {
    EXPECT(a == b, "defaults-empty type should share representative instance");
  }
}

// ===================== 测试 9: defaults > max 拒绝加载（#40）
// =====================
static void test_defaults_exceed_max_rejected() {
  TEST("defaults_exceed_max_rejected");

  auto &mgr = EntityManager::getManager();
  bool ok = mgr.reload("test/test_items.json");
  EXPECT(ok, "baseline reload should succeed");

  // 写一个 defaults > max 的非法配置到临时文件
  {
    std::ofstream f("test/test_bad_max.json");
    f << R"({
  "archetypes": [
    {
      "type_id": 9999,
      "name": "非法",
      "components": {"121": 3},
      "defaults": {"30": 100},
      "max": {"30": 50}
    }
  ]
})";
  }

  bool bad = mgr.reload("test/test_bad_max.json");
  EXPECT(!bad, "reload with defaults>max should be rejected");
}

// ===================== 测试 10: reload 钳制存量实例（#40）
// =====================
static void test_reload_clamps_instances() {
  TEST("reload_clamps_over_limit_instances");

  auto &mgr = EntityManager::getManager();
  mgr.reload("test/test_items.json");

  // 创建实例并把计数器抬到超过 max（30 的上限 10）
  auto inst = mgr.createInstance(4001);
  EXPECT(inst != nullptr, "create 4001 should succeed");
  if (!inst)
    return;
  auto *counter = inst->getComponent<CounterComponent>(30);
  EXPECT(counter != nullptr, "4001 should have Counter at 30");
  if (counter)
    counter->setCounter(99); // 超过 max=10

  // reload 后应被钳制到 10
  bool ok = mgr.reload("test/test_items.json");
  EXPECT(ok, "reload should succeed");
  if (counter)
    EXPECT_EQ(counter->getCounter(), 10);
}

// ===================== main =====================
int main() {
  std::cout << "=== EntityManager Tests ===" << std::endl;

  test_load_valid_json();
  test_get_missing_type();
  test_reload();
  test_create_instance();
  test_get_and_destroy();
  test_handler_register();
  test_persistence_roundtrip();
  test_representative_shared();
  test_defaults_exceed_max_rejected();
  test_reload_clamps_instances();

  std::cout << "\n=== Results: " << g_passed << " passed, " << g_failed
            << " failed ===" << std::endl;

  return g_failed > 0 ? 1 : 0;
}
