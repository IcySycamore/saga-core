// test/test_time.cpp
// Clock 时间系统测试（#9，唯一引擎时钟）
//
// 编译与运行:
//   cmake --build build/vscodeBuild --target test_time &&
//   .\build\vscodeBuild\test_time.exe

#include "core/Time/Clock.h"
#include <chrono>
#include <cmath>
#include <ctime>
#include <iostream>

// ===================== 轻量断言宏 =====================
static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) std::cout << "\n[RUN] " << name << std::endl
#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "  FAILED: " << msg << std::endl;                           \
      ++g_failed;                                                              \
    } else {                                                                   \
      ++g_passed;                                                              \
    }                                                                          \
  } while (false)
#define EXPECT_EQ(a, b) EXPECT((a) == (b), #a " == " #b)
#define EXPECT_NEAR(a, b, eps)                                                 \
  EXPECT(std::abs((a) - (b)) <= (eps), #a " ≈ " #b " within " #eps)

// ===================== 1: 固定步长 =====================
static void test_fixed_step() {
  TEST("fixed_step");

  lCYC::clock::Clock clock;
  EXPECT_EQ(clock.getTickRate(), 60.0f);
  EXPECT_NEAR(clock.getTickDelta(), 1.0 / 60.0, 1e-9);

  // 0.1s 真实时间 → 60Hz → 6 次 tick
  clock.addDuration(0.1);
  EXPECT_EQ(clock.getTick(), 6);
  EXPECT_NEAR(clock.getLogicTime(), 6.0 / 60.0, 1e-9);
}

// ===================== 2: 累积器残余与 getLogicAcc =====================
static void test_accumulator() {
  TEST("accumulator_residual");

  lCYC::clock::Clock clock;
  // 0.005s 不足一个步长(0.0167) → 0 tick
  clock.addDuration(0.005);
  EXPECT_EQ(clock.getTick(), 0);
  EXPECT_NEAR(clock.getLogicAcc(), 0.005, 1e-9);
  // 再 0.012s → 累积 0.017 → 1 tick，残余保留
  clock.addDuration(0.012);
  EXPECT_EQ(clock.getTick(), 1);
  EXPECT_NEAR(clock.getLogicAcc(), 0.017, 1e-9);
  EXPECT_NEAR(clock.getLogicTime(), 1.0 / 60.0, 1e-9);
}

// ===================== 3: timeScale 慢放 =====================
static void test_time_scale_slowmo() {
  TEST("time_scale_slowmo");

  lCYC::clock::Clock clock;
  clock.setScale(0.5);

  clock.addDuration(0.1);
  EXPECT_EQ(clock.getTick(), 3);
  EXPECT_NEAR(clock.getLogicTime(), 3.0 / 60.0, 1e-9);
}

// ===================== 4: timeScale 暂停 =====================
static void test_time_scale_pause() {
  TEST("time_scale_pause");

  lCYC::clock::Clock clock;
  clock.setScale(0.0);

  clock.addDuration(0.5);
  EXPECT_EQ(clock.getTick(), 0);
  EXPECT_EQ(clock.getLogicTime(), 0.0);
}

// ===================== 5: 尖峰保护 =====================
static void test_spike_protection() {
  TEST("spike_protection");

  lCYC::clock::Clock clock;
  // 注入 5s（远超 maxFrameTime=0.1）→ 逻辑侧被钳制为 0.1s（6 tick）
  clock.addDuration(5.0);
  EXPECT_EQ(clock.getTick(), 6); // 0.1/0.0167
}

// ===================== 6: 防死亡螺旋 =====================
static void test_death_spiral() {
  TEST("death_spiral");

  lCYC::clock::Clock clock;
  // 0.5s 被钳制到 0.1 → 6 tick（正常补帧，不超 maxTicksPerFrame=16）
  clock.addDuration(0.5);
  EXPECT_EQ(clock.getTick(), 6);
  // 死亡螺旋保护：残余超步长时被清空（不会无限积压）
  // 注入 0.1s 后再注入小量，确认残余不会累积到第二次补 6 tick
  clock.addDuration(0.005); // 残余清理后，0.005 不足以补满一步
  EXPECT(clock.getTick() <= 7, "residual should not cause unbounded catchup");
}

// ===================== 7: getAlpha 渲染插值因子 =====================
static void test_alpha() {
  TEST("alpha");

  lCYC::clock::Clock clock;
  clock.addDuration(0.008); // 半个步长
  EXPECT_EQ(clock.getTick(), 0);
  EXPECT(clock.getAlpha() > 0.0 && clock.getAlpha() < 1.0,
         "alpha should be in (0,1) when reg < step");
  EXPECT_NEAR(clock.getAlpha(), 0.008 / (1.0 / 60.0), 1e-6);
}

// ===================== 8: 更换 tick rate =====================
static void test_change_tick_rate() {
  TEST("change_tick_rate");

  lCYC::clock::Clock clock(30.0f);
  EXPECT_EQ(clock.getTickRate(), 30.0f);
  EXPECT_NEAR(clock.getTickDelta(), 1.0 / 30.0, 1e-9);

  clock.addDuration(0.1);
  EXPECT_EQ(clock.getTick(), 3);
}

// ===================== 9: 系统时间接口 =====================
static void test_system_time() {
  TEST("system_time");

  lCYC::clock::Clock clock;
  std::time_t now = clock.getSysTime();
  std::time_t start = clock.getSysStart();
  EXPECT(now >= start, "sysStart should be <= sysNow");
}

// ===================== 10: update() 注入式 =====================
static void test_update_injection() {
  TEST("update_injection");

  lCYC::clock::Clock clock;
  using namespace std::chrono;
  auto t0 = steady_clock::now();
  auto t1 = t0 + milliseconds(50);
  clock.update(t1);
  // 50ms @ 60Hz → 3 tick
  EXPECT_EQ(clock.getTick(), 3);
  auto t2 = t1 + milliseconds(50);
  clock.update(t2);
  // 再 50ms → 共 6 tick
  EXPECT_EQ(clock.getTick(), 6);
}

// ===================== main =====================
int main() {
  std::cout << "=== Clock Time Tests ===" << std::endl;

  test_fixed_step();
  test_accumulator();
  test_time_scale_slowmo();
  test_time_scale_pause();
  test_spike_protection();
  test_death_spiral();
  test_alpha();
  test_change_tick_rate();
  test_system_time();
  test_update_injection();

  std::cout << "\n=== Results: " << g_passed << " passed, " << g_failed
            << " failed ===" << std::endl;

  return g_failed > 0 ? 1 : 0;
}
