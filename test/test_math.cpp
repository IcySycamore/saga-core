// test/test_math.cpp
// M1 常量与标量工具 + M2 Vector3 + M3 Vector2 + M4 Quaternion 测试
// 规格: docs/math/M1-math-constants.md, docs/math/M2-vector3.md,
//       docs/math/M3-vector2.md, docs/math/M4-quaternion.md
//
// 编译与运行:
//   cmake --build build/vscodeBuild --target test_math &&
//   .\build\vscodeBuild\test_math.exe

#include "core/math/Math.h"
#include "core/math/Matrix3x3.h"
#include "core/math/Matrix4x4.h"
#include "core/math/Quaternion.h"
#include "core/math/TransformTools.h"
#include "core/math/Vector2.h"
#include "core/math/Vector3.h"
#include <cmath>
#include <iostream>
#include <string>

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

// ===================== M1: 常量 =====================
static void test_constants() {
  TEST("M1_constants");

  EXPECT_NEAR(math::PI, 3.14159265358979, 1e-12);
  EXPECT_NEAR(math::TWO_PI, 2.0 * math::PI, 1e-12);
  EXPECT_NEAR(math::HALF_PI, 0.5 * math::PI, 1e-12);
  EXPECT_EQ(math::EPSILON, 1e-6f);
}

// ===================== M1: 角度转换 =====================
static void test_angle_conversion() {
  TEST("M1_angle_conversion");

  EXPECT_NEAR(math::deg2rad(180.0), math::PI, 1e-12);
  EXPECT_NEAR(math::deg2rad(90.0), math::HALF_PI, 1e-12);
  EXPECT_NEAR(math::rad2deg(math::PI), 180.0, 1e-12);
  EXPECT_NEAR(math::rad2deg(math::HALF_PI), 90.0, 1e-12);
  // float 版本
  EXPECT_NEAR(math::deg2rad(180.0f), static_cast<float>(math::PI), 1e-6f);
  EXPECT_NEAR(math::rad2deg(static_cast<float>(math::PI)), 180.0f, 1e-4f);
  // constexpr 编译期求值
  constexpr double half = math::deg2rad(90.0);
  static_assert(half == math::HALF_PI, "deg2rad should be constexpr");
}

// ===================== M1: clamp =====================
static void test_clamp() {
  TEST("M1_clamp");

  EXPECT_EQ(std::clamp(5, 0, 10), 5);
  EXPECT_EQ(std::clamp(-5, 0, 10), 0);
  EXPECT_EQ(std::clamp(15, 0, 10), 10);
  EXPECT_EQ(std::clamp(5.0f, 0.0f, 10.0f), 5.0f);
  // constexpr（std::clamp 约定 min <= max）
  constexpr int c = std::clamp(99, 0, 10);
  static_assert(c == 10, "std::clamp should be constexpr");
}

// ===================== M1: lerp =====================
static void test_lerp() {
  TEST("M1_lerp");

  EXPECT_EQ(std::lerp(0, 10, 0.5f), 5);
  EXPECT_EQ(std::lerp(10, 20, 0.0f), 10);
  EXPECT_EQ(std::lerp(10, 20, 1.0f), 20);
  EXPECT_NEAR(std::lerp(0.0f, 1.0f, 0.25f), 0.25f, 1e-6f);
  // std::lerp 端点健壮性：a==b 时返回 a（即使 t 溢出）
  EXPECT_EQ(std::lerp(7.0f, 7.0f, 1e10f), 7.0f);
}

// ===================== M1: approxEqual / smoothstep =====================
static void test_misc_tools() {
  TEST("M1_misc_tools");

  EXPECT(math::approxEqual(1.0f, 1.0f + 1e-7f), "within eps should be equal");
  EXPECT(!math::approxEqual(1.0f, 1.1f), "beyond eps should differ");
  EXPECT(math::approxEqual(0.1 + 0.2, 0.3, 1e-9), "double approx");
  // double 默认容差 1e-12（比 float 的 1e-6 严格）
  EXPECT(math::approxEqual(1.0, 1.0 + 1e-13), "double default eps 1e-12");
  EXPECT(!math::approxEqual(1.0, 1.0 + 1e-10),
         "double beyond 1e-12 should differ");
  EXPECT_EQ(math::smoothstep(0.0f), 0.0f);
  EXPECT_EQ(math::smoothstep(1.0f), 1.0f);
  EXPECT_EQ(math::smoothstep(0.5f), 0.5f);
  // GLSL 三参数版
  EXPECT_EQ(math::smoothstep(0.0f, 1.0f, 0.0f), 0.0f);  // x <= edge0 → 0
  EXPECT_EQ(math::smoothstep(0.0f, 1.0f, 1.0f), 1.0f);  // x >= edge1 → 1
  EXPECT_EQ(math::smoothstep(0.0f, 1.0f, -5.0f), 0.0f); // 区间外钳制
  EXPECT_EQ(math::smoothstep(0.0f, 1.0f, 5.0f), 1.0f);
  EXPECT_NEAR(math::smoothstep(0.0f, 1.0f, 0.5f), 0.5f, 1e-6f);     // 中点对称
  EXPECT_NEAR(math::smoothstep(2.0f, 4.0f, 3.0f), 0.5f, 1e-6f);     // 中点=1/2
  EXPECT_NEAR(math::smoothstep(2.0f, 4.0f, 3.5f), 0.84375f, 1e-5f); // t=0.75
  // 双精度版
  EXPECT_NEAR(math::smoothstep(0.0, 1.0, 0.5), 0.5, 1e-12);
  // constexpr 编译期求值
  constexpr float ss = math::smoothstep(0.0f, 1.0f, 0.5f);
  static_assert(ss == 0.5f, "smoothstep(3-arg) should be constexpr");
  // 标准库工具验证
  EXPECT_EQ(std::abs(-3), 3);
  EXPECT_EQ(std::min(3, 5), 3);
  EXPECT_EQ(std::max(3, 5), 5);
}

// ===================== M2: 构造与常量 =====================
static void test_vector3_construct() {
  TEST("M2_construct");

  math::Vector3 v0;
  EXPECT_EQ(v0.x, 0.0f);
  EXPECT_EQ(v0.y, 0.0f);
  EXPECT_EQ(v0.z, 0.0f);

  math::Vector3 v(1.0f, 2.0f, 3.0f);
  EXPECT_EQ(v.x, 1.0f);
  EXPECT_EQ(v.y, 2.0f);
  EXPECT_EQ(v.z, 3.0f);

  math::Vector3 s(5.0f); // 显式标量构造
  EXPECT_EQ(s.x, 5.0f);
  EXPECT_EQ(s.z, 5.0f);

  EXPECT_EQ(math::Vector3::zero().x, 0.0f);
  EXPECT_EQ(math::Vector3::one().y, 1.0f);
  EXPECT_EQ(math::Vector3::up().y, 1.0f);
  EXPECT_EQ(math::Vector3::forward().z, 1.0f);
  EXPECT_EQ(math::Vector3::right().x, 1.0f);
  EXPECT_EQ(math::Vector3::down().y, -1.0f);
}

// ===================== M2: 运算符 =====================
static void test_vector3_operators() {
  TEST("M2_operators");

  math::Vector3 a(1.0f, 2.0f, 3.0f);
  math::Vector3 b(4.0f, 5.0f, 6.0f);

  auto sum = a + b;
  EXPECT_EQ(sum.x, 5.0f);
  EXPECT_EQ(sum.y, 7.0f);
  EXPECT_EQ(sum.z, 9.0f);

  auto diff = a - b;
  EXPECT_EQ(diff.x, -3.0f);

  auto scaled = a * 2.0f;
  EXPECT_EQ(scaled.y, 4.0f);

  auto scaledL = 2.0f * a; // 左标量乘
  EXPECT_EQ(scaledL.z, 6.0f);

  auto divided = b / 2.0f;
  EXPECT_EQ(divided.x, 2.0f);

  auto neg = -a;
  EXPECT_EQ(neg.x, -1.0f);

  math::Vector3 c = a;
  c += b;
  EXPECT_EQ(c.x, 5.0f);
  c -= b;
  EXPECT_EQ(c.x, 1.0f);
  c *= 3.0f;
  EXPECT_EQ(c.y, 6.0f);
  c /= 3.0f;
  EXPECT_EQ(c.y, 2.0f);

  EXPECT(a == math::Vector3(1.0f, 2.0f, 3.0f), "equality");
  EXPECT(a != b, "inequality");

  // 索引访问
  EXPECT_EQ(a[0], 1.0f);
  EXPECT_EQ(a[1], 2.0f);
  EXPECT_EQ(a[2], 3.0f);
}

// ===================== M2: 点积/叉积 =====================
static void test_vector3_dot_cross() {
  TEST("M2_dot_cross");

  math::Vector3 a(1.0f, 0.0f, 0.0f);
  math::Vector3 b(0.0f, 1.0f, 0.0f);

  EXPECT_EQ(math::dot(a, b), 0.0f); // 正交
  EXPECT_EQ(math::dot(a, a), 1.0f); // 单位向量自点积
  EXPECT_EQ(math::dot(math::Vector3(1, 2, 3), math::Vector3(4, 5, 6)),
            32.0f); // 1*4+2*5+3*6

  auto cr = math::cross(a, b);
  EXPECT_EQ(cr.x, 0.0f);
  EXPECT_EQ(cr.y, 0.0f);
  EXPECT_EQ(cr.z, 1.0f); // right × up = forward

  // 叉积正交性
  math::Vector3 p(2.0f, 3.0f, 4.0f);
  math::Vector3 q(5.0f, 6.0f, 7.0f);
  auto c2 = math::cross(p, q);
  EXPECT_NEAR(math::dot(c2, p), 0.0f, 1e-5f);
  EXPECT_NEAR(math::dot(c2, q), 0.0f, 1e-5f);
}

// ===================== M2: 长度/距离 =====================
static void test_vector3_length() {
  TEST("M2_length_distance");

  math::Vector3 v(3.0f, 4.0f, 0.0f);
  EXPECT_EQ(math::lengthSquared(v), 25.0f);
  EXPECT_EQ(math::length(v), 5.0f);

  math::Vector3 a(0.0f, 0.0f, 0.0f);
  math::Vector3 b(3.0f, 4.0f, 0.0f);
  EXPECT_EQ(math::distance(a, b), 5.0f);
}

// ===================== M2: 归一化 =====================
static void test_vector3_normalize() {
  TEST("M2_normalize");

  math::Vector3 v(0.0f, 10.0f, 0.0f);
  auto n = math::normalized(v);
  EXPECT_NEAR(n.x, 0.0f, 1e-6f);
  EXPECT_NEAR(n.y, 1.0f, 1e-6f);
  EXPECT_NEAR(n.z, 0.0f, 1e-6f);
  EXPECT_NEAR(math::length(n), 1.0f, 1e-6f);

  // 零向量安全
  math::Vector3 zero(0.0f, 0.0f, 0.0f);
  auto nz = math::normalized(zero);
  EXPECT_EQ(nz.x, 0.0f);
  EXPECT_EQ(nz.y, 0.0f);
  EXPECT_EQ(nz.z, 0.0f);

  // 就地归一化
  math::Vector3 v2(0.0f, 0.0f, 5.0f);
  math::normalize(v2);
  EXPECT_NEAR(v2.z, 1.0f, 1e-6f);

  // 已单位向量归一化不改变
  math::Vector3 unit = math::normalized(math::Vector3(1.0f, 0.0f, 0.0f));
  EXPECT_NEAR(unit.x, 1.0f, 1e-6f);
}

// ===================== M2: clampMagnitude / angle / reflect
// =====================
static void test_vector3_misc() {
  TEST("M2_clamp_magnitude_angle_reflect");

  math::Vector3 v(3.0f, 4.0f, 0.0f); // len=5
  auto clamped = math::clampMagnitude(v, 2.0f);
  EXPECT_NEAR(math::length(clamped), 2.0f, 1e-5f);
  // 方向不变
  EXPECT_NEAR(clamped.x / clamped.y, 3.0f / 4.0f, 1e-5f);

  // 未超限不改变
  auto unchanged = math::clampMagnitude(v, 10.0f);
  EXPECT_NEAR(unchanged.x, 3.0f, 1e-6f);

  // 边界：maxLen = 0 → 归零（合法输入）
  auto zeroClamp = math::clampMagnitude(v, 0.0f);
  EXPECT_NEAR(math::length(zeroClamp), 0.0f, 1e-6f);
  // （负 maxLen 触发 assert 防护，debug 下不测）

  // 分量逐乘（Hadamard）
  auto h = math::Vector3(2.0f, 3.0f, 4.0f) * math::Vector3(5.0f, 6.0f, 7.0f);
  EXPECT_EQ(h.x, 10.0f);
  EXPECT_EQ(h.y, 18.0f);
  EXPECT_EQ(h.z, 28.0f);

  // angle
  math::Vector3 a(1.0f, 0.0f, 0.0f);
  math::Vector3 b(0.0f, 1.0f, 0.0f);
  EXPECT_NEAR(math::angle(a, b), math::HALF_PI, 1e-6f);
  EXPECT_EQ(math::angle(a, a), 0.0f);
  EXPECT_EQ(math::angle(a, math::Vector3::zero()), 0.0f); // 零向量安全

  // reflect
  math::Vector3 dir(1.0f, -1.0f, 0.0f);
  math::Vector3 normal(0.0f, 1.0f, 0.0f);
  auto refl = math::reflect(dir, normal);
  EXPECT_NEAR(refl.x, 1.0f, 1e-6f);
  EXPECT_NEAR(refl.y, 1.0f, 1e-6f);
  EXPECT_NEAR(refl.z, 0.0f, 1e-6f);
}

// ===================== M2: toString =====================
static void test_vector3_tostring() {
  TEST("M2_toString");

  math::Vector3 v(1.0f, 2.0f, 3.0f);
  std::string s = v.toString();
  EXPECT(s.find('(') != std::string::npos, "should contain '('");
  EXPECT(s.find(')') != std::string::npos, "should contain ')'");
  EXPECT(s.find("1.000000") != std::string::npos, "should contain x value");
}

// ===================== M3: Vector2 =====================
static void test_vector2_construct() {
  TEST("M3_Vector2_construct");

  math::Vector2 v0;
  EXPECT_EQ(v0.x, 0.0f);
  EXPECT_EQ(v0.y, 0.0f);

  math::Vector2 v(3.0f, 4.0f);
  EXPECT_EQ(v.x, 3.0f);
  EXPECT_EQ(v.y, 4.0f);

  EXPECT_EQ(math::Vector2::up().y, 1.0f);
  EXPECT_EQ(math::Vector2::right().x, 1.0f);

  // 索引访问
  EXPECT_EQ(v[0], 3.0f);
  EXPECT_EQ(v[1], 4.0f);
}

static void test_vector2_ops() {
  TEST("M3_Vector2_ops");

  math::Vector2 a(1.0f, 2.0f);
  math::Vector2 b(3.0f, 4.0f);

  auto sum = a + b;
  EXPECT_EQ(sum.x, 4.0f);
  EXPECT_EQ(sum.y, 6.0f);

  auto diff = a - b;
  EXPECT_EQ(diff.x, -2.0f);

  auto scaled = a * 2.0f;
  EXPECT_EQ(scaled.y, 4.0f);

  // 分量逐乘
  auto h = a * b;
  EXPECT_EQ(h.x, 3.0f);
  EXPECT_EQ(h.y, 8.0f);

  auto neg = -a;
  EXPECT_EQ(neg.x, -1.0f);

  // dot
  EXPECT_EQ(math::dot(a, b), 11.0f); // 1*3 + 2*4

  // 2D cross（标量）
  EXPECT_EQ(math::cross(math::Vector2(1.0f, 0.0f), math::Vector2(0.0f, 1.0f)),
            1.0f);
  EXPECT_EQ(math::cross(math::Vector2(0.0f, 1.0f), math::Vector2(1.0f, 0.0f)),
            -1.0f);

  // perp
  auto p = math::perp(math::Vector2(1.0f, 0.0f));
  EXPECT_NEAR(p.x, 0.0f, 1e-6f);
  EXPECT_NEAR(p.y, 1.0f, 1e-6f);

  // length
  EXPECT_EQ(math::length(math::Vector2(3.0f, 4.0f)), 5.0f);
  EXPECT_EQ(
      math::distance(math::Vector2(0.0f, 0.0f), math::Vector2(3.0f, 4.0f)),
      5.0f);

  // normalize
  auto n = math::normalized(math::Vector2(0.0f, 10.0f));
  EXPECT_NEAR(n.y, 1.0f, 1e-6f);
  auto nz = math::normalized(math::Vector2(0.0f, 0.0f));
  EXPECT_EQ(nz.x, 0.0f); // 零向量安全
}

static void test_vector2_rotate() {
  TEST("M3_Vector2_rotate");

  // 绕原点逆时针旋转 90°
  auto r90 = math::rotate(math::Vector2(1.0f, 0.0f), math::HALF_PI);
  EXPECT_NEAR(r90.x, 0.0f, 1e-5f);
  EXPECT_NEAR(r90.y, 1.0f, 1e-5f);

  // 旋转 180°
  auto r180 = math::rotate(math::Vector2(1.0f, 0.0f), math::PI);
  EXPECT_NEAR(r180.x, -1.0f, 1e-5f);
  EXPECT_NEAR(r180.y, 0.0f, 1e-5f);

  // 长度不变
  EXPECT_NEAR(math::length(r180), 1.0f, 1e-5f);

  // angle
  EXPECT_NEAR(math::angle(math::Vector2(1.0f, 0.0f), math::Vector2(0.0f, 1.0f)),
              math::HALF_PI, 1e-6f);
}

// ===================== M4: Quaternion =====================
static void test_quaternion_identity() {
  TEST("M4_identity");

  math::Quaternion q;
  EXPECT_EQ(q.x, 0.0f);
  EXPECT_EQ(q.y, 0.0f);
  EXPECT_EQ(q.z, 0.0f);
  EXPECT_EQ(q.w, 1.0f);

  // 单位四元数旋转向量不变
  math::Vector3 v(1.0f, 2.0f, 3.0f);
  auto r = q * v;
  EXPECT_NEAR(r.x, 1.0f, 1e-6f);
  EXPECT_NEAR(r.y, 2.0f, 1e-6f);
  EXPECT_NEAR(r.z, 3.0f, 1e-6f);
}

static void test_quaternion_axis_angle() {
  TEST("M4_axisAngle");

  // 绕 Z 轴旋转 90°
  auto q = math::axisAngle(math::Vector3::forward(), math::HALF_PI);
  auto v = q * math::Vector3::right();
  EXPECT_NEAR(v.x, 0.0f, 1e-5f);
  EXPECT_NEAR(v.y, 1.0f, 1e-5f);
  EXPECT_NEAR(v.z, 0.0f, 1e-5f);

  // 绕 Z 轴旋转 180°
  auto q180 = math::axisAngle(math::Vector3::forward(), math::PI);
  auto v180 = q180 * math::Vector3::right();
  EXPECT_NEAR(v180.x, -1.0f, 1e-5f);
  EXPECT_NEAR(v180.y, 0.0f, 1e-5f);

  // 旋转保持长度
  EXPECT_NEAR(math::length(v180), 1.0f, 1e-5f);

  // 零轴安全 → 单位四元数
  auto qz = math::axisAngle(math::Vector3::zero(), math::HALF_PI);
  auto vz = qz * math::Vector3::right();
  EXPECT_NEAR(vz.x, 1.0f, 1e-6f); // 不旋转
}

static void test_quaternion_composition() {
  TEST("M4_composition");

  // 先绕 Z 转 90° 再绕 X 转 90° = 复合
  auto qz = math::axisAngle(math::Vector3::forward(), math::HALF_PI);
  auto qx = math::axisAngle(math::Vector3::right(), math::HALF_PI);

  // 先应用 qz 再应用 qx
  auto composed = qx * qz;
  auto v = composed * math::Vector3::right();
  // 结果: 先右→上，再上→(绕X转90后的上 = 前)
  EXPECT_NEAR(v.x, 0.0f, 1e-5f);
  EXPECT_NEAR(v.y, 0.0f, 1e-5f);
  EXPECT_NEAR(v.z, 1.0f, 1e-5f);
}

static void test_quaternion_inverse() {
  TEST("M4_inverse");

  auto q = math::axisAngle(math::Vector3::up(), math::deg2rad(60.0f));
  auto inv = q.inverse();

  // q * q⁻¹ = 单位
  auto product = q * inv;
  EXPECT_NEAR(product.x, 0.0f, 1e-5f);
  EXPECT_NEAR(product.y, 0.0f, 1e-5f);
  EXPECT_NEAR(product.z, 0.0f, 1e-5f);
  EXPECT_NEAR(product.w, 1.0f, 1e-5f);

  // 旋转后逆旋转 = 不变
  math::Vector3 v(1.0f, 2.0f, 3.0f);
  auto rotated = q * v;
  auto restored = inv * rotated;
  EXPECT_NEAR(restored.x, 1.0f, 1e-4f);
  EXPECT_NEAR(restored.y, 2.0f, 1e-4f);
  EXPECT_NEAR(restored.z, 3.0f, 1e-4f);

  // 零四元数防御
  auto qz = math::Quaternion(0, 0, 0, 0);
  auto iz = qz.inverse();
  EXPECT_EQ(iz.w, 1.0f); // 返回单位
}

static void test_quaternion_euler() {
  TEST("M4_euler");

  // 纯绕 Z 的欧拉角（roll 90°）
  auto q = math::euler(0.0f, 0.0f, math::HALF_PI);
  auto v = q * math::Vector3::right();
  EXPECT_NEAR(v.x, 0.0f, 1e-5f);
  EXPECT_NEAR(v.y, 1.0f, 1e-5f);

  // 纯绕 Y（yaw 90°）：right → back
  auto qy = math::euler(0.0f, math::HALF_PI, 0.0f);
  auto vy = qy * math::Vector3::right();
  EXPECT_NEAR(vy.x, 0.0f, 1e-5f);
  EXPECT_NEAR(vy.z, -1.0f, 1e-5f);
}

static void test_quaternion_slerp() {
  TEST("M4_slerp");

  auto q0 = math::Quaternion::identity();
  auto q1 = math::axisAngle(math::Vector3::forward(), math::HALF_PI);

  // t=0 → q0
  auto s0 = math::slerp(q0, q1, 0.0f);
  EXPECT_NEAR(s0.w, 1.0f, 1e-5f);

  // t=1 → q1
  auto s1 = math::slerp(q0, q1, 1.0f);
  EXPECT_NEAR(s1.x, q1.x, 1e-5f);
  EXPECT_NEAR(s1.w, q1.w, 1e-5f);

  // t=0.5 → 绕 Z 转 45°
  auto s05 = math::slerp(q0, q1, 0.5f);
  auto v = s05 * math::Vector3::right();
  EXPECT_NEAR(v.x, std::cos(math::HALF_PI * 0.5f), 1e-4f);
  EXPECT_NEAR(v.y, std::sin(math::HALF_PI * 0.5f), 1e-4f);

  // 结果单位化
  EXPECT_NEAR(s05.length(), 1.0f, 1e-5f);

  // 边界：t 超范围被钳制（t<0 等价 t=0，t>1 等价 t=1）
  auto sNeg = math::slerp(q0, q1, -1.0f);
  EXPECT_NEAR(sNeg.w, 1.0f, 1e-5f); // 钳制到 0
  auto sOver = math::slerp(q0, q1, 2.0f);
  EXPECT_NEAR(sOver.x, q1.x, 1e-5f); // 钳制到 1
  EXPECT_NEAR(sOver.w, q1.w, 1e-5f);
}

static void test_quaternion_look_rotation() {
  TEST("M4_lookRotation");

  // 朝向 +Z
  auto q = math::lookRotation(math::Vector3::forward(), math::Vector3::up());
  auto v = q * math::Vector3::forward();
  EXPECT_NEAR(v.z, 1.0f, 1e-5f);

  // 朝向 +X（up 默认）
  auto qx = math::lookRotation(math::Vector3::right(), math::Vector3::up());
  auto vx = qx * math::Vector3::forward();
  EXPECT_NEAR(vx.x, 1.0f, 1e-4f);
  EXPECT_NEAR(vx.y, 0.0f, 1e-4f);
  EXPECT_NEAR(vx.z, 0.0f, 1e-4f);

  // 万向锁：forward == up（应回退到世界 up）
  auto qg = math::lookRotation(math::Vector3::up(), math::Vector3::up());
  auto vg = qg * math::Vector3::forward();
  EXPECT_NEAR(math::length(vg), 1.0f, 1e-4f); // 至少不崩溃、长度保持

  // 零向量防御
  auto qz = math::lookRotation(math::Vector3::zero(), math::Vector3::up());
  EXPECT_EQ(qz.w, 1.0f);
}

// ===================== M5: Matrix4x4 =====================
static void test_mat4_identity() {
  TEST("M5_identity");

  math::Matrix4x4 m;
  // 单位矩阵：对角 1，其余 0
  EXPECT_EQ(m.at(0, 0), 1.0f);
  EXPECT_EQ(m.at(1, 1), 1.0f);
  EXPECT_EQ(m.at(2, 2), 1.0f);
  EXPECT_EQ(m.at(3, 3), 1.0f);
  EXPECT_EQ(m.at(0, 1), 0.0f);
  EXPECT_EQ(m.at(1, 0), 0.0f);
  EXPECT_EQ(m.at(2, 3), 0.0f);

  // 单位矩阵变换向量不变
  math::Vector3 v(1.0f, 2.0f, 3.0f);
  auto r = m * v;
  EXPECT_NEAR(r.x, 1.0f, 1e-6f);
  EXPECT_NEAR(r.y, 2.0f, 1e-6f);
  EXPECT_NEAR(r.z, 3.0f, 1e-6f);

  // data() 返回连续内存
  EXPECT(m.data() != nullptr, "data() should be non-null");
}

static void test_mat4_translation() {
  TEST("M5_translation");

  auto t = math::translation(math::Vector3(10.0f, 20.0f, 30.0f));
  // 列主序：平移在 m[12..14]
  EXPECT_EQ(t.at(0, 3), 10.0f);
  EXPECT_EQ(t.at(1, 3), 20.0f);
  EXPECT_EQ(t.at(2, 3), 30.0f);

  math::Vector3 v(1.0f, 2.0f, 3.0f);
  auto r = t * v;
  EXPECT_NEAR(r.x, 11.0f, 1e-6f);
  EXPECT_NEAR(r.y, 22.0f, 1e-6f);
  EXPECT_NEAR(r.z, 33.0f, 1e-6f);
}

static void test_mat4_scale() {
  TEST("M5_scale");

  auto s = math::scale(math::Vector3(2.0f, 3.0f, 4.0f));
  EXPECT_EQ(s.at(0, 0), 2.0f);
  EXPECT_EQ(s.at(1, 1), 3.0f);
  EXPECT_EQ(s.at(2, 2), 4.0f);

  math::Vector3 v(1.0f, 1.0f, 1.0f);
  auto r = s * v;
  EXPECT_NEAR(r.x, 2.0f, 1e-6f);
  EXPECT_NEAR(r.y, 3.0f, 1e-6f);
  EXPECT_NEAR(r.z, 4.0f, 1e-6f);
}

static void test_mat4_rotation() {
  TEST("M5_rotation");

  // 绕 Z 轴 90°（与 Quaternion 一致）
  auto q = math::axisAngle(math::Vector3::forward(), math::HALF_PI);
  auto rot = math::rotation(q);

  auto v = rot * math::Vector3::right();
  EXPECT_NEAR(v.x, 0.0f, 1e-5f);
  EXPECT_NEAR(v.y, 1.0f, 1e-5f);
  EXPECT_NEAR(v.z, 0.0f, 1e-5f);
}

static void test_mat4_multiply() {
  TEST("M5_multiply");

  // 先缩放再平移: M = T * S（先应用 S 再应用 T）
  auto s = math::scale(math::Vector3(2.0f, 2.0f, 2.0f));
  auto t = math::translation(math::Vector3(10.0f, 0.0f, 0.0f));
  auto m = t * s;

  math::Vector3 v(1.0f, 1.0f, 1.0f);
  auto r = m * v;
  // 先缩放 → (2,2,2)，再平移 → (12,2,2)
  EXPECT_NEAR(r.x, 12.0f, 1e-6f);
  EXPECT_NEAR(r.y, 2.0f, 1e-6f);
  EXPECT_NEAR(r.z, 2.0f, 1e-6f);

  // 顺序敏感：s * t ≠ t * s
  auto m2 = s * t;
  auto r2 = m2 * v;
  // 先平移 → (11,1,1)，再缩放 → (22,2,2)
  EXPECT_NEAR(r2.x, 22.0f, 1e-6f);
  EXPECT_NEAR(r2.y, 2.0f, 1e-6f);
}

static void test_mat4_transpose() {
  TEST("M5_transpose");

  auto t = math::translation(math::Vector3(1.0f, 2.0f, 3.0f));
  auto tt = t.transposed();
  // 转置后平移在行
  EXPECT_EQ(tt.at(3, 0), 1.0f);
  EXPECT_EQ(tt.at(3, 1), 2.0f);
  EXPECT_EQ(tt.at(3, 2), 3.0f);

  // 双重转置还原
  auto back = tt.transposed();
  EXPECT(back == t, "double transpose should restore");
}

static void test_mat4_inverse() {
  TEST("M5_inverse");

  // TRS 矩阵
  auto s = math::scale(math::Vector3(2.0f, 3.0f, 4.0f));
  auto t = math::translation(math::Vector3(10.0f, 20.0f, 30.0f));
  auto m = t * s;

  // M * M⁻¹ = I
  auto inv = m.inverse();
  auto prod = m * inv;
  EXPECT_NEAR(prod.at(0, 0), 1.0f, 1e-4f);
  EXPECT_NEAR(prod.at(1, 1), 1.0f, 1e-4f);
  EXPECT_NEAR(prod.at(2, 2), 1.0f, 1e-4f);
  EXPECT_NEAR(prod.at(3, 3), 1.0f, 1e-4f);
  EXPECT_NEAR(prod.at(0, 1), 0.0f, 1e-4f);
  EXPECT_NEAR(prod.at(1, 0), 0.0f, 1e-4f);
  EXPECT_NEAR(prod.at(0, 3), 0.0f, 1e-4f);

  // 变换后逆变换还原
  math::Vector3 v(1.0f, 2.0f, 3.0f);
  auto mv = m * v;
  auto restored = inv * mv;
  EXPECT_NEAR(restored.x, 1.0f, 1e-4f);
  EXPECT_NEAR(restored.y, 2.0f, 1e-4f);
  EXPECT_NEAR(restored.z, 3.0f, 1e-4f);

  // 不可逆矩阵防御（全零 → 返回单位）
  math::Matrix4x4 zero;
  for (int i = 0; i < 16; ++i)
    zero[i] = 0.0f;
  auto zi = zero.inverse();
  EXPECT_EQ(zi.at(0, 0), 1.0f); // 单位矩阵
}

static void test_mat4_determinant() {
  TEST("M5_determinant");

  // 单位矩阵 det = 1
  math::Matrix4x4 id;
  EXPECT_NEAR(id.determinant(), 1.0f, 1e-5f);

  // 缩放矩阵 det = 乘积
  auto s = math::scale(math::Vector3(2.0f, 3.0f, 4.0f));
  EXPECT_NEAR(s.determinant(), 24.0f, 1e-4f);

  // 平移矩阵 det = 1（不改变体积）
  auto t = math::translation(math::Vector3(1.0f, 2.0f, 3.0f));
  EXPECT_NEAR(t.determinant(), 1.0f, 1e-5f);
}

static void test_mat4_projection() {
  TEST("M5_projection");

  // 正交投影：中心点 → 原点附近
  auto ortho = math::orthographic(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
  auto center = ortho * math::Vector3(0.0f, 0.0f, -50.0f);
  EXPECT_NEAR(center.x, 0.0f, 1e-4f);
  EXPECT_NEAR(center.y, 0.0f, 1e-4f);

  // 透视：近平面中心点保持
  auto persp = math::perspective(math::deg2rad(60.0f), 1.0f, 0.1f, 100.0f);
  auto pv = persp * math::Vector3(0.0f, 0.0f, -0.1f); // 近平面
  EXPECT_NEAR(pv.x, 0.0f, 1e-4f);
  EXPECT_NEAR(pv.y, 0.0f, 1e-4f);
}

static void test_mat4_lookat() {
  TEST("M5_lookAt");

  // 相机在原点看向 -Z（默认前方）
  auto view = math::lookAt(math::Vector3::zero(), math::Vector3::back(),
                           math::Vector3::up());
  auto v = view * math::Vector3::back();
  // 相机看向 -Z：back(-Z) 处的物体在视空间前方 -Z
  EXPECT_NEAR(v.x, 0.0f, 1e-4f);
  EXPECT_NEAR(v.y, 0.0f, 1e-4f);
  EXPECT_NEAR(v.z, -1.0f, 1e-4f);

  // 相机看向 +Z：+Z 处物体在视空间前方 -Z
  auto view2 = math::lookAt(math::Vector3::zero(), math::Vector3::forward(),
                            math::Vector3::up());
  auto v2 = view2 * math::Vector3::forward();
  EXPECT_NEAR(v2.x, 0.0f, 1e-4f);
  EXPECT_NEAR(v2.y, 0.0f, 1e-4f);
  EXPECT_NEAR(v2.z, -1.0f, 1e-4f);

  // 边界：eye == target → 返回单位矩阵（不产生 NaN）
  auto viewId = math::lookAt(math::Vector3(1, 2, 3), math::Vector3(1, 2, 3),
                             math::Vector3::up());
  auto vid = viewId * math::Vector3(1.0f, 1.0f, 1.0f);
  EXPECT(vid.x == vid.x && vid.y == vid.y && vid.z == vid.z,
         "eye==target should not produce NaN");
  EXPECT_NEAR(viewId.at(0, 0), 1.0f, 1e-6f); // 单位矩阵

  // 边界：up ∥ forward（万向锁）→ 不产生 NaN
  auto viewG = math::lookAt(math::Vector3::zero(), math::Vector3::up(),
                            math::Vector3::up());
  auto vg = viewG * math::Vector3::up();
  EXPECT(vg.x == vg.x && vg.y == vg.y && vg.z == vg.z,
         "gimbal lock should not produce NaN");
}

// ===================== M6: Matrix3x3 =====================
static void test_mat3_identity() {
  TEST("M6_identity");

  math::Matrix3x3 m;
  EXPECT_EQ(m.at(0, 0), 1.0f);
  EXPECT_EQ(m.at(1, 1), 1.0f);
  EXPECT_EQ(m.at(2, 2), 1.0f);
  EXPECT_EQ(m.at(0, 1), 0.0f);

  math::Vector3 v(1.0f, 2.0f, 3.0f);
  auto r = m * v;
  EXPECT_NEAR(r.x, 1.0f, 1e-6f);
  EXPECT_NEAR(r.z, 3.0f, 1e-6f);
}

static void test_mat3_rotation() {
  TEST("M6_rotation");

  // 绕 Z 轴 90°（列主序：col0=(0,-1,0), col1=(1,0,0)）
  auto rz = math::rotationZ(math::HALF_PI);
  auto v = rz * math::Vector3::right();
  EXPECT_NEAR(v.x, 0.0f, 1e-5f);
  EXPECT_NEAR(v.y, 1.0f, 1e-5f);

  // 绕 X 轴 90°：up → forward
  auto rx = math::rotationX(math::HALF_PI);
  auto vu = rx * math::Vector3::up();
  EXPECT_NEAR(vu.z, 1.0f, 1e-5f);

  // 绕 Y 轴 90°：right → back
  auto ry = math::rotationY(math::HALF_PI);
  auto vr = ry * math::Vector3::right();
  EXPECT_NEAR(vr.z, -1.0f, 1e-5f);

  // 四元数构造矩阵与绕轴一致
  auto q = math::axisAngle(math::Vector3::forward(), math::HALF_PI);
  auto mq = math::rotation3x3(q);
  auto vq = mq * math::Vector3::right();
  EXPECT_NEAR(vq.x, 0.0f, 1e-5f);
  EXPECT_NEAR(vq.y, 1.0f, 1e-5f);
}

static void test_mat3_inverse_det() {
  TEST("M6_inverse_det");

  auto s = math::scale(math::Vector3(2.0f, 3.0f, 4.0f));
  EXPECT_NEAR(s.determinant(), 24.0f, 1e-4f);

  auto inv = s.inverse();
  auto prod = s * inv;
  EXPECT_NEAR(prod.at(0, 0), 1.0f, 1e-5f);
  EXPECT_NEAR(prod.at(1, 1), 1.0f, 1e-5f);
  EXPECT_NEAR(prod.at(2, 2), 1.0f, 1e-5f);

  // 旋转矩阵逆 = 转置
  auto rz = math::rotationZ(math::deg2rad(30.0f));
  auto invR = rz.inverse();
  auto trR = rz.transposed();
  for (int i = 0; i < 9; ++i) {
    EXPECT_NEAR(invR[i], trR[i], 1e-5f);
  }
}

static void test_mat3_2d() {
  TEST("M6_2d");

  // 2D 旋转 90°
  auto r2d = math::rotation2D(math::HALF_PI);
  auto v = r2d * math::Vector2(1.0f, 0.0f);
  EXPECT_NEAR(v.x, 0.0f, 1e-5f);
  EXPECT_NEAR(v.y, 1.0f, 1e-5f);

  // 2D 平移
  auto t2d = math::translation2D(10.0f, 20.0f);
  auto p = t2d * math::Vector2(1.0f, 2.0f);
  EXPECT_NEAR(p.x, 11.0f, 1e-5f);
  EXPECT_NEAR(p.y, 22.0f, 1e-5f);
}

static void test_mat3_4x4_convert() {
  TEST("M6_4x4_convert");

  auto m4 =
      math::translation(math::Vector3(1.0f, 2.0f, 3.0f)) *
      math::rotation(math::axisAngle(math::Vector3::forward(), math::HALF_PI));
  auto m3 = math::Matrix3x3::fromMatrix4x4(m4);
  // 3x3 提取应保留旋转部分
  auto v = m3 * math::Vector3::right();
  EXPECT_NEAR(v.x, 0.0f, 1e-5f);
  EXPECT_NEAR(v.y, 1.0f, 1e-5f);

  // 3x3 → 4x4 往返
  auto back = m3.toMatrix4x4();
  EXPECT_NEAR(back.at(0, 3), 0.0f, 1e-6f); // 无平移
  EXPECT_NEAR(back.at(0, 0), m3.at(0, 0), 1e-6f);
}

static void test_mat3_normal_matrix() {
  TEST("M6_normalMatrix");

  // 非均匀缩放下的法线矩阵（3x3）
  auto s = math::scale3x3(math::Vector3(2.0f, 1.0f, 1.0f));
  auto nm = s.normalMatrix();

  // 法线 (1,0,0) 经缩放后应仍垂直（非均匀缩放下法线需逆转置）
  math::Vector3 normal(1.0f, 0.0f, 0.0f);
  auto transformed = nm * normal;
  EXPECT_NEAR(transformed.x, 0.5f, 1e-5f); // 逆转置 = 1/scale
  EXPECT_NEAR(transformed.y, 0.0f, 1e-5f);
}

// ===================== M7: TransformTools =====================
static void test_trs() {
  TEST("M7_trs");

  auto m = math::trs(math::Vector3(10.0f, 20.0f, 30.0f),
                     math::axisAngle(math::Vector3::forward(), math::HALF_PI),
                     math::Vector3(2.0f, 2.0f, 2.0f));
  // 先缩放再旋转再平移: v=(1,0,0) → scale(2,0,0) → rot(0,2,0) → trans(10,22,30)
  auto v = m * math::Vector3(1.0f, 0.0f, 0.0f);
  EXPECT_NEAR(v.x, 10.0f, 1e-5f);
  EXPECT_NEAR(v.y, 22.0f, 1e-5f);
  EXPECT_NEAR(v.z, 30.0f, 1e-5f);
}

static void test_decompose() {
  TEST("M7_decompose");

  math::Vector3 pos, scale;
  math::Quaternion rot;
  auto m = math::trs(math::Vector3(1.0f, 2.0f, 3.0f),
                     math::axisAngle(math::Vector3::up(), math::deg2rad(45.0f)),
                     math::Vector3(2.0f, 3.0f, 4.0f));

  bool ok = math::decomposeTRS(m, pos, rot, scale);
  EXPECT(ok, "decompose should succeed");

  // 位置还原
  EXPECT_NEAR(pos.x, 1.0f, 1e-4f);
  EXPECT_NEAR(pos.y, 2.0f, 1e-4f);
  EXPECT_NEAR(pos.z, 3.0f, 1e-4f);
  // 缩放还原
  EXPECT_NEAR(scale.x, 2.0f, 1e-4f);
  EXPECT_NEAR(scale.y, 3.0f, 1e-4f);
  EXPECT_NEAR(scale.z, 4.0f, 1e-4f);
  // 旋转还原（与原始一致）
  auto orig = math::axisAngle(math::Vector3::up(), math::deg2rad(45.0f));
  EXPECT(std::abs(math::dot(rot, orig)) > 0.999f,
         "rotation should be restored (up to sign)");

  // 往返重建
  auto rebuilt = math::trs(pos, rot, scale);
  auto test = rebuilt * math::Vector3(1.0f, 1.0f, 1.0f);
  auto ref = m * math::Vector3(1.0f, 1.0f, 1.0f);
  EXPECT_NEAR(test.x, ref.x, 1e-4f);
  EXPECT_NEAR(test.y, ref.y, 1e-4f);
  EXPECT_NEAR(test.z, ref.z, 1e-4f);
}

static void test_euler_roundtrip() {
  TEST("M7_euler_roundtrip");

  // 非奇点往返
  math::Vector3 e(0.3f, 0.5f, -0.2f);
  auto q = math::eulerToQuat(e);
  auto back = math::quatToEuler(q);
  EXPECT_NEAR(back.x, e.x, 1e-4f);
  EXPECT_NEAR(back.y, e.y, 1e-4f);
  EXPECT_NEAR(back.z, e.z, 1e-4f);

  // 纯 roll 90°
  auto qr = math::eulerToQuat(0.0f, 0.0f, math::HALF_PI);
  auto er = math::quatToEuler(qr);
  EXPECT_NEAR(er.z, math::HALF_PI, 1e-4f);

  // 欧拉→矩阵→欧拉
  auto m = math::eulerToMatrix(e);
  auto em = math::matrixToEuler(m);
  EXPECT_NEAR(em.x, e.x, 1e-3f);
  EXPECT_NEAR(em.y, e.y, 1e-3f);
  EXPECT_NEAR(em.z, e.z, 1e-3f);
}

static void test_transform_point_direction() {
  TEST("M7_transformPointDirection");

  auto m =
      math::trs(math::Vector3(10.0f, 0.0f, 0.0f), math::Quaternion::identity());
  // transformPoint 带平移
  auto p = math::transformPoint(m, math::Vector3(1.0f, 2.0f, 3.0f));
  EXPECT_NEAR(p.x, 11.0f, 1e-6f);
  // transformDirection 不带平移
  auto d = math::transformDirection(m, math::Vector3(1.0f, 0.0f, 0.0f));
  EXPECT_NEAR(d.x, 1.0f, 1e-6f); // 方向不受平移影响
}

static void test_rotate_vector() {
  TEST("M7_rotateVector");

  auto q = math::axisAngle(math::Vector3::forward(), math::HALF_PI);
  auto v = math::rotateVector(q, math::Vector3::right());
  EXPECT_NEAR(v.x, 0.0f, 1e-5f);
  EXPECT_NEAR(v.y, 1.0f, 1e-5f);
}

// ===================== main =====================
int main() {
  std::cout << "=== Math Library Tests (M1-M7) ===" << std::endl;

  test_constants();
  test_angle_conversion();
  test_clamp();
  test_lerp();
  test_misc_tools();

  test_vector3_construct();
  test_vector3_operators();
  test_vector3_dot_cross();
  test_vector3_length();
  test_vector3_normalize();
  test_vector3_misc();
  test_vector3_tostring();

  test_vector2_construct();
  test_vector2_ops();
  test_vector2_rotate();

  test_quaternion_identity();
  test_quaternion_axis_angle();
  test_quaternion_composition();
  test_quaternion_inverse();
  test_quaternion_euler();
  test_quaternion_slerp();
  test_quaternion_look_rotation();

  test_mat4_identity();
  test_mat4_translation();
  test_mat4_scale();
  test_mat4_rotation();
  test_mat4_multiply();
  test_mat4_transpose();
  test_mat4_inverse();
  test_mat4_determinant();
  test_mat4_projection();
  test_mat4_lookat();

  test_mat3_identity();
  test_mat3_rotation();
  test_mat3_inverse_det();
  test_mat3_2d();
  test_mat3_4x4_convert();
  test_mat3_normal_matrix();

  test_trs();
  test_decompose();
  test_euler_roundtrip();
  test_transform_point_direction();
  test_rotate_vector();

  std::cout << "\n=== Results: " << g_passed << " passed, " << g_failed
            << " failed ===" << std::endl;

  return g_failed > 0 ? 1 : 0;
}
