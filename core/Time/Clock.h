#pragma once
/**
 * @brief 引擎时钟（固定步长 ticker + 双时间轴视图 + 时间缩放）
 * @namespace clockns
 * @note 原则:
 *   - update(steady_now) / addDuration(dur) 由外部注入
 *   - getLogicTime()（tick 对齐，确定性）/ getRealAcc()（真实）
 *   - 尖峰保护 maxFrameTime：巨大 dt 被钳制，防物理爆炸
 *   - 防死亡螺旋：单帧最多补 maxTicksPerFrame 个 tick，超出丢弃时间
 * @note C++20
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>

namespace clockns {

class Clock {
public:
  // ============================ 常量 ============================

  /// 尖峰保护上限（秒）：单帧真实 dt 超过它会被钳制，防调试/卡顿导致爆炸
  static constexpr double maxFrameTime = 0.1;

  /// 单帧最多补的 tick 数（防死亡螺旋：FixedUpdate 超时不会死循环）
  /// 尖峰保护(0.1s)内正常补帧最多 ~6 tick(60Hz)，此值应 > 6
  static constexpr int maxTicksPerFrame = 16;

  // ============================ 构造 ============================

  explicit Clock(float tickRate_ = 60.0f)
      : m_tickRate(tickRate_), m_tickDelta(1.0 / tickRate_),
        m_sys_start(std::chrono::system_clock::now()),
        m_mck_last(std::chrono::steady_clock::now()), m_scale(1.0), m_ticks(0),
        m_logic_reg(0.0), m_real_acc(0.0) {}

  // ============================ 单例 ============================

  static Clock &getClock() {
    static Clock s_clk;
    return s_clk;
  }

  // ============================ 查询 ============================

  int64_t getTick() const { return m_ticks; }
  float getTickRate() const { return m_tickRate; }
  double getTickDelta() const { return m_tickDelta; }

  double getRealAcc() const { return m_real_acc; }
  /// 连续逻辑时间
  double getLogicAcc() const { return m_ticks * m_tickDelta + m_logic_reg; }
  /// tick对齐逻辑时间
  double getLogicTime() const { return m_ticks * m_tickDelta; }

  /// 渲染插值因子 alpha = reg / Δ ∈ [0,1)：两固定状态间插值位置
  double getAlpha() const { return m_logic_reg / m_tickDelta; }
  double getScale() const { return m_scale; }
  void setScale(double scale_) { m_scale = scale_; }

  /// 系统日历时间（存档/日志）
  std::time_t getSysTime() const {
    return std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
  }
  std::time_t getSysStart() const {
    return std::chrono::system_clock::to_time_t(m_sys_start);
  }

  // ============================ 驱动（注入式） ============================

  /// 主循环调用：传入当前单调时间点，内部计算与上一帧的差
  void update(std::chrono::time_point<std::chrono::steady_clock> steady_now) {
    const double dur =
        std::chrono::duration<double>(steady_now - m_mck_last).count();
    addDuration(dur);
    m_mck_last = steady_now;
  }

  /// 直接注入一段真实时长（秒）——供回放/测试/网络同步喂受控时间
  void addDuration(double dur) {
    // 真实时间：记录全部（不钳制）
    m_real_acc += dur;
    // 尖峰保护：只钳逻辑侧，防物理爆炸
    dur = std::min(dur, maxFrameTime);
    m_logic_reg += dur * m_scale;
    // 固定步长补帧 + 防死亡螺旋
    int ticks = 0;
    while (m_logic_reg >= m_tickDelta && ticks < maxTicksPerFrame) {
      m_logic_reg -= m_tickDelta;
      ++m_ticks;
      ++ticks;
      clockUpdated();
    }
    // 超限丢弃：若 reg 仍超过步长（死亡螺旋），清掉残余防无限积压
    if (m_logic_reg >= m_tickDelta) {
      m_logic_reg = 0.0;
    }
  }

  // ============================ 回调（1:N 每 tick 通知）
  // ============================

  /// 每 tick 触发一次（占位：后续接信号/回调系统）
  void clockUpdated() {}

private:
  int64_t m_ticks;    // 逻辑 tick 计数
  double m_scale;     // 时间缩放
  double m_logic_reg; // 逻辑时长寄存器（不足一步的残余）
  double m_real_acc;  // 真实时长累加
  float m_tickRate;   // 固定逻辑帧率
  double m_tickDelta; // 固定步长（秒）
  std::chrono::time_point<std::chrono::system_clock>
      m_sys_start; // 启动日历时间戳
  std::chrono::time_point<std::chrono::steady_clock>
      m_mck_last; // 上一帧单调时间点
};

} // namespace clockns
