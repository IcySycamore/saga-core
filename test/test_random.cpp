// test/test_random.cpp
// RandomEng 种子注入确定性测试（#12：随机种子可复现）
//
// 编译与运行:
//   cmake --build build/vscodeBuild --target test_random &&
//   .\build\vscodeBuild\test_random.exe

#include "core/helper/RandomEng.h"
#include <cstdint>
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

// 取前 N 个原始随机值（避免依赖具体分布）
static void takeN(std::uint32_t *out, int n) {
  for (int i = 0; i < n; ++i)
    out[i] = randomEng()();
}

// ===================== 1: 同种子 → 同序列 =====================
static void test_same_seed_same_sequence() {
  TEST("same_seed_same_sequence");

  std::uint32_t a[5], b[5];
  setSeed(42);
  takeN(a, 5);
  setSeed(42);
  takeN(b, 5);
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(a[i], b[i]);
  }
}

// ===================== 2: 不同种子 → 不同序列 =====================
static void test_diff_seed_diff_sequence() {
  TEST("diff_seed_diff_sequence");

  std::uint32_t a[5], b[5];
  setSeed(1);
  takeN(a, 5);
  setSeed(2);
  takeN(b, 5);
  bool allSame = true;
  for (int i = 0; i < 5; ++i) {
    if (a[i] != b[i]) {
      allSame = false;
      break;
    }
  }
  EXPECT(!allSame, "different seeds should give different sequences");
}

// ===================== 3: 连续取值推进序列 =====================
static void test_sequence_advances() {
  TEST("sequence_advances");

  setSeed(7);
  std::uint32_t first = randomEng()();
  std::uint32_t second = randomEng()();
  EXPECT(first != second, "raw draws should advance the sequence");
}

// ===================== 4: reseedRandom 恢复非确定性 =====================
static void test_reseed_random() {
  TEST("reseed_random");

  // 注入固定种子后再重随机：序列应不同于固定种子序列（极大概率）
  setSeed(100);
  std::uint32_t a = randomEng()();
  resetSeed();
  std::uint32_t b = randomEng()();
  EXPECT(a != b, "reseed should diverge from fixed-seed sequence");
}

// ===================== main =====================
int main() {
  std::cout << "=== RandomEng Determinism Tests ===" << std::endl;

  test_same_seed_same_sequence();
  test_diff_seed_diff_sequence();
  test_sequence_advances();
  test_reseed_random();

  std::cout << "\n=== Results: " << g_passed << " passed, " << g_failed
            << " failed ===" << std::endl;
  return g_failed == 0 ? 0 : 1;
}
