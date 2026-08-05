#pragma once
#include "../helper/UuidGen.h"
#include "Component/Dynamic/DynamicComponent.h"
#include "Component/Dynamic/DynamicComponentSemantic.h"
#include "Component/Static/StaticComponent.h"
#include "Component/Static/StaticComponentSemantic.h"
#include <boost/uuid.hpp>
#include <memory>
#include <unordered_map>
#include <utility>

/**
 *  @brief EntityArcheType
 *  是一个结构体，用于描述一类实体的基本属性，包括类型ID、名字和其他可选用组件。
 *  适用于物品、生物等所有实体：组件由语义枚举（StaticComponentSemantic）索引。
 */
struct EntityArcheType {
  int32_t m_type_id;  // 实体类型id，如#1087
  std::string m_name; // 实体类型名

  std::unordered_map<int32_t, std ::unique_ptr<StaticComponent>> m_component;
  std::unordered_map<int32_t, std ::unique_ptr<StaticComponent>> m_defaults;
};

class EntityInstance {
private:
  boost::uuids::uuid m_id; // uuid
  int32_t m_type_id;       // 实体类型id
  int32_t m_ref_count;     // 槽引用计数
  std::unordered_map<int32_t, std ::unique_ptr<DynamicComponent>> m_component;

public:
  EntityInstance() : m_id(uuidGen()), m_type_id(0), m_ref_count(0) {}
  EntityInstance(const EntityInstance &) = delete;
  boost::uuids::uuid getUuid() const { return m_id; }
  int32_t getTypeID() const { return m_type_id; }
  int32_t getRefC() const { return m_ref_count; }
  // 提供基于 int32_t 的每个实体类不同的组件索引
  template <typename T> T *getComponent(int32_t index) {
    auto it = m_component.find(index);
    return dynamic_cast<T *>(it != m_component.end() ? it->second.get()
                                                     : nullptr);
  }
  // 推荐使用自文档化的所有实体类统一的枚举索引
  template <typename T> T *getComponent(DynamicComponentSemantic semantic) {
    auto it = m_component.find(static_cast<int32_t>(semantic));
    return dynamic_cast<T *>(it != m_component.end() ? it->second.get()
                                                     : nullptr);
  }
  // 添加动态组件（int32_t 键）
  void addComponent(int32_t index, std::unique_ptr<DynamicComponent> comp) {
    m_component[index] = std::move(comp);
  }
  // 添加动态组件（语义枚举键）
  void addComponent(DynamicComponentSemantic semantic,
                    std::unique_ptr<DynamicComponent> comp) {
    m_component[static_cast<int32_t>(semantic)] = std::move(comp);
  }
  void setTypeID(int32_t type_id) { m_type_id = type_id; }

  void onAttach() { ++m_ref_count; }
  void onDetach() { --m_ref_count; }
};