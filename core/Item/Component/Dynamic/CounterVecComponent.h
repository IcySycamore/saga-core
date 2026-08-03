#pragma once
#include "DynamicComponent.h"
#include <vector>
/**
 * @brief 存储int32_t的向量组件
 * @note 请提供异常检查
 *
 */
class CounterVecComponent : public DynamicComponent {
  std::vector<int32_t> m_counter_vec;

public:
  CounterVecComponent() = default;
  explicit CounterVecComponent(size_t size) : m_counter_vec(size, 0) {}

  void resize(size_t newSize) { m_counter_vec.resize(newSize, 0); }
  size_t size() const { return m_counter_vec.size(); }

  void resetAll() {
    for (auto &counter : m_counter_vec) {
      counter = 0;
    }
  }

  void increase(int32_t index) { m_counter_vec.at(index) += 1; }
  void decrease(int32_t index) { m_counter_vec.at(index) -= 1; }
  void setCounter(int32_t index, int32_t num) { m_counter_vec.at(index) = num; }
  int32_t getCounter(int32_t index) const { return m_counter_vec.at(index); }
};