#pragma once
#include "DynamicComponent.h"
#include <cstdint>


class CounterComponent : public DynamicComponent {
  int32_t m_counter;

public:
  CounterComponent() : m_counter(0) {}
  void clear() { m_counter = 0; }
  void increase() { ++m_counter; }
  void decrease() { --m_counter; }
  void setCounter(int32_t num) { m_counter = num; }
  int32_t getCounter() const { return m_counter; }
};