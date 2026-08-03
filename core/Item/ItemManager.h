#include "../Entity/Entity.h"
#include "ItemType.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

// ===============================设计待完善==================================
using ItemHandler =
    std::function<void(ItemInstance &, const ItemArcheType &, Entity &)>;
//===========================================================================
using uuid = boost::uuids::uuid;

class ItemManager {
private:
  std::unordered_map<int32_t, ItemArcheType> m_item_archetypes; // 物品Arche表
  std::unordered_map<int32_t, ItemHandler> m_item_handler;      // 物品handler表
  std::unordered_map<uuid, std::unique_ptr<ItemInstance>>
      m_item_pool;                                     // 全量物品池
  std::unordered_map<int32_t, uuid> m_rep_type_2_uuid; // 代表物快速索引
  mutable std::shared_mutex m_arche_mutex;             // arche锁
  mutable std::shared_mutex m_item_mutex;              // item锁

  ItemManager() = default;
  ~ItemManager() = default;
  ItemManager &operator=(const ItemManager &) = delete;
  ItemManager(const ItemManager &) = delete;

  bool loadArche(const std::string &);

public:
  // 获取ItemManager实例
  static ItemManager &getManager();

  // 从文件读实例
  bool loadItemInstance(const std::string &path);
  // 写文件到实例
  bool saveItemInstance(const std::string &path);

  // 初始化静态信息表
  bool initArche(const std::string configPath);
  // 热重载静态表
  bool reload(const std::string configPath);
  const ItemArcheType *getArche(int32_t) const;

  // 清空逻辑
  void clrHandler();
  // 移除逻辑
  void rmvHandler(int32_t);
  // 注册逻辑
  void regHandler(int32_t, ItemHandler);
  // 获取逻辑
  ItemHandler getHandler(int32_t);

  /**
   *  @brief 创建物品实例
   *  @param int32_t 类型id
   *  @return unique_ptr<ItemInstance> 创建好的物品实例的指针
   */
  ItemInstance *createItemInstance(int32_t);
  ItemInstance *getItemInstace(uuid) const;
  bool destoryItemInstace(const uuid &);
};