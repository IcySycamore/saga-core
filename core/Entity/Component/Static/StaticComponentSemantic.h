#include <cstdint>
enum class StaticComponentSemantic : int32_t {
  // 基础属性
  Quality = 110,     // 品质标签 common,rare,epic,myth,legend
  AliasName_0 = 111, // 别名0
  AliasName_1 = 112, // 别名1

  // 数值
  DefaultCharges = 121, // 默认消耗次数计数器初始值
  HealAmount = 122,
  DefendAmount = 123,

  // 容器与交互
  MaxContentCount = 130,     // 最大容纳内容物数量
  MinContentCount = 131,     // 最小容纳内容物数量
  DefaultContentCount = 132, // 默认容纳内容物数量
  ContainerTypes = 135,      // 可装入的物品类型ID列表
  RelaventTypes = 136,       // 可交互物品类型ID列表

  // 规则
  Rule_0 = 140,
  Rule_1 = 141,
  Rule_2 = 142,
  Rule_arr = 143

};