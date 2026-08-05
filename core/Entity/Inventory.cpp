#include "Inventory.h"
#include "EntityType.h"
#include <cmath>
#include <cstdint>

// TODO(游戏层适配): 以下方法依赖旧 API（getArcheType/m_max_stack_size/
// setCharges/getVal1/m_val_0/relavent_type/getItemSemantic），
// 需按新组件体系重写（语义键见
// StaticComponentSemantic/DynamicComponentSemantic）。

Inventory::Inventory(int32_t capacity, EntityManager &entity_manager)
    : m_current_capacity(capacity), m_max_capacity(capacity),
      m_manager(entity_manager) {}
int32_t Inventory::canFit(int32_t type_id, int32_t slot_index) const {
  auto slot = m_slots.at(slot_index);
  // 空槽返回typeId的 max stack size
  if (!slot.getItem())
    return m_manager.getArcheType(type_id)->m_max_stack_size;
  // 非空同类槽返回查询到的最大堆叠量与当前堆叠量之差
  if (slot.getItem()->getTypeID() == type_id)
    return m_manager.getArcheType(type_id)->m_max_stack_size -
           slot.getStackNum();
  // 非空非同类槽
  else
    return 0;
}
void Inventory::moveItems(int32_t src, int32_t des, int32_t num) {
  ItemSlot &src_slot = m_slots[src];
  ItemSlot &des_slot = m_slots[des];
  int32_t src_type = src_slot.getItem()->getTypeID();
  if (des_slot.getStackNum() == 0) {
    des_slot.replaceItem(src_slot.getItem());
  } // 不新建代表物而是共享代表物指针，replaceitem自动维护槽引用计数
  des_slot.increaseStack(num);
  src_slot.decreaseStack(num);
  if (src_slot.getStackNum() == 0) {
    src_slot.replaceItem(nullptr);
  }
}
bool Inventory::add(uuid item_uuid) {
  if (isFull()) {
    return false;
  }
  auto item = m_manager.getItemInstace(item_uuid);
  int32_t item_type = item->getTypeID();
  // 遍历同类型canfit（非满非空）槽，一定是不是代表物因为最大堆叠大于一且非空
  for (int c = 0; c < m_max_capacity; ++c) {
    if (!m_slots[c].isFullEmpty() &&
        m_slots[c].getItem()->getTypeID() == item_type) {
      m_slots[c].increaseStack();
      m_manager.destoryItemInstace(item_uuid);
      return true;
    }
  }
  // 未找到同类canfit槽，遍历找第一个空槽
  for (int c = 0; c < m_max_capacity; ++c) {
    if (m_slots[c].isFullEmpty()) {
      m_slots[c].replaceItem(item);
      m_slots[c].increaseStack();
    }
  }
}

bool Inventory::use(int32_t index) {
  auto item_slot = m_slots[index];
  auto item = item_slot.getItem();
  auto item_uuid = item->getUuid();
  switch (m_manager.getItemSemantic(item_uuid)) {
    // 不可堆叠非消耗品
    // 可堆叠非消耗品
  case ItemSemantic::UNC:
  case ItemSemantic::SNC:
    return true;
    // 单次消耗品(可堆叠)
  case ItemSemantic::SUC:
    item_slot.decreaseStack();
    if (item_slot.getStackNum() == 0) {
      item_slot.replaceItem(nullptr);
      if (item->getRefC() == 0)
        m_manager.destoryItemInstace(item_uuid);
    }
    // 多次消耗品(不可堆叠)
  case ItemSemantic::MUC:
    /* xxxxxxx
      xx    xx
     xx     xxxxxxxxxxxxxxxxxx
    xx    装备扣减耐久逻辑   xx
     xx     xxxxxxxxxxxxxxxxxx
      xx    xx
       xxxxxxx
    */
    item->setCharges(item->getCharges() - 1);
    // 不可堆叠，总是只被一个槽引用，检查这个槽对应实例的消耗程度
    if (item->getCharges() == 0) {
      item_slot.clear();
      m_manager.destoryItemInstace(item_uuid);
    }
  }
  return 0;
}
// 丢弃，从背包中丢弃一（整）格物品
EntityInstance *Inventory::remove(int32_t slot_index) {
  auto item_slot = m_slots[slot_index];
  auto item = item_slot.getItem();
  item_slot.clear();
  return item;
}

bool Inventory::sort_the_slots() {}
bool Inventory::interact(int32_t src, int32_t des) {
  auto &src_slot = m_slots[src];
  auto &des_slot = m_slots[des];
  EntityInstance *src_item = src_slot.getItem();
  EntityInstance *des_item = des_slot.getItem();
  int32_t src_type = src_item->getTypeID();
  int32_t des_type = des_item->getTypeID();
  if (src_type == des_type) {
    // min(src能提供的量, des能接收的量)尽可能多的传入
    moveItems(src, des,
              std::min(src_slot.getStackNum(), canFit(src_type, des)));
    return true;
  } else {
    auto &relavent_arr = m_manager.getArcheType(src_type)->relavent_type;
    if (relavent_arr.empty())
      return false;
    else if (std::find(relavent_arr.begin(), relavent_arr.end(), des_type) !=
             relavent_arr.end()) {
      auto des_arche = m_manager.getArcheType(des_type);
      int32_t avaliable_num = des_arche->m_val_0 - des_item->getVal1()
    }
  }
}