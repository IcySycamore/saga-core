#include "EntityManager.h"
#include "ItemSlot.h"
#include <array>
#include <cstdint>

/*  物品域语义：消耗/堆叠分类（游戏层概念，非引擎层）
 *  按需要时，总是大于1
 *  类型            最大消耗次数   最大堆叠数量     实例指针语义
 *  非消耗品           -1              1           物品本身
 *  非消耗品            -1            按需要          代表物
 *  单次消耗品           1            按需要          代表物
 *  多次消耗品         按需要            1           物品本身
 *  判断类型先check最大消耗次数即可
 */
enum class ItemSemantic { UNC = 1, SNC = 2, SUC = 3, MUC = 4 };

/**
 * @brief   库存类，用于实体背包和玩家仓库，对外提供的方法：use,add,remove,sort
 * @note 这个类不应该构造任何物品实例
 * @details   0        c_c   m_c      MAX_SLOT
  ┌─────────┬─────┬────────┐
  │/////////│     │ xxxxxx │
  └─────────┴─────┴────────┘
      已用    可用   待分配
        └───────┘
          总量
 */
class Inventory {
private:
  static constexpr int MAX_SLOT = 50; // 预分配50槽空间
  int32_t m_current_capacity;         // 当前已用槽数量
  int32_t m_max_capacity;             // 当前槽总量

  std::array<ItemSlot, MAX_SLOT> m_slots;
  EntityManager &m_manager;
  /**
   * @brief 检查索引的槽可装入typeid类型的物品数量
   * @return 0无效，>0有效
   * @details 先检查是否为该类型的槽且堆叠未满，否则检查是否还有可用槽
   */
  int32_t canFit(int32_t type_id, int32_t slot_index) const;
  // 移动，可用于合并/拆分/移动语义
  void moveItems(int32_t src, int32_t des, int32_t num);

public:
  Inventory(int32_t capacity, EntityManager &entity_manager);
  bool isFull() const { return m_current_capacity == m_max_capacity; }
  bool sort_the_slots();
  // 背包中物品间交互
  bool interact(int32_t src, int32_t des);
  // 背包中对物品交互
  bool use(int32_t);
  bool add(uuid);
  // 丢弃
  EntityInstance *remove(int32_t);
};