// test/test_item_manager.cpp
// 编译与运行:
//   cd build && cmake .. -G Ninja && ninja && ctest -V
//
// 依赖: test/test_items.json（自动复制到 build/test/）

#include "core/Item/ItemManager.h"
#include "core/Item/ItemSlot.h"
#include "core/Item/ItemType.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

// ===================== 轻量断言宏 =====================
static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) std::cout << "\n[ RUN      ] " << name << std::endl
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

  auto &mgr = ItemManager::getInstance();
  bool ok = mgr.initilizeArch("test/test_items.json");
  EXPECT(ok, "initilize should succeed with valid JSON");

  // 验证 4 个物品都加载了
  const auto *arch = mgr.getArcheType(1001);
  EXPECT(arch != nullptr, "should find type_id 1001");
  if (arch) {
    EXPECT_EQ(arch->m_type_id, 1001);
    EXPECT_EQ(arch->m_name, "生命药水");
    EXPECT_EQ(arch->m_max_stack_size, 10);
    EXPECT_EQ(arch->m_max_charges, 1);
    EXPECT_EQ(arch->m_price, 50);
    EXPECT_EQ(arch->m_val_1, 30); // 可选字段有值
    EXPECT_EQ(arch->m_val_2, 0);  // 可选字段无值 → 默认 0
  }

  arch = mgr.getArcheType(2001);
  EXPECT(arch != nullptr, "should find type_id 2001");
  if (arch) {
    EXPECT_EQ(arch->m_name, "魔法卷轴");
    EXPECT_EQ(arch->m_max_charges, 5);
  }

  arch = mgr.getArcheType(3001);
  EXPECT(arch != nullptr, "should find type_id 3001");
  if (arch) {
    EXPECT_EQ(arch->m_name, "铁剑");
    EXPECT_EQ(arch->m_max_charges, -1); // 非消耗品
    EXPECT_EQ(arch->m_val_1, 8);
  }

  arch = mgr.getArcheType(4001);
  EXPECT(arch != nullptr, "should find type_id 4001");
  if (arch) {
    EXPECT_EQ(arch->m_name, "解毒草");
    EXPECT_EQ(arch->m_val_1, 0); // JSON 没写 → 默认 0
    EXPECT_EQ(arch->m_val_2, 0);
  }
}

// ===================== 测试 2: 查询不存在的类型 =====================
static void test_get_missing_type() {
  TEST("get_missing_type");

  auto &mgr = ItemManager::getInstance();
  const auto *arch = mgr.getArcheType(99999);
  EXPECT(arch == nullptr, "non-existent type_id should return nullptr");
}

// ===================== 测试 3: reload 清除旧数据 =====================
static void test_reload() {
  TEST("reload_clears_old_data");

  auto &mgr = ItemManager::getInstance();
  bool ok = mgr.reload("test/test_items.json");
  EXPECT(ok, "reload should succeed");

  // 之前加载的数据应该还在（同文件 reload）
  EXPECT(mgr.getArcheType(1001) != nullptr,
         "after reload, 1001 should still exist");
  EXPECT(mgr.getArcheType(3001) != nullptr,
         "after reload, 3001 should still exist");
}

// ===================== 测试 4: createItemInstance =====================
static void test_create_instance() {
  TEST("create_item_instance");

  auto &mgr = ItemManager::getInstance();
  mgr.reload("test/test_items.json");

  auto inst = mgr.createItemInstance(1001);
  EXPECT(inst != nullptr,
         "createItemInstance should return non-null for valid type_id");
  if (inst) {
    EXPECT_EQ(inst->getTypeID(), 1001);
    // UUID 不应是 nil（已由构造函数自动分配）
    EXPECT(!inst->getUuid().is_nil(), "new instance should have non-nil UUID");
  }

  auto inst_null = mgr.createItemInstance(99999);
  EXPECT(inst_null == nullptr,
         "createItemInstance should return null for missing type");
}

// ===================== 测试 5: ItemSlot 增删 =====================
static void test_item_slot() {
  TEST("item_slot_add_remove");

  ItemSlot slot;
  EXPECT(!slot.isEmpty(), "new slot should not be full");

  bool ok = slot.add(3);
  EXPECT(ok, "add(3) should succeed");
  EXPECT(slot.isEmpty(), "slot should be full after adding items");

  ok = slot.add(1);
  EXPECT(!ok, "add(1) on full slot should fail");

  ok = slot.remove(2);
  EXPECT(ok, "remove(2) should succeed");
  EXPECT(slot.isEmpty(), "slot should still be full (3-2=1)");

  ok = slot.remove(1);
  EXPECT(ok, "remove(1) should succeed");
  EXPECT(!slot.isEmpty(), "slot should be empty after removing last item");
}

// ===================== 测试 6: 注册与获取 Handler =====================
static void test_handler_register() {
  TEST("handler_register_and_get");

  auto &mgr = ItemManager::getInstance();

  bool called = false;
  mgr.registerHandeler(1001, [&called](ItemInstance &, const ItemArcheType &,
                                       Entity &) { called = true; });

  auto handler = mgr.getHandler(1001);
  EXPECT(handler != nullptr, "handler should be registered");

  // 调用一下
  if (handler) {
    ItemInstance inst;
    ItemArcheType arch{};
    Entity e{10, 10, 10, 10, 10};
    handler(inst, arch, e);
    EXPECT(called, "handler should have been invoked");
  }

  auto no_handler = mgr.getHandler(99999);
  EXPECT(no_handler == nullptr, "unregistered handler should be nullptr");
}

// ===================== main =====================
int main() {
  std::cout << "=== ItemManager Tests ===" << std::endl;

  test_load_valid_json();
  test_get_missing_type();
  test_reload();
  test_create_instance();
  test_item_slot();
  test_handler_register();

  std::cout << "\n=== Results: " << g_passed << " passed, " << g_failed
            << " failed ===" << std::endl;

  return g_failed > 0 ? 1 : 0;
}
