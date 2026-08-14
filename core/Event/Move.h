#pragma once
/**
 * @brief 具体事件示例：移动事件（携带数据 + 自带逻辑）
 * @namespace lCYC::event
 * @note 演示 Event 虚接口用法：数据成员（x/y/z）+ type() + process() 逻辑
 * @note C++20
 */

#include "Event.h"

namespace lCYC::event {

/**
 * @brief 移动事件：携带目标坐标 + 数据合法性校验逻辑
 */
class MoveEvent : public Event {
public:
  float x; ///< 目标 x（数据成员）
  float y; ///< 目标 y
  float z; ///< 目标 z

  /**
   * @brief 构造
   * @param x_ 目标 x
   * @param y_ 目标 y
   * @param z_ 目标 z
   */
  explicit MoveEvent(float x_ = 0.0f, float y_ = 0.0f, float z_ = 0.0f)
      : x(x_), y(y_), z(z_) {}

  /** @brief 事件类型（订阅/分发用） */
  EventType getTypeId() const override { return 1001; }

  /**
   * @brief 事件自带逻辑：NaN 校验并回传结果码
   * @param result 结果回传（code：0 合法 / -1 非法）
   */
  void process(EventResult &result) override {
    if (x != x || y != y || z != z) { // NaN 检测（NaN != NaN）
      result.code = -1;
      return;
    }
    result.code = 0;
  }
};

} // namespace lCYC::event
