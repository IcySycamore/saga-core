#pragma once
#include "StaticComponent.h"
#include <vector>
/**
 * @brief 存储int32_t的向量组件
 * @note 请提供异常检查
 *
 */
class ValVecComponent : public StaticComponent {

  std::vector<int32_t> m_val_vec;

public:
  ValVecComponent() = default;

  void push_back(int32_t val) { m_val_vec.push_back(val); }
  size_t size() const { return m_val_vec.size(); }
  int32_t getVal(int32_t index) const { return m_val_vec.at(index); }
};