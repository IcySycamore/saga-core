#include "EntityManager.h"
#include "Component/DynamicComponents.h"
#include "Component/Static/ValVecComponent.h"
#include "Component/StaticComponents.h"
#include <boost/json.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

bool EntityManager::loadArche(const std::string &path) {

  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "E[EntityManager] Failed to open file: " << path << std::endl;
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();
  boost::system::error_code ec;
  boost::json::value root = boost::json::parse(content, ec);

  if (ec) {
    std::cerr << "[EntityManager] JSON parse error at offset " << ec.failed()
              << ": " << ec.message() << std::endl;
    return false;
  }

  // 解析整体object
  if (!root.is_object()) {
    std::cerr << "[EntityManager] Root must be a JSON object." << std::endl;
    return false;
  }
  auto &root_obj = root.as_object();

  // 解析 archetypes 数组
  if (!root_obj.contains("archetypes") ||
      !root_obj.at("archetypes").is_array()) {
    std::cerr << "[EntityManager] Missing 'archetypes' array." << std::endl;
    return false;
  }
  auto &items_array = root_obj.at("archetypes").as_array();
  static int total_nob = 0;
  int nob = 0;
  for (const auto &item_value : items_array) {
    if (!item_value.is_object()) {
      std::cerr << "[EntityManager] Skipping non-object archetype."
                << std::endl;
      ++total_nob;
      continue;
    }
    const auto &item_obj = item_value.as_object();

    // 必填字段
    if (!item_obj.contains("type_id") || !item_obj.at("type_id").is_int64()) {
      std::cerr
          << "[EntityManager] Archetype missing 'type_id' or invalid type."
          << std::endl;
      ++total_nob;
      continue;
    }
    int32_t type_id = static_cast<int32_t>(item_obj.at("type_id").as_int64());

    if (!item_obj.contains("name") || !item_obj.at("name").is_string()) {
      std::cerr << "[EntityManager] Archetype missing 'name' or invalid type."
                << std::endl;
      ++total_nob;
      continue;
    }
    std::string item_name = (item_obj.at("name").as_string()).c_str();
    EntityArcheType arch = {
        type_id,  // m_type_id
        item_name // m_name
    };
    auto &arch_components = arch.m_component;
    // 组件
    if (item_obj.contains("components") &&
        item_obj.at("components").is_object()) {
      const auto &component_arr = item_obj.at("components").as_object();
      for (const auto &[key, value] : component_arr) {

        int32_t semantic = std::stoi(std::string(key)); // "121" → 121

        if (value.is_int64()) {
          // → valLabelComponent
          auto comp = std::make_unique<ValLabelComponent>();
          comp->m_val_label = static_cast<int32_t>(value.as_int64());
          arch.m_component[semantic] = std::move(comp);
        } else if (value.is_string()) {
          // → StrLabelComponent
          auto comp = std::make_unique<StrLabelComponent>();
          comp->m_str_label = value.as_string();
          arch.m_component[semantic] = std::move(comp);
        } else if (value.is_array()) {
          // → ValVecComponent
          auto comp = std::make_unique<ValVecComponent>();
          for (auto &c : value.as_array()) {
            if (c.is_int64())
              comp->push_back(static_cast<int32_t>(c.as_int64()));
          }
          arch.m_component[semantic] = std::move(comp);
        } else {
          std::cout << "[EntityManager] Unknown component type for key "
                    << semantic << std::endl;
          ++nob;
          ++total_nob;
        }
      }
      std::cout << " [EntityManager] " << nob << " error occuerd in #"
                << type_id << " 's components parsing\n";
    }
    if (item_obj.contains("defaults") && item_obj.at("defaults").is_object()) {
      const auto &default_arr = item_obj.at("defaults").as_object();
      for (const auto &[key, value] : default_arr) {
        int32_t semantic = std::stoi(std::string(key)); // "121" → 121

        if (value.is_int64()) {
          // → valLabelComponent
          auto comp = std::make_unique<ValLabelComponent>();
          comp->m_val_label = static_cast<int32_t>(value.as_int64());
          arch.m_defaults[semantic] = std::move(comp);
        } else if (value.is_array()) {
          // → ValVecComponent
          auto comp = std::make_unique<ValVecComponent>();
          for (auto &c : value.as_array()) {
            if (c.is_int64())
              comp->push_back(static_cast<int32_t>(c.as_int64()));
          }
          arch.m_defaults[semantic] = std::move(comp);
        } else {
          std::cout << "[EntityManager] Unknown default type for key "
                    << semantic << std::endl;
          ++nob;
          ++total_nob;
        }
      }
      std::cout << " [EntityManager] " << nob << " error occuerd in #"
                << type_id << " 's defaults parsing\n";
    }
    m_archetypes[type_id] = std::move(arch);

    std::cout << "[EntityManager] Loaded TypeId: " << type_id
              << ", Name: " << item_name << std::endl;
  }
  std::cerr << " [EntityManager] " << total_nob
            << " error occuerd in parsing\n";
  return true;
}
EntityManager &EntityManager::getManager() {
  static EntityManager entity_manager;
  return entity_manager;
}

bool EntityManager::initArche(const std::string configPath) {
  std::unique_lock lock(m_arche_mutex);
  return loadArche(configPath);
}

bool EntityManager::reload(const std::string configPath) {
  std::unique_lock lock(m_arche_mutex);
  m_archetypes.clear();
  return loadArche(configPath);
}

const EntityArcheType *EntityManager::getArche(int32_t type_id) const {
  std::shared_lock lock(m_arche_mutex);
  auto it = m_archetypes.find(type_id);
  if (it == m_archetypes.end())
    return nullptr;
  return &it->second;
}
void EntityManager::clrHandler() {
  std::unique_lock lock(m_arche_mutex);
  m_handler.clear();
}
void EntityManager::rmvHandler(int32_t type_id) {
  std::unique_lock lock(m_arche_mutex);
  m_handler.erase(type_id);
}
void EntityManager::regHandler(int32_t type_id, EntityHandler handler) {
  std::unique_lock lock(m_arche_mutex);
  m_handler[type_id] = std::move(handler);
}

EntityHandler EntityManager::getHandler(int32_t type_id) {
  std::shared_lock lock(m_arche_mutex);
  auto it = m_handler.find(type_id);
  if (it == m_handler.end())
    return nullptr;
  return it->second;
}
EntityInstance *EntityManager::createInstance(int32_t type_id) {
  std::unique_lock lock(m_pool_mutex);
  auto itArc = this->getArche(type_id);
  if (itArc == nullptr)
    return nullptr;
// 代表物语义优化（ADR-0003）：由构建时宏 REP_SEMANTIC_OPTIMIZATION 控制
//（CMake option TRPG_ENABLE_REP_SEMANTIC_OPTIMIZATION，默认 ON）
#ifdef REP_SEMANTIC_OPTIMIZATION
  if (itArc->m_defaults.empty()) {
    if (m_rep_type_2_uuid.contains(type_id)) {
      return m_pool[m_rep_type_2_uuid[type_id]].get();
    } else {
      auto inst = std::make_unique<EntityInstance>();
      EntityInstance *raw = inst.get();
      inst->setTypeID(type_id);
      m_rep_type_2_uuid[type_id] = raw->getUuid();
      m_pool[raw->getUuid()] = std::move(inst);
      return raw;
    }
  }
#endif

  auto inst = std::make_unique<EntityInstance>();
  inst->setTypeID(type_id);
  for (const auto &[semantic, static_comp] : itArc->m_defaults) {
    if (auto *val = dynamic_cast<ValLabelComponent *>(static_comp.get())) {
      // ValLabel → Counter
      auto counter = std::make_unique<CounterComponent>();
      counter->setCounter(val->m_val_label);
      inst->addComponent(semantic, std::move(counter));
    } else if (auto *vec = dynamic_cast<ValVecComponent *>(static_comp.get())) {
      // ValVec → CounterVec
      auto counter_vec = std::make_unique<CounterVecComponent>(vec->size());
      for (size_t i = 0; i < vec->size(); ++i) {
        counter_vec->setCounter(static_cast<int32_t>(i),
                                vec->getVal(static_cast<int32_t>(i)));
      }
      inst->addComponent(semantic, std::move(counter_vec));
    } else {
      std::cerr << "[EntityManager] Unsupported default component type for "
                   "semantic "
                << semantic << std::endl;
    }
  }
  EntityInstance *raw = inst.get();
  m_pool[raw->getUuid()] = std::move(inst);
  return raw;
}
bool EntityManager::destroyInstance(const uuid &id) {
  std::unique_lock lock(m_pool_mutex);
  auto it = m_pool.find(id);
  if (it == m_pool.end())
    return false;
  m_pool.erase(it);
  return true;
}
EntityInstance *EntityManager::getInstance(uuid entity_uuid) const {
  std::shared_lock lock(m_pool_mutex);
  auto it = m_pool.find(entity_uuid);
  if (it == m_pool.end())
    return nullptr;
  return it->second.get();
}

bool EntityManager::saveInstances(const std::string &path) {
  std::shared_lock lock(m_pool_mutex);
  boost::json::object root;
  boost::json::array instances;
  boost::json::object representatives;

  // 代表物注册表：type_id → uuid
  for (const auto &[type_id, rep_uuid] : m_rep_type_2_uuid) {
    representatives[std::to_string(type_id)] =
        boost::json::string(boost::uuids::to_string(rep_uuid));
  }

  // 全量实例池
  for (const auto &[id, inst] : m_pool) {
    boost::json::object obj;
    obj["uuid"] = boost::json::string(boost::uuids::to_string(id));
    obj["type_id"] = inst->getTypeID();

    // 代表物判定：该 type 的代表物 uuid == 当前实例（ADR-0003）
    const bool is_rep = m_rep_type_2_uuid.contains(inst->getTypeID()) &&
                        m_rep_type_2_uuid.at(inst->getTypeID()) == id;
    if (!is_rep) {
      // 动态组件序列化（ADR-0006：dynamic_cast 外部分支）
      boost::json::object comps;
      inst->forEachComponent([&](int32_t semantic,
                                 const DynamicComponent *comp) {
        if (auto *counter = dynamic_cast<const CounterComponent *>(comp)) {
          comps[std::to_string(semantic)] = counter->getCounter();
        } else if (auto *vec =
                       dynamic_cast<const CounterVecComponent *>(comp)) {
          boost::json::array arr;
          for (size_t i = 0; i < vec->size(); ++i) {
            arr.push_back(vec->getCounter(static_cast<int32_t>(i)));
          }
          comps[std::to_string(semantic)] = std::move(arr);
        } else {
          std::cerr << "[EntityManager] Unsupported dynamic component on save, "
                       "semantic="
                    << semantic << std::endl;
        }
      });
      if (!comps.empty()) {
        obj["components"] = std::move(comps);
      }
    }
    instances.push_back(std::move(obj));
  }

  root["instances"] = std::move(instances);
  root["representatives"] = std::move(representatives);

  std::ofstream file(path);
  if (!file.is_open()) {
    std::cerr << "[EntityManager] Failed to open file for save: " << path
              << std::endl;
    return false;
  }
  file << boost::json::serialize(root);
  return true;
}

bool EntityManager::loadInstances(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "[EntityManager] Failed to open file: " << path << std::endl;
    return false;
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  boost::system::error_code ec;
  boost::json::value root = boost::json::parse(buffer.str(), ec);
  if (ec) {
    std::cerr << "[EntityManager] JSON parse error: " << ec.message()
              << std::endl;
    return false;
  }
  if (!root.is_object()) {
    std::cerr << "[EntityManager] Root must be a JSON object." << std::endl;
    return false;
  }
  const auto &root_obj = root.as_object();

  std::unique_lock lock(m_pool_mutex);

  // 先读代表物注册表（供实例条目判定）
  std::unordered_map<int32_t, uuid> rep_table;
  if (root_obj.contains("representatives") &&
      root_obj.at("representatives").is_object()) {
    boost::uuids::string_generator gen;
    for (const auto &[key, value] :
         root_obj.at("representatives").as_object()) {
      if (!value.is_string()) {
        continue; // 损坏条目跳过（尽力而为）
      }
      int32_t type_id = std::stoi(std::string(key));
      rep_table[type_id] = gen(value.as_string().c_str());
    }
  }

  if (!root_obj.contains("instances") ||
      !root_obj.at("instances").is_array()) {
    std::cerr << "[EntityManager] Missing 'instances' array." << std::endl;
    return false;
  }

  boost::uuids::string_generator uuid_gen;
  for (const auto &item : root_obj.at("instances").as_array()) {
    if (!item.is_object()) {
      continue; // 损坏条目跳过
    }
    const auto &obj = item.as_object();
    if (!obj.contains("uuid") || !obj.at("uuid").is_string() ||
        !obj.contains("type_id") || !obj.at("type_id").is_int64()) {
      continue;
    }
    const uuid id = uuid_gen(obj.at("uuid").as_string().c_str());
    if (id.is_nil()) {
      continue; // 无效 uuid
    }
    const int32_t type_id = static_cast<int32_t>(obj.at("type_id").as_int64());

    auto inst = std::make_unique<EntityInstance>();
    inst->setUuid(id);
    inst->setTypeID(type_id);

    // 代表物判定：该 type 的代表物 uuid == 此条 → 注册，不填组件
    const bool is_rep = rep_table.contains(type_id) && rep_table.at(type_id) == id;
    if (is_rep) {
      m_rep_type_2_uuid[type_id] = id;
    } else if (obj.contains("components") &&
               obj.at("components").is_object()) {
      for (const auto &[key, value] : obj.at("components").as_object()) {
        const int32_t semantic = std::stoi(std::string(key));
        if (value.is_int64()) {
          auto counter = std::make_unique<CounterComponent>();
          counter->setCounter(static_cast<int32_t>(value.as_int64()));
          inst->addComponent(semantic, std::move(counter));
        } else if (value.is_array()) {
          auto vec = std::make_unique<CounterVecComponent>(
              value.as_array().size());
          size_t i = 0;
          for (const auto &elem : value.as_array()) {
            if (elem.is_int64()) {
              vec->setCounter(static_cast<int32_t>(i),
                              static_cast<int32_t>(elem.as_int64()));
            }
            ++i;
          }
          inst->addComponent(semantic, std::move(vec));
        } else {
          std::cerr
              << "[EntityManager] Unknown component type on load, semantic="
              << semantic << ", skipped" << std::endl;
        }
      }
    }
    m_pool[id] = std::move(inst);
  }
  return true;
}