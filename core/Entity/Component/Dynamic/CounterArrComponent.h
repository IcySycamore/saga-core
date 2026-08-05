// CounterArrComponent.h
#pragma once
#include "DynamicComponent.h"
#include <array>
/**
 * @brief 存储int32_t的大小为5的数组组件
 * @note 请提供异常检查
 *
 */
class CounterArrComponent : public DynamicComponent {
public:
  static constexpr size_t kSize = 5;
  using ArrayType = std::array<int32_t, kSize>;

private:
  ArrayType m_counter_arr = {0, 0,0,0,0};

public:
  CounterArrComponent() = default;
  explicit CounterArrComponent(const ArrayType &values) : m_counter_arr(values) {}

  void resetAll() { m_counter_arr.fill(0); }

  int32_t get(size_t index) const { return m_counter_arr.at(index); }
  void set(size_t index, int32_t value) { m_counter_arr.at(index) = value; }
  void increase(size_t index) { m_counter_arr.at(index)++; }
  void decrease(size_t index) { m_counter_arr.at(index)--; }

  size_t size() const { return kSize; }
  // 序列化
  const ArrayType &getArray() const { return m_counter_arr; }
};