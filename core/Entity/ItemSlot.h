#include "EntityType.h"
#include <cassert>
#include <cstdint>

/**
 * @brief
 * ItemSlot是一个用于背包管理的轻量级槽对象，维护堆叠数以及一个实体实例指针(不是数组)；
 * @note
 * 使用ItemSlot的模块必须自己保证传入参数和函数调用的正确性。也即保证上下限以及内部指针的状态
 * always replace the item_ptr before adding from 0.
 * @details
 * 维持的该实例指针在如下逻辑类型下具有不同语义（物品域）：
 * 类型            最大消耗次数   最大堆叠数量     实例指针语义
 * 非消耗品(装备等)    -1              1           物品本身
 * 非消耗品(材料等)    -1            按需要          代表物
 * 单次消耗品           1            按需要          代表物
 * 多次消耗品         按需要            1           物品本身
 * 这样的实现意味着如果某种类型的物品间如果存在当前剩余消耗量和动态值的不同的可能，
 * 总将其最大堆叠量实现为1，实例指针指向的对象则总是物品本身
 * 其他类型（最大堆叠数大于1）可以使用代表物是因为他们的动态实例中有意义的成员变量除uuid外不存在任何可能上的不同。
 * 永不应该修改这些类型的item实例的动态值成员对象
 *
 * 调用示例：
 * 语义             调用顺序
 * add_zero2num     relpaceItem(target_item_ptr);
 *                  increaseStack(num);
 * add_k2k_plus_num increaseStack(num);
 * rmv_num2zero     clear();
 *                  or
 *                  decreaseStack(getStackNum());
 *                  relpaceItem(nullptr);
 * rmv_k_plus_num2k decreaseStack(num);
 * add_uni          if(getStackNum()==0)replaceItem(target_item-ptr)
 *                  increaseStack(num);
 * rmv_uni          decreaseStack(num);
 *                  if(getStackNum()==0)replaceItem(nullptr);
 *                         ↑
 * 注意不要使用isFullEmpty()，该函数用于判断完备的零状态，此时只是计数归零
 * 除非采取的是“移动”语义，否则总在rmv或rmv导致的replaceItem操作后检查代表物对象的槽引用计数
 */
class ItemSlot {
private:
  EntityInstance *m_item;
  int32_t m_stack_num;

public:
  ItemSlot() : m_item(nullptr), m_stack_num(0) {}
  // 当指针为空且计数为零的完备状态时返回true
  bool isFullEmpty() const {
    return !static_cast<bool>(m_item) && m_stack_num == 0;
  }
  // 当计数为零时返回true
  bool isEmpty() const { return m_stack_num == 0; }
  int32_t getStackNum() const { return m_stack_num; }
  void increaseStack(int32_t num = 1) {
    assert(num > 0 && "increaseStack: num must be positive");
    m_stack_num += num;
  }
  void decreaseStack(int32_t num = 1) {
    assert(m_stack_num >= num && "decreaseStack: stack underflow");
    assert(num > 0 && "decreaseStack: num must be positive");
    m_stack_num -= num;
  }
  void replaceItem(EntityInstance *newItem) {
    if (m_item) {
      m_item->onDetach();
    }
    m_item = newItem;
    if (newItem) {
      newItem->onAttach();
    }
  }
  void clear() {
    m_stack_num = 0;
    replaceItem(nullptr);
  }
  EntityInstance *getItem() const { return m_item; }
};