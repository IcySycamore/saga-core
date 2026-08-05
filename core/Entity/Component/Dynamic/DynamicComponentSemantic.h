#pragma once
#include <cstdint>
enum class DynamicComponentSemantic : int32_t {
  // 基础属性

  // 数值

  // 容器与交互
  ContentCount = 30,   // 容纳内容物数量
  ContainerTypes = 35, // 装入的物品类型ID列表
  RelaventTypes = 36,  // 交互物品类型ID列表

  // 应用的规则
  Rule_0 = 40,
  Rule_1 = 41,
  Rule_2 = 42,
  Rule_arr = 43

};