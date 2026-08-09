#pragma once
/**
 * @brief 基础数学库
 * @namespace lCYC::math
 */

#include <cmath>
#include <numbers>

namespace lCYC::math {

// ============================ 常量 ============================

inline constexpr double PI = std::numbers::pi_v<double>;
inline constexpr double TWO_PI = 2.0 * PI;
inline constexpr double HALF_PI = 0.5 * PI;
inline constexpr double DEG2RAD_FACTOR = PI / 180.0;
inline constexpr double RAD2DEG_FACTOR = 180.0 / PI;

/// 浮点比较容差
inline constexpr float EPSILON = 1e-6f;

// ============================ 角度转换 ============================

/// 度 -> 弧度
constexpr double deg2rad(double deg) { return deg * DEG2RAD_FACTOR; }
constexpr float deg2rad(float deg) {
  return static_cast<float>(deg * DEG2RAD_FACTOR);
}

/// 弧度 -> 度
constexpr double rad2deg(double rad) { return rad * RAD2DEG_FACTOR; }
constexpr float rad2deg(float rad) {
  return static_cast<float>(rad * RAD2DEG_FACTOR);
}

// ============================ 标量工具 ============================

/// 浮点近似相等
/// @note 容差：float EPSILON，double  1e-12
constexpr bool approxEqual(float a, float b, float eps = EPSILON) {
  return std::abs(a - b) <= eps;
}
constexpr bool approxEqual(double a, double b, double eps = 1e-12) {
  return std::abs(a - b) <= eps;
}

/// 平滑插值因子（Hermite）
constexpr float smoothstep(float t) {
  return t <= 0.0f ? 0.0f : (t >= 1.0f ? 1.0f : t * t * (3.0f - 2.0f * t));
}

/// @brief GLSL 风格平滑插值：在 [edge0, edge1] 上归一化后做 smoothstep
/// @note \c egde0 < \c edge1
constexpr float smoothstep(float edge0, float edge1, float x) {
  const float t = (x - edge0) / (edge1 - edge0);
  return smoothstep(t);
}
constexpr double smoothstep(double edge0, double edge1, double x) {
  const double t = (x - edge0) / (edge1 - edge0);
  return t <= 0.0 ? 0.0 : (t >= 1.0 ? 1.0 : t * t * (3.0 - 2.0 * t));
}

} // namespace lCYC::math
