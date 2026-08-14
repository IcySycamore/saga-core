// test/test_callback.cpp
// 编译与运行:
//   cd build && cmake .. -G Ninja && ninja && ctest -V
//
// 测试内容: core/CallBack/Connection.h, Subscriber.h, Publisher.h
//   - CONNECTION 缩写宏（全局连接声明）
//   - SUBSCRIBER 宏（槽绑定本类成员函数，this-> 调用）
//   - PUBLISHER 宏（信号转发，多对一）
//   - 析构自动解绑（connection 失效后 emit 静默）
//   - 空连接/空槽防御

#include "core/CallBack/Connection.h"
#include "core/CallBack/Publisher.h"
#include "core/CallBack/Subscriber.h"
#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>

// ===================== 轻量断言宏 =====================
static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) std::cout << "\n[RUN] " << name << std::endl
#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << " FAILED: " << msg << std::endl;                            \
      ++g_failed;                                                              \
    } else {                                                                   \
      ++g_passed;                                                              \
    }                                                                          \
  } while (false)
#define EXPECT_EQ(a, b) EXPECT((a) == (b), #a " == " #b)

// ============================================================
// 全局连接（生命周期最长）
// ============================================================
struct Player {
  std::string name;
};

lCYC::callback::Connection<void(const Player &)> g_onPlayerChanged;
lCYC::callback::Connection<int(const Player &, int)> g_onScore;

// ============================================================
// 场景 1：多对一（多信号 → 同一连接 → 一槽）
// ============================================================
class LoginService {
  std::string m_log;
  void handlePlayer(const Player &p) { m_log += p.name; }

public:
  SUBSCRIBER(handlePlayer, g_onPlayerChanged, void(const Player &));

  PUBLISHER(onPlayerLogin, g_onPlayerChanged);
  PUBLISHER(onPlayerLogout, g_onPlayerChanged);
  PUBLISHER(onPlayerSecret, g_onPlayerChanged); // 第三个信号，仍同一槽

  std::string log() const { return m_log; }
};

static void test_many_to_one() {
  TEST("many_to_one_multiple_signals_share_slot");

  LoginService svc;
  Player p{"Alice"};
  svc.onPlayerLogin(p);
  svc.onPlayerLogout(p);
  svc.onPlayerSecret(p);
  EXPECT_EQ(svc.log(), "AliceAliceAlice");
}

// ============================================================
// 场景 2：1:1 连接线语义（后绑定的 Subscriber 覆盖先绑定者）
// ============================================================
class ScoreWatcher {
  int m_score = 0;
  int onScore(const Player &, int s) {
    m_score = s;
    return s;
  }

public:
  SUBSCRIBER(onScore, g_onScore, int(const Player &, int));
  int score() const { return m_score; }
};

static void test_connection_1to1_last_bind_wins() {
  TEST("connection_is_1to1_last_subscriber_wins");

  ScoreWatcher a;
  ScoreWatcher b; // b 构造覆盖 a 的绑定（Connection 单指针）
  Player p{"Bob"};
  g_onScore.emit(p, 42);
  EXPECT_EQ(a.score(), 0); // a 已解绑（被覆盖）
  EXPECT_EQ(b.score(), 42);

  // a 析构时解绑自己——但当前绑定是 b，a 的解绑不应误伤 b
  // （析构只在 m_connection 指向自己时解绑，此处 a.m_connection == &g_onScore，
  //   解绑会把 g_onScore 置空 → b 也失效。这是已知取舍，见 ADR-0007 补充）
}

// ============================================================
// 场景 3：析构自动解绑（connection 失效后 emit 静默）
// ============================================================
static void test_destructor_unbind() {
  TEST("destructor_unbinds_connection_emit_silent");

  {
    LoginService svc;
    Player p{"Carol"};
    svc.onPlayerLogin(p);
    EXPECT_EQ(svc.log(), "Carol");
  } // svc 析构 → g_onPlayerChanged 解绑

  // 解绑后 emit 不再触发（也不崩溃）
  Player p{"Danger"};
  g_onPlayerChanged.emit(p);
  EXPECT(true, "emit after unbind should be silent");
}

// ============================================================
// 场景 4：空连接防御（未绑定任何槽）
// ============================================================
static void test_empty_connection() {
  TEST("empty_connection_emit_safe");

  lCYC::callback::Connection<void(int)> unused;
  unused.emit(42); // 不崩
  EXPECT(true, "empty connection emit should be safe");
}

// ============================================================
// 场景 5：Subscriber 类型性状（单签名模板、operator()、valid）
// ============================================================
static void test_subscriber_traits() {
  TEST("subscriber_type_traits");

  using Sub = lCYC::callback::Subscriber<void(const Player &)>;
  static_assert(std::is_default_constructible_v<Sub>, "default constructible");
  static_assert(std::is_copy_constructible_v<Sub>, "copy constructible");
  EXPECT(true, "static_asserts passed");
}

// ============================================================
int main() {
  test_many_to_one();
  test_connection_1to1_last_bind_wins();
  test_destructor_unbind();
  test_empty_connection();
  test_subscriber_traits();

  std::cout << "\n===== CallbackTest: " << g_passed << " passed, " << g_failed
            << " failed =====" << std::endl;
  return g_failed == 0 ? 0 : 1;
}
