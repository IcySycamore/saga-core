#include "ItemManager.h"
#include "Component/DynamicComponents.h"
#include "Component/Static/ValVecComponent.h"
#include "Component/StaticComponents.h"
#include <boost/json.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

bool ItemManager::loadArche(const std::string &path) {

  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "E[ItemManager] Failed to open file: " << path << std::endl;
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();
  boost::system::error_code ec;
  boost::json::value root = boost::json::parse(content, ec);

  if (ec) {
    std::cerr << "[ItemManager] JSON parse error at offset " << ec.failed()
              << ": " << ec.message() << std::endl;
    return false;
  }

  // 解析整体object
  if (!root.is_object()) {
    std::cerr << "[ItemManager] Root must be a JSON object." << std::endl;
    return false;
  }
  auto &root_obj = root.as_object();

  // 解析items数组
  if (!root_obj.contains("items") || !root_obj.at("items").is_array()) {
    std::cerr << "[ItemManager] Missing 'items' array." << std::endl;
    return false;
  }
  auto &items_array = root_obj.at("items").as_array();
  static int total_nob = 0;
  int nob = 0;
  for (const auto &item_value : items_array) {
    if (!item_value.is_object()) {
      std::cerr << "[ItemManager] Skipping non-object item." << std::endl;
      ++total_nob;
      continue;
    }
    const auto &item_obj = item_value.as_object();

    // 必填字段
    if (!item_obj.contains("type_id") || !item_obj.at("type_id").is_int64()) {
      std::cerr << "[ItemManager] Item missing 'type_id' or invalueid type."
                << std::endl;
      ++total_nob;
      continue;
    }
    int32_t type_id = static_cast<int32_t>(item_obj.at("type_id").as_int64());

    if (!item_obj.contains("name") || !item_obj.at("name").is_string()) {
      std::cerr << "[ItemManager] Item missing 'name' or invalueid type."
                << std::endl;
      ++total_nob;
      continue;
    }
    std::string item_name = (item_obj.at("name").as_string()).c_str();
    ItemArcheType arch = {
        type_id,  // m_type_id
        item_name // m_name
    };
    auto &arch_components = arch.m_component;
    // 可选组件
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
              comp->push_back(c.as_int64());
            std::cerr << "[ItemManager]  Skipping non-int element in \""
                      << semantic << "\"\n";
            ++nob;
            ++total_nob;
          }
          arch.m_component[semantic] = std::move(comp);
        } else {
          std::cerr << "[ItemManager] Unknown component type for key "
                    << semantic << std::endl;
          ++nob;
          ++total_nob;
        }
      }
      std::cerr << " [ItemManager] " << nob << " error occuerd in this parse\n";
      std::cerr << " [ItemManager] " << total_nob
                << " error occuerd in all parse\n";
    }

    m_item_archetypes[type_id] = std::move(arch);

    std::cout << "[ItemManager] Loaded TypeId: " << type_id
              << ", Name: " << item_name << std::endl;
  }

  return true;
}

ItemManager &ItemManager::getManager() {
  static ItemManager item_manager;
  return item_manager;
}

bool ItemManager::initArche(const std::string configPath) {
  std::unique_lock lock(m_arche_mutex);
  return loadArche(configPath);
}

bool ItemManager::reload(const std::string configPath) {
  std::unique_lock lock(m_arche_mutex);
  m_item_archetypes.clear();
  return loadArche(configPath);
}

const ItemArcheType *ItemManager::getArche(int32_t type_id) const {
  std::shared_lock lock(m_arche_mutex);
  auto it = m_item_archetypes.find(type_id);
  if (it == m_item_archetypes.end())
    return nullptr;
  return &it->second;
}
void ItemManager::clrHandler() {
  std::unique_lock lock(m_arche_mutex);
  m_item_handler.clear();
}
void ItemManager::rmvHandler(int32_t type_id) {
  std::unique_lock lock(m_arche_mutex);
  m_item_handler.erase(type_id);
}
void ItemManager::regHandler(int32_t type_id, ItemHandler handler) {
  std::unique_lock lock(m_arche_mutex);
  m_item_handler[type_id] = std::move(handler);
}

ItemHandler ItemManager::getHandler(int32_t type_id) {
  std::shared_lock lock(m_arche_mutex);
  auto it = m_item_handler.find(type_id);
  if (it == m_item_handler.end())
    return nullptr;
  return it->second;
}
ItemInstance *ItemManager::createItemInstance(int32_t type_id) {
  std::unique_lock lock(m_item_mutex);
#define REP_SEMANTIC_OPTIMIZATION
#ifdef REP_SEMANTIC_OPTIMIZATION
  if (m_rep_type_2_uuid.contains(type_id)) {
    return m_item_pool[m_rep_type_2_uuid[type_id]].get();
  }
#endif
  auto itArc = this->getArche(type_id);
  if (itArc == nullptr)
    return nullptr;

  auto inst = std::make_unique<ItemInstance>();
  inst->setTypeID(type_id);
  // inst->setRef(0);
  m_item_pool[inst->getUuid()] = std::move(inst);


  return inst.get();
}
bool ItemManager::destoryItemInstace(const uuid &id) {
  std::unique_lock lock(m_item_mutex);
  auto it = m_uuid_to_index.find(id);
  if (it == m_uuid_to_index.end())
    return false;

  size_t index = it->second;
  // 将最后一个元素移动到被删除的位置
  m_items[index] = std::move(m_items.back());
  // 更新被移动物品的索引
  m_uuid_to_index[m_items[index]->getUuid()] = index;
  // 删除末尾
  m_items.pop_back();
  m_uuid_to_index.erase(it);
  return true;
}
ItemInstance *ItemManager::getItemInstace(uuid item_uuid) const {
  std::shared_lock lock(m_item_mutex);
  auto it = m_uuid_to_index.find(item_uuid);
  if (it == m_uuid_to_index.end())
    return nullptr;
  return (m_items[it->second]).get();
}