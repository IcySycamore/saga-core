// test/test_item_type.cpp
// 编译与运行:
//   cd build && cmake .. -G Ninja && ninja && ctest -V
//
// 测试内容: ItemType.h, Component/Static/*, Component/Dynamic/*

#include "core/Item/Component/Dynamic/CounterArrComponent.h"
#include "core/Item/Component/Dynamic/CounterComponent.h"
#include "core/Item/Component/Dynamic/CounterVecComponent.h"
#include "core/Item/Component/Static/StrLabelComponent.h"
#include "core/Item/Component/Static/ValLabelComponent.h"
#include "core/Item/ItemType.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>


// ===================== 轻量断言宏 =====================
static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) std::cout << "\n[RUN] " << name << std::endl
#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << " FAILED: " << msg << std::endl;                           \
      ++g_failed;                                                              \
    } else {                                                                   \
      ++g_passed;                                                              \
    }                                                                          \
  } while (false)
#define EXPECT_EQ(a, b) EXPECT((a) == (b), #a " == " #b)
#define EXPECT_THROWS(expr, exc_type)                                          \
  do {                                                                         \
    bool caught = false;                                                       \
    try {                                                                      \
      (expr);                                                                  \
    } catch (const exc_type &) {                                               \
      caught = true;                                                           \
    } catch (...) {                                                            \
    }                                                                          \
    EXPECT(caught, #expr " should throw " #exc_type);                          \
  } while (false)

// ============================================================
// ItemArcheType 测试
// ============================================================
static void test_archetype_default() {
  TEST("ItemArcheType_default_values");

  ItemArcheType arch{};
  EXPECT_EQ(arch.m_type_id, 0);
  EXPECT(arch.m_name.empty(), "default name should be empty");
  EXPECT(arch.m_component.empty(), "default component map should be empty");
}

static void test_archetype_add_static_component() {
  TEST("ItemArcheType_add_static_component");

  ItemArcheType arch{};
  arch.m_type_id = 1001;
  arch.m_name = "测试物品";

  // 添加 StrLabelComponent
  auto label = std::make_unique<StrLabelComponent>();
  label->m_str_label = "品质: 传说";
  arch.m_component[11] = std::move(label); // AliasName_0 = 11

  EXPECT(!arch.m_component.empty(), "component map should not be empty");
  EXPECT(arch.m_component.find(11) != arch.m_component.end(),
         "should find component at index 11");

  auto *found = arch.m_component[11].get();
  auto *strComp = dynamic_cast<StrLabelComponent *>(found);
  EXPECT(strComp != nullptr, "should dynamic_cast to StrLabelComponent");
  if (strComp) {
    EXPECT_EQ(strComp->m_str_label, "品质: 传说");
  }
}

static void test_archetype_add_val_label_component() {
  TEST("ItemArcheType_add_val_label_component");

  ItemArcheType arch{};
  auto val = std::make_unique<ValLabelComponent>();
  val->m_val_label = 100;
  arch.m_component[21] = std::move(val); // DefaultCharges = 21

  auto *found = arch.m_component[21].get();
  auto *valComp = dynamic_cast<ValLabelComponent *>(found);
  EXPECT(valComp != nullptr, "should dynamic_cast to ValLabelComponent");
  if (valComp) {
    EXPECT_EQ(valComp->m_val_label, 100);
  }
}

// ============================================================
// ItemInstance 基础测试
// ============================================================
static void test_instance_default_construction() {
  TEST("ItemInstance_default_construction");

  ItemInstance inst;
  EXPECT(!inst.getUuid().is_nil(), "UUID should not be nil");
  EXPECT_EQ(inst.getTypeID(), 0);
  EXPECT_EQ(inst.getRefC(), 0);
}

static void test_instance_uuid_unique() {
  TEST("ItemInstance_uuid_unique");

  ItemInstance inst1;
  ItemInstance inst2;
  EXPECT(!(inst1.getUuid() == inst2.getUuid()),
         "two instances should have different UUIDs");
  EXPECT(!inst1.getUuid().is_nil(), "inst1 UUID should not be nil");
  EXPECT(!inst2.getUuid().is_nil(), "inst2 UUID should not be nil");
}

static void test_instance_set_get_type_id() {
  TEST("ItemInstance_set_get_type_id");

  ItemInstance inst;
  EXPECT_EQ(inst.getTypeID(), 0);

  inst.setTypeID(1001);
  EXPECT_EQ(inst.getTypeID(), 1001);

  inst.setTypeID(9999);
  EXPECT_EQ(inst.getTypeID(), 9999);
}

static void test_instance_ref_count() {
  TEST("ItemInstance_ref_count");

  ItemInstance inst;
  EXPECT_EQ(inst.getRefC(), 0);

  inst.onAttach();
  EXPECT_EQ(inst.getRefC(), 1);

  inst.onAttach();
  EXPECT_EQ(inst.getRefC(), 2);

  inst.onDetach();
  EXPECT_EQ(inst.getRefC(), 1);

  inst.onDetach();
  EXPECT_EQ(inst.getRefC(), 0);
}

static void test_instance_non_copyable() {
  TEST("ItemInstance_non_copyable");

  // 编译期检查：ItemInstance 不可拷贝
  bool not_copyable = !std::is_copy_constructible_v<ItemInstance>;
  EXPECT(not_copyable, "ItemInstance should NOT be copy constructible");
}

// ============================================================
// ItemInstance getComponent 测试
// ============================================================
static void test_get_component_not_found_int32() {
  TEST("getComponent_int32_not_found");

  ItemInstance inst;
  auto *ptr = inst.getComponent<CounterComponent>(999);
  EXPECT(ptr == nullptr,
         "getComponent with no matching index should return nullptr");
}

static void test_get_component_not_found_enum() {
  TEST("getComponent_enum_not_found");

  ItemInstance inst;
  auto *ptr = inst.getComponent<CounterComponent>(
      DynamicComponentSemantic::ContentCount);
  EXPECT(ptr == nullptr,
         "getComponent with no matching semantic should return nullptr");
}

static void test_get_component_wrong_type() {
  TEST("getComponent_wrong_type");

  // 注意：ItemInstance 当前没有 addComponent 方法，
  // 以下测试验证不存在的 component 返回 nullptr
  ItemInstance inst;
  auto *ptr = inst.getComponent<CounterComponent>(
      static_cast<int32_t>(DynamicComponentSemantic::ContentCount));
  EXPECT(ptr == nullptr, "non-existent component should return nullptr");

  auto *ptr2 = inst.getComponent<CounterArrComponent>(
      static_cast<int32_t>(DynamicComponentSemantic::Rule_arr));
  EXPECT(ptr2 == nullptr, "non-existent component should return nullptr");
}

// ============================================================
// CounterComponent 测试
// ============================================================
static void test_counter_component_default() {
  TEST("CounterComponent_default");

  CounterComponent cc;
  EXPECT_EQ(cc.getCounter(), 0);
}

static void test_counter_component_operations() {
  TEST("CounterComponent_operations");

  CounterComponent cc;

  cc.increase();
  EXPECT_EQ(cc.getCounter(), 1);

  cc.increase();
  cc.increase();
  EXPECT_EQ(cc.getCounter(), 3);

  cc.decrease();
  EXPECT_EQ(cc.getCounter(), 2);

  cc.setCounter(42);
  EXPECT_EQ(cc.getCounter(), 42);

  cc.clear();
  EXPECT_EQ(cc.getCounter(), 0);
}

static void test_counter_component_negative() {
  TEST("CounterComponent_negative");

  CounterComponent cc;
  cc.decrease(); // 0 -> -1
  EXPECT_EQ(cc.getCounter(), -1);

  cc.decrease();
  cc.decrease();
  EXPECT_EQ(cc.getCounter(), -3);
}

// ============================================================
// CounterArrComponent 测试
// ============================================================
static void test_counter_arr_default() {
  TEST("CounterArrComponent_default");

  CounterArrComponent cac;
  EXPECT_EQ(cac.size(), 5);

  // 默认全部为 0
  for (size_t i = 0; i < cac.size(); ++i) {
    EXPECT_EQ(cac.get(i), 0);
  }
}

static void test_counter_arr_operations() {
  TEST("CounterArrComponent_operations");

  CounterArrComponent cac;

  cac.set(0, 10);
  EXPECT_EQ(cac.get(0), 10);

  cac.increase(0);
  EXPECT_EQ(cac.get(0), 11);

  cac.decrease(0);
  EXPECT_EQ(cac.get(0), 10);

  // 最后一个元素
  cac.set(4, 99);
  EXPECT_EQ(cac.get(4), 99);
}

static void test_counter_arr_reset() {
  TEST("CounterArrComponent_reset");

  CounterArrComponent cac;
  cac.set(0, 1);
  cac.set(1, 2);
  cac.set(2, 3);
  cac.set(3, 4);
  cac.set(4, 5);

  cac.resetAll();
  for (size_t i = 0; i < cac.size(); ++i) {
    EXPECT_EQ(cac.get(i), 0);
  }
}

static void test_counter_arr_out_of_range() {
  TEST("CounterArrComponent_out_of_range");

  CounterArrComponent cac;
  EXPECT_THROWS(cac.get(5), std::out_of_range);
  EXPECT_THROWS(cac.set(5, 10), std::out_of_range);
  EXPECT_THROWS(cac.increase(5), std::out_of_range);
  EXPECT_THROWS(cac.decrease(5), std::out_of_range);
}

static void test_counter_arr_get_array() {
  TEST("CounterArrComponent_get_array");

  CounterArrComponent::ArrayType vals = {1, 2, 3, 4, 5};
  CounterArrComponent cac(vals);

  const auto &arr = cac.getArray();
  EXPECT_EQ(arr[0], 1);
  EXPECT_EQ(arr[1], 2);
  EXPECT_EQ(arr[2], 3);
  EXPECT_EQ(arr[3], 4);
  EXPECT_EQ(arr[4], 5);
}

// ============================================================
// CounterVecComponent 测试
// ============================================================
static void test_counter_vec_default() {
  TEST("CounterVecComponent_default");

  CounterVecComponent cvc;
  EXPECT_EQ(cvc.size(), 0);
}

static void test_counter_vec_sized_constructor() {
  TEST("CounterVecComponent_sized_constructor");

  CounterVecComponent cvc(5);
  EXPECT_EQ(cvc.size(), 5);
  for (size_t i = 0; i < cvc.size(); ++i) {
    EXPECT_EQ(cvc.getCounter(static_cast<int32_t>(i)), 0);
  }
}

static void test_counter_vec_resize() {
  TEST("CounterVecComponent_resize");

  CounterVecComponent cvc(3);
  EXPECT_EQ(cvc.size(), 3);

  cvc.resize(10);
  EXPECT_EQ(cvc.size(), 10);

  cvc.resize(1);
  EXPECT_EQ(cvc.size(), 1);
}

static void test_counter_vec_operations() {
  TEST("CounterVecComponent_operations");

  CounterVecComponent cvc(3);

  cvc.setCounter(0, 100);
  EXPECT_EQ(cvc.getCounter(0), 100);

  cvc.increase(0);
  EXPECT_EQ(cvc.getCounter(0), 101);

  cvc.decrease(0);
  EXPECT_EQ(cvc.getCounter(0), 100);

  cvc.setCounter(2, -50);
  EXPECT_EQ(cvc.getCounter(2), -50);
}

static void test_counter_vec_reset() {
  TEST("CounterVecComponent_reset");

  CounterVecComponent cvc(3);
  cvc.setCounter(0, 10);
  cvc.setCounter(1, 20);
  cvc.setCounter(2, 30);

  cvc.resetAll();
  for (size_t i = 0; i < cvc.size(); ++i) {
    EXPECT_EQ(cvc.getCounter(static_cast<int32_t>(i)), 0);
  }
}

static void test_counter_vec_out_of_range() {
  TEST("CounterVecComponent_out_of_range");

  CounterVecComponent cvc(3);
  EXPECT_THROWS(cvc.getCounter(5), std::out_of_range);
  EXPECT_THROWS(cvc.setCounter(5, 10), std::out_of_range);
  EXPECT_THROWS(cvc.increase(5), std::out_of_range);
  EXPECT_THROWS(cvc.decrease(5), std::out_of_range);
  EXPECT_THROWS(cvc.getCounter(-1), std::out_of_range);
}

// ============================================================
// StaticComponent 子类测试
// ============================================================
static void test_str_label_component() {
  TEST("StrLabelComponent");

  StrLabelComponent slc;
  EXPECT(slc.m_str_label.empty(), "default str_label should be empty");

  slc.m_str_label = "common";
  EXPECT_EQ(slc.m_str_label, "common");

  slc.m_str_label = "史诗品质物品";
  EXPECT_EQ(slc.m_str_label, "史诗品质物品");
}

static void test_val_label_component() {
  TEST("ValLabelComponent");

  ValLabelComponent vlc;
  // int32_t 默认值不确定（POD），不做断言

  vlc.m_val_label = 42;
  EXPECT_EQ(vlc.m_val_label, 42);

  vlc.m_val_label = -1;
  EXPECT_EQ(vlc.m_val_label, -1);

  vlc.m_val_label = 0;
  EXPECT_EQ(vlc.m_val_label, 0);
}

// ============================================================
// DynamicComponent 多态测试
// ============================================================
static void test_dynamic_component_polymorphism() {
  TEST("DynamicComponent_polymorphism");

  // CounterComponent 可以当作 DynamicComponent* 使用
  auto cc = std::make_unique<CounterComponent>();
  cc->setCounter(99);

  DynamicComponent *base = cc.get();
  EXPECT(base != nullptr, "base pointer should be valid");

  auto *derived = dynamic_cast<CounterComponent *>(base);
  EXPECT(derived != nullptr, "should dynamic_cast back to CounterComponent");
  if (derived) {
    EXPECT_EQ(derived->getCounter(), 99);
  }
}

static void test_all_dynamic_components_polymorphic() {
  TEST("all_DynamicComponents_are_polymorphic");

  // 验证所有三种 DynamicComponent 子类都可以通过基类指针访问
  {
    auto cc = std::make_unique<CounterComponent>();
    DynamicComponent *base = cc.get();
    EXPECT(dynamic_cast<CounterComponent *>(base) != nullptr,
           "CounterComponent should be polymorphic");
  }
  {
    auto cac = std::make_unique<CounterArrComponent>();
    DynamicComponent *base = cac.get();
    EXPECT(dynamic_cast<CounterArrComponent *>(base) != nullptr,
           "CounterArrComponent should be polymorphic");
  }
  {
    auto cvc = std::make_unique<CounterVecComponent>();
    DynamicComponent *base = cvc.get();
    EXPECT(dynamic_cast<CounterVecComponent *>(base) != nullptr,
           "CounterVecComponent should be polymorphic");
  }
}

// ============================================================
// StaticComponent 多态测试
// ============================================================
static void test_all_static_components_polymorphic() {
  TEST("all_StaticComponents_are_polymorphic");

  {
    auto slc = std::make_unique<StrLabelComponent>();
    StaticComponent *base = slc.get();
    EXPECT(dynamic_cast<StrLabelComponent *>(base) != nullptr,
           "StrLabelComponent should be polymorphic");
  }
  {
    auto vlc = std::make_unique<ValLabelComponent>();
    StaticComponent *base = vlc.get();
    EXPECT(dynamic_cast<ValLabelComponent *>(base) != nullptr,
           "ValLabelComponent should be polymorphic");
  }
}

// ============================================================
// ItemSemantic 枚举测试
// ============================================================
static void test_item_semantic_enum_values() {
  TEST("ItemSemantic_enum_values");

  // 验证枚举值是连续的且按预期定义
  EXPECT_EQ(static_cast<int>(ItemSemantic::UNC), 1);
  EXPECT_EQ(static_cast<int>(ItemSemantic::SNC), 2);
  EXPECT_EQ(static_cast<int>(ItemSemantic::SUC), 3);
  EXPECT_EQ(static_cast<int>(ItemSemantic::MUC), 4);
}

// ============================================================
// StaticComponentSemantic 枚举测试
// ============================================================
static void test_static_component_semantic_values() {
  TEST("StaticComponentSemantic_values");

  EXPECT_EQ(static_cast<int32_t>(StaticComponentSemantic::Quality), 10);
  EXPECT_EQ(static_cast<int32_t>(StaticComponentSemantic::DefaultCharges), 21);
  EXPECT_EQ(static_cast<int32_t>(StaticComponentSemantic::MaxContentCount), 30);
  EXPECT_EQ(static_cast<int32_t>(StaticComponentSemantic::Rule_0), 40);
  EXPECT_EQ(static_cast<int32_t>(StaticComponentSemantic::Rule_arr), 43);

  // 验证 Static 和 Dynamic 语义不冲突（不同 enum class 可以同值）
  EXPECT_EQ(static_cast<int32_t>(StaticComponentSemantic::ContainerTypes),
            static_cast<int32_t>(DynamicComponentSemantic::ContainerTypes));
}

// ============================================================
// DynamicComponentSemantic 枚举测试
// ============================================================
static void test_dynamic_component_semantic_values() {
  TEST("DynamicComponentSemantic_values");

  EXPECT_EQ(static_cast<int32_t>(DynamicComponentSemantic::ContentCount), 30);
  EXPECT_EQ(static_cast<int32_t>(DynamicComponentSemantic::ContainerTypes), 35);
  EXPECT_EQ(static_cast<int32_t>(DynamicComponentSemantic::RelaventTypes), 36);
  EXPECT_EQ(static_cast<int32_t>(DynamicComponentSemantic::Rule_0), 40);
  EXPECT_EQ(static_cast<int32_t>(DynamicComponentSemantic::Rule_arr), 43);
}

// ============================================================
// main
// ============================================================
int main() {
  std::cout << "=== ItemType & Component Tests ===" << std::endl;

  // ItemArcheType
  test_archetype_default();
  test_archetype_add_static_component();
  test_archetype_add_val_label_component();

  // ItemInstance 基础
  test_instance_default_construction();
  test_instance_uuid_unique();
  test_instance_set_get_type_id();
  test_instance_ref_count();
  test_instance_non_copyable();

  // ItemInstance getComponent
  test_get_component_not_found_int32();
  test_get_component_not_found_enum();
  test_get_component_wrong_type();

  // CounterComponent
  test_counter_component_default();
  test_counter_component_operations();
  test_counter_component_negative();

  // CounterArrComponent
  test_counter_arr_default();
  test_counter_arr_operations();
  test_counter_arr_reset();
  test_counter_arr_out_of_range();
  test_counter_arr_get_array();

  // CounterVecComponent
  test_counter_vec_default();
  test_counter_vec_sized_constructor();
  test_counter_vec_resize();
  test_counter_vec_operations();
  test_counter_vec_reset();
  test_counter_vec_out_of_range();

  // StaticComponent 子类
  test_str_label_component();
  test_val_label_component();

  // 多态
  test_dynamic_component_polymorphism();
  test_all_dynamic_components_polymorphic();
  test_all_static_components_polymorphic();

  // 枚举
  test_item_semantic_enum_values();
  test_static_component_semantic_values();
  test_dynamic_component_semantic_values();

  std::cout << "\n=== Results: " << g_passed << " passed, " << g_failed
            << " failed ===" << std::endl;

  return g_failed > 0 ? 1 : 0;
}
