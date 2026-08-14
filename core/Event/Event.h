#pragma once
/**
 * @brief 事件系统：虚事件接口 + 事件管理器
 * @namespace lCYC::event
 * @note 设计（用户定）:
 *   - 虚事件接口：数据（成员变量）+ 逻辑（虚 process）
 *   - EventManager：std::queue 入队 / 注册监听（1:N 多播，ADR-0007 Signal）/
 *     发送事件 / 信号与结果回传
 * @note Signal = 1:N 多播，
 *       handler = 1:1 延迟回调走事件队列
 * @note C++20
 */

#include "../helper/UuidGen.h"
#include <boost/uuid.hpp>
#include <cstdint>

namespace lCYC::event {

using EventType = int32_t;
struct EventResult {
  bool handled = false;
  int32_t code = 0;
};

/**
 * @brief 虚事件接口
 */
class Event {
protected:
  boost::uuids::uuid m_uuid;

public:
  Event() : m_uuid(uuidGen()) {}
  virtual ~Event() = default;
  Event(const Event &) = delete;
  Event &operator=(const Event &) = delete;

  boost::uuids::uuid getUuid() const { return m_uuid; }
  virtual EventType getTypeId() const = 0;
  virtual void process(EventResult &result) { (void)result; }
  virtual void onFinished() = 0;
};

} // namespace lCYC::event