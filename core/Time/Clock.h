#pragma once
/**
 * @brief 引擎时钟
 * @namespace lCYC::clock
 * @note update(steady_now) / addDuration(dur) 由外部注入
 * @note 只维护逻辑时间轴（tick/reg/alpha/scale）；真实时长由主循环维护
 * @note C++20
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>

namespace lCYC::clock {

class Clock {
public:
  /// 尖峰保护 上限/秒
  static constexpr double maxFrameTime = 0.1;
  /// 单帧最多 tick/个
  static constexpr int maxTicksPerFrame = 16;

  explicit Clock(double tick_rate = 60.0f, double scale_ = 1.0f)
      : m_tick_rate(tick_rate), m_tick_delta(1.0 / tick_rate),
        m_sys_start(std::chrono::system_clock::now()),
        m_mck_last(std::chrono::steady_clock::now()), m_scale(scale_),
        m_ticks(0), m_logic_reg(0.0) {}
  static Clock &getMasterClock() {
    static Clock s_clk;
    return s_clk;
  }
  int64_t getTick() const { return m_ticks; }
  float getTickRate() const { return m_tick_rate; }
  double getTickDelta() const { return m_tick_delta; }

  /// 连续 逻辑时间
  double getLogicAcc() const { return m_ticks * m_tick_delta + m_logic_reg; }
  /// tick对齐 离散 逻辑时间
  double getLogicTime() const { return m_ticks * m_tick_delta; }

  /// 渲染插值因子
  double getAlpha() const { return m_logic_reg / m_tick_delta; }
  double getScale() const { return m_scale; }
  void setScale(double scale_) { m_scale = scale_; }

  std::time_t getSysTime() const {
    return std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
  }
  std::time_t getSysStart() const {
    return std::chrono::system_clock::to_time_t(m_sys_start);
  }
  void update(std::chrono::time_point<std::chrono::steady_clock> steady_now) {
    const double dur =
        std::chrono::duration<double>(steady_now - m_mck_last).count();
    addDuration(dur);
    m_mck_last = steady_now;
  }

  void addDuration(double dur) {
    // 尖峰保护
    dur = std::min(dur, maxFrameTime);
    m_logic_reg += dur * m_scale;
    // 固定步长补帧防死亡螺旋
    int ticks = maxTicksPerFrame;
    while (m_logic_reg >= m_tick_delta && ticks--) {
      m_logic_reg -= m_tick_delta;
      ++m_ticks;
      clockUpdated();
    }
    // 仍超限，丢弃
    if (m_logic_reg >= m_tick_delta) {
      m_logic_reg = 0.0;
    }
  }
  void clockUpdated() {}

private:
  int64_t m_ticks;     // 逻辑 tick 计数
  double m_scale;      // 时间缩放
  double m_logic_reg;  // 逻辑时长寄存
  double m_tick_rate;  // 固定逻辑帧率
  double m_tick_delta; // 固定步长
  std::chrono::time_point<std::chrono::system_clock> m_sys_start; // 时间戳
  std::chrono::time_point<std::chrono::steady_clock> m_mck_last; // 上一帧时间点
};

} // namespace lCYC::clock
