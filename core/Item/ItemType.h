#pragma once
#include "../helper/UuidGen.h"
#include "Component/Dynamic/DynamicComponent.h"
#include "Component/Dynamic/DynamicComponentSemantic.h"
#include "Component/Static/StaticComponent.h"
#include "Component/Static/StaticComponentSemantic.h"
#include <boost/uuid.hpp>
#include <unordered_map>


enum class ItemSemantic { UNC = 1, SNC = 2, SUC = 3, MUC = 4 };

/*  按需要时，总是大于1
 *  类型            最大消耗次数   最大堆叠数量     实例指针语义
 *  非消耗品           -1              1           物品本身
 *  非消耗品            -1            按需要          代表物
 *  单次消耗品           1            按需要          代表物
 *  多次消耗品         按需要            1           物品本身
 *  判断类型先check最大消耗次数即可
 */

/**
 *  @brief ItemArcheType
 *  是一个结构体，用于描述物品的基本属性，包括物品类型ID、名字和其他可选用组件
 *  如最大消耗值标签、最大堆叠值标签、
 *  参考价格值标签、别名字符标签以及其他标签。
 */
struct ItemArcheType {
  int32_t m_type_id;  // 物品类型id，如#1087
  std::string m_name; // 物品类型名
  // int32_t m_max_charges;    // 最大消耗次数
  // int32_t m_max_stack_size; // 最大堆叠数量
  // int32_t m_price;          // 参考价格

  std::unordered_map<int32_t, std ::unique_ptr<StaticComponent>> m_component;
};

class ItemInstance {
private:
  boost::uuids::uuid m_id; // uuid
  int32_t m_type_id;       // 物品类型id
  int32_t m_ref_count;     // 槽引用计数
  // int32_t m_charges;       // 剩余消耗次数
  std::unordered_map<int32_t, std ::unique_ptr<DynamicComponent>> m_component;

public:
  ItemInstance() : m_id(uuidGen()), m_type_id(0), m_ref_count(0) {}
  ItemInstance(const ItemInstance &) = delete;
  boost::uuids::uuid getUuid() const { return m_id; }
  int32_t getTypeID() const { return m_type_id; }
  int32_t getRefC() const { return m_ref_count; }
  // 提供基于 int32_t 的每个物品类不同的组件索引
  template <typename T> T *getComponent(int32_t index) {
    auto it = m_component.find(index);
    return dynamic_cast<T *>(it != m_component.end() ? it->second.get()
                                                     : nullptr);
  }
  // 推荐使用自文档化的所有物品类统一的枚举索引
  template <typename T> T *getComponent(DynamicComponentSemantic semantic) {
    auto it = m_component.find(static_cast<int32_t>(semantic));
    return dynamic_cast<T *>(it != m_component.end() ? it->second.get()
                                                     : nullptr);
  }
  void setTypeID(int32_t type_id) { m_type_id = type_id; }

  void onAttach() { ++m_ref_count; }
  void onDetach() { --m_ref_count; }
};