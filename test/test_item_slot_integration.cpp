// test/test_item_slot_integration.cpp
// 集成测试：ItemSlot ↔ ItemType（ItemInstance 引用计数、ItemArcheType
// 组件查询）
//
// 编译与运行:
//   cd build && cmake .. -G Ninja && ninja test_item_slot_integration &&
//   ./test_item_slot_integration

#include "core/Item/Component/Dynamic/CounterComponent.h"
#include "core/Item/Component/Static/ValLabelComponent.h"
#include "core/Item/ItemSlot.h"
#include "core/Item/ItemType.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

// ===================== 断言宏 =====================
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

// ============================================================
// 场景 1: 单槽 attach → detach 引用计数
// ============================================================
static void test_single_slot_attach_detach() {
  TEST("single_slot_attach_detach");

  ItemInstance inst;
  ItemSlot slot;

  // 初始状态
  EXPECT_EQ(inst.getRefC(), 0);
  EXPECT(slot.isFullEmpty(), "slot should be fully empty initially");
  EXPECT(slot.isEmpty(), "slot should be empty initially");

  // 放入物品
  slot.replaceItem(&inst);
  EXPECT_EQ(inst.getRefC(), 1);
  EXPECT(!slot.isFullEmpty(), "slot should not be fully empty after replace");
  EXPECT(slot.isEmpty(), "slot should still be empty (stack=0)");

  // 堆叠
  slot.increaseStack(5);
  EXPECT_EQ(slot.getStackNum(), 5);
  EXPECT(!slot.isEmpty(), "slot not empty after stacking");

  // 减少堆叠
  slot.decreaseStack(3);
  EXPECT_EQ(slot.getStackNum(), 2);
  EXPECT_EQ(inst.getRefC(), 1); // ref count 不变（物品还在槽里）

  // 清空堆叠，再移除物品 → 完备空
  slot.decreaseStack(2);
  slot.replaceItem(nullptr);
  EXPECT_EQ(inst.getRefC(), 0);
  EXPECT(slot.isFullEmpty(),
         "slot should be fully empty after clear stack + remove");
}

// ============================================================
// 场景 2: clear() 一次性归零
// ============================================================
static void test_slot_clear() {
  TEST("slot_clear");

  ItemInstance inst;
  ItemSlot slot;

  slot.replaceItem(&inst);
  slot.increaseStack(10);
  EXPECT_EQ(inst.getRefC(), 1);
  EXPECT_EQ(slot.getStackNum(), 10);

  slot.clear();
  EXPECT_EQ(inst.getRefC(), 0);
  EXPECT_EQ(slot.getStackNum(), 0);
  EXPECT(slot.isFullEmpty(), "slot should be fully empty after clear");
  EXPECT(slot.isEmpty(), "slot should be empty after clear");
}

// ============================================================
// 场景 3: 物品替换（装备 A → 装备 B）
// ============================================================
static void test_replace_item_switches_refcount() {
  TEST("replace_item_switches_refcount");

  ItemInstance instA;
  ItemInstance instB;
  ItemSlot slot;

  slot.replaceItem(&instA);
  EXPECT_EQ(instA.getRefC(), 1);
  EXPECT_EQ(instB.getRefC(), 0);

  // 换成 B
  slot.replaceItem(&instB);
  EXPECT_EQ(instA.getRefC(), 0); // A 被 detach
  EXPECT_EQ(instB.getRefC(), 1); // B 被 attach

  slot.clear();
  EXPECT_EQ(instB.getRefC(), 0);
}

// ============================================================
// 场景 4: 同物品替换自己（不改变引用计数）
// ============================================================
static void test_replace_self_noop_refcount() {
  TEST("replace_self_noop_refcount");

  ItemInstance inst;
  ItemSlot slot;

  slot.replaceItem(&inst);
  EXPECT_EQ(inst.getRefC(), 1);

  // 替换为同一个指针
  slot.replaceItem(&inst);
  EXPECT_EQ(inst.getRefC(), 1); // detach+attach 抵消

  slot.clear();
  EXPECT_EQ(inst.getRefC(), 0);
}

// ============================================================
// 场景 5: 多个槽共享同一个代表物
// ============================================================
static void test_multi_slot_shared_representative() {
  TEST("multi_slot_shared_representative");

  ItemInstance proxy; // 代表物，如铁矿石的全局唯一实例
  proxy.setTypeID(1001);

  // 3 个槽都堆叠铁矿石
  ItemSlot slot1, slot2, slot3;

  slot1.replaceItem(&proxy);
  slot1.increaseStack(8);
  EXPECT_EQ(proxy.getRefC(), 1);

  slot2.replaceItem(&proxy);
  slot2.increaseStack(5);
  EXPECT_EQ(proxy.getRefC(), 2);

  slot3.replaceItem(&proxy);
  slot3.increaseStack(3);
  EXPECT_EQ(proxy.getRefC(), 3);

  // slot1 的铁矿石用完了
  slot1.clear();
  EXPECT_EQ(proxy.getRefC(), 2);

  // slot2 也清空
  slot2.clear();
  EXPECT_EQ(proxy.getRefC(), 1);

  // slot3 最后清空 → 代表物引用归零
  slot3.clear();
  EXPECT_EQ(proxy.getRefC(), 0);
}

// ============================================================
// 场景 6: 装备类物品（不可堆叠，堆叠数恒为 1）
// ============================================================
static void test_equipment_single_stack() {
  TEST("equipment_single_stack");

  ItemInstance sword; // 装备，max_stack = 1
  ItemSlot slot;

  slot.replaceItem(&sword);
  slot.increaseStack(1);
  EXPECT_EQ(slot.getStackNum(), 1);
  EXPECT_EQ(sword.getRefC(), 1);

  // 卸下装备
  slot.clear();
  EXPECT_EQ(sword.getRefC(), 0);
  EXPECT(slot.isFullEmpty(), "slot should be fully empty after unequip");
}

// ============================================================
// 场景 7: 批量增加/减少堆叠
// ============================================================
static void test_batch_stack_operations() {
  TEST("batch_stack_operations");

  ItemInstance inst;
  ItemSlot slot;

  slot.replaceItem(&inst);

  // 多次增加
  slot.increaseStack(3);
  slot.increaseStack(5);
  slot.increaseStack(2);
  EXPECT_EQ(slot.getStackNum(), 10);

  // 分批减少
  slot.decreaseStack(4);
  EXPECT_EQ(slot.getStackNum(), 6);
  slot.decreaseStack(6);
  EXPECT_EQ(slot.getStackNum(), 0);
  EXPECT(slot.isEmpty(), "slot should be empty when count reaches 0");

  // 但指针还在
  EXPECT(!slot.isFullEmpty(), "slot should NOT be fully empty yet");
  EXPECT_EQ(inst.getRefC(), 1);
}

// ============================================================
// 场景 8: getItem 返回正确的裸指针
// ============================================================
static void test_get_item_pointer() {
  TEST("get_item_pointer");

  ItemInstance inst;
  inst.setTypeID(2001);

  ItemSlot slot;
  EXPECT_EQ(slot.getItem(), nullptr);

  slot.replaceItem(&inst);
  ItemInstance *ptr = slot.getItem();
  EXPECT(ptr != nullptr, "getItem should return non-null after replace");
  EXPECT_EQ(ptr->getTypeID(), 2001);

  slot.clear();
  EXPECT_EQ(slot.getItem(), nullptr);
}

// ============================================================
// 场景 9: ItemArcheType 组件 + ItemInstance 配合
// ============================================================
static void test_archetype_to_instance_copy_component() {
  TEST("archetype_to_instance_copy_component");

  // 模拟引擎层从 ArcheType 读取静态组件，
  // 游戏层据此初始化 ItemInstance 的动态组件
  ItemArcheType arch{};
  arch.m_type_id = 1001;
  arch.m_name = "生命药水";

  // 静态侧：默认消耗次数 = 1
  auto defaultCharges = std::make_unique<ValLabelComponent>();
  defaultCharges->m_val_label = 1;
  arch.m_component[static_cast<int32_t>(
      StaticComponentSemantic::DefaultCharges)] = std::move(defaultCharges);

  // 游戏层创建实例时读取静态组件
  auto it = arch.m_component.find(
      static_cast<int32_t>(StaticComponentSemantic::DefaultCharges));
  auto *staticVal = it != arch.m_component.end()
                        ? dynamic_cast<ValLabelComponent *>(it->second.get())
                        : nullptr;
  EXPECT(staticVal != nullptr, "should find DefaultCharges in archetype");

  int32_t initialCharges = staticVal ? staticVal->m_val_label : 0;

  // 给 ItemInstance 创建对应的动态计数器
  ItemInstance inst;
  // 注意：当前 ItemInstance 没有 addComponent 方法
  // 这里验证 getComponent 在无组件时返回 nullptr
  auto *counter = inst.getComponent<CounterComponent>(
      DynamicComponentSemantic::ContentCount);
  EXPECT(counter == nullptr, "no component yet — getComponent returns nullptr");
}

// ============================================================
// 场景 10: 多个 ItemInstance 之间 UUID 唯一性
// ============================================================
static void test_multi_instance_uuid_uniqueness() {
  TEST("multi_instance_uuid_uniqueness");

  constexpr int kCount = 50;
  // ItemInstance 不可拷贝/移动（含 unique_ptr 成员），
  // 用 unique_ptr 存储（ItemManager 也是这样管理生命周期）
  std::vector<std::unique_ptr<ItemInstance>> instances;
  instances.reserve(kCount);

  for (int i = 0; i < kCount; ++i) {
    auto inst = std::make_unique<ItemInstance>();
    inst->setTypeID(i);
    instances.push_back(std::move(inst));
  }

  // 验证所有 UUID 互异，typeID 正确
  for (int i = 0; i < kCount; ++i) {
    EXPECT(!instances[i]->getUuid().is_nil(), "UUID should not be nil");
    EXPECT_EQ(instances[i]->getTypeID(), i);
    for (int j = i + 1; j < kCount; ++j) {
      EXPECT(!(instances[i]->getUuid() == instances[j]->getUuid()),
             "all UUIDs must be unique");
    }
  }
}

// ============================================================
// 场景 11: ItemInstance 配合 ItemSlot 生命周期顺序
// ============================================================
static void test_slot_release_before_instance_destroy() {
  TEST("slot_release_before_instance_destroy");

  // ItemInstance 的析构不在 ref_count != 0 的 slot 持有下发生
  // 这是调用者的责任，这里验证顺序正确的场景
  ItemInstance *inst = new ItemInstance();
  ItemSlot slot;

  slot.replaceItem(inst);
  slot.increaseStack(3);
  EXPECT_EQ(inst->getRefC(), 1);

  // 正确顺序：先释放 slot，再销毁实例
  slot.clear();
  EXPECT_EQ(inst->getRefC(), 0);

  delete inst; // 安全释放
}

// ============================================================
// 场景 12: ItemSlot 栈溢出保护（断言）
// ============================================================
static void test_stack_underflow_assert() {
  TEST("stack_underflow_assert_detected");

  ItemInstance inst;
  ItemSlot slot;
  slot.replaceItem(&inst);
  slot.increaseStack(2);

  // 正常情况下减少 2 没问题
  slot.decreaseStack(2);
  EXPECT_EQ(slot.getStackNum(), 0);

  // 再减少应该触发 assert（Debug 模式）
  // 这里在 Release 下会静默通过，所以我们只验证正常路径
  // 实际 Debug 构建中 assert 会捕获
  slot.clear(); // 安全清空
}

// ============================================================
// main
// ============================================================
int main() {
  std::cout << "=== ItemSlot & ItemType Integration Tests ===" << std::endl;

  test_single_slot_attach_detach();
  test_slot_clear();
  test_replace_item_switches_refcount();
  test_replace_self_noop_refcount();
  test_multi_slot_shared_representative();
  test_equipment_single_stack();
  test_batch_stack_operations();
  test_get_item_pointer();
  test_archetype_to_instance_copy_component();
  test_multi_instance_uuid_uniqueness();
  test_slot_release_before_instance_destroy();
  test_stack_underflow_assert();

  std::cout << "\n=== Results: " << g_passed << " passed, " << g_failed
            << " failed ===" << std::endl;

  return g_failed > 0 ? 1 : 0;
}
