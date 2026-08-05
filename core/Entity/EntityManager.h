#include "EntityType.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

// ===============================设计待完善==================================
using EntityHandler =
    std::function<void(EntityInstance &, const EntityArcheType &)>;
//===========================================================================
using uuid = boost::uuids::uuid;

class EntityManager {
private:
  std::unordered_map<int32_t, EntityArcheType> m_archetypes; // Arche表
  std::unordered_map<int32_t, EntityHandler> m_handler;      // handler表
  std::unordered_map<uuid, std::unique_ptr<EntityInstance>>
      m_pool;                                          // 全量实体池
  std::unordered_map<int32_t, uuid> m_rep_type_2_uuid; // 代表物快速索引
  mutable std::shared_mutex m_arche_mutex;             // arche锁
  mutable std::shared_mutex m_pool_mutex;              // 池锁

  EntityManager() = default;
  ~EntityManager() = default;
  EntityManager &operator=(const EntityManager &) = delete;
  EntityManager(const EntityManager &) = delete;

  bool loadArche(const std::string &);

public:
  // 获取EntityManager实例
  static EntityManager &getManager();

  // 从文件读实例
  bool loadInstance(const std::string &path);
  // 写文件到实例
  bool saveInstance(const std::string &path);

  // 初始化静态信息表
  bool initArche(const std::string configPath);
  // 热重载静态表
  bool reload(const std::string configPath);
  const EntityArcheType *getArche(int32_t) const;

  // 清空逻辑
  void clrHandler();
  // 移除逻辑
  void rmvHandler(int32_t);
  // 注册逻辑
  void regHandler(int32_t, EntityHandler);
  // 获取逻辑
  EntityHandler getHandler(int32_t);

  /**
   *  @brief 创建实体实例
   *  @param int32_t 类型id
   *  @return EntityInstance* 创建好的实体实例的指针
   */
  EntityInstance *createInstance(int32_t);
  EntityInstance *getInstance(uuid) const;
  bool destroyInstance(const uuid &);
};