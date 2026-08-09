#pragma once
/**
 * @brief 二维向量
 * @namespace lCYC::math
 * @note 内存布局: 2 × float = 8 字节
 */

#include <cassert>
#include <cmath>
#include <string>

namespace lCYC::math {

struct Vector2 {
  float x;
  float y;

  // ============================ 构造 ============================

  constexpr Vector2() : x(0.0f), y(0.0f) {}
  constexpr Vector2(float x_, float y_) : x(x_), y(y_) {}
  explicit constexpr Vector2(float scalar) : x(scalar), y(scalar) {}

  // ============================ 静态常量 ============================

  static constexpr Vector2 zero() { return Vector2(0.0f, 0.0f); }
  /**
   * @brief 全 1 向量
   * @note 需要单位长度向量请用 up/down/left/right
   */
  static constexpr Vector2 one() { return Vector2(1.0f, 1.0f); }
  static constexpr Vector2 up() { return Vector2(0.0f, 1.0f); }
  static constexpr Vector2 down() { return Vector2(0.0f, -1.0f); }
  static constexpr Vector2 left() { return Vector2(-1.0f, 0.0f); }
  static constexpr Vector2 right() { return Vector2(1.0f, 0.0f); }

  // ============================ 索引访问 ============================

  constexpr float &operator[](std::size_t index) {
    assert(index < 2 && "Vector2: index out of range");
    return (&x)[index];
  }
  constexpr const float &operator[](std::size_t index) const {
    assert(index < 2 && "Vector2: index out of range");
    return (&x)[index];
  }

  // ============================ 运算符 ============================

  constexpr Vector2 operator+(const Vector2 &rhs) const {
    return Vector2(x + rhs.x, y + rhs.y);
  }
  constexpr Vector2 operator-(const Vector2 &rhs) const {
    return Vector2(x - rhs.x, y - rhs.y);
  }
  constexpr Vector2 operator*(float scalar) const {
    return Vector2(x * scalar, y * scalar);
  }
  constexpr Vector2 operator*(const Vector2 &rhs) const {
    return Vector2(x * rhs.x, y * rhs.y);
  }
  constexpr Vector2 operator/(float scalar) const {
    assert(scalar != 0.0f && "Vector2: division by zero");
    const float inv = 1.0f / scalar;
    return Vector2(x * inv, y * inv);
  }
  constexpr Vector2 operator-() const { return Vector2(-x, -y); }

  constexpr Vector2 &operator+=(const Vector2 &rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }
  constexpr Vector2 &operator-=(const Vector2 &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
  }
  constexpr Vector2 &operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    return *this;
  }
  constexpr Vector2 &operator/=(float scalar) {
    assert(scalar != 0.0f && "Vector2: division by zero");
    const float inv = 1.0f / scalar;
    x *= inv;
    y *= inv;
    return *this;
  }

  constexpr bool operator==(const Vector2 &rhs) const {
    return x == rhs.x && y == rhs.y;
  }
  constexpr bool operator!=(const Vector2 &rhs) const {
    return !(*this == rhs);
  }

  // ============================ 调试 ============================

  std::string toString() const {
    return "( " + std::to_string(x) + ", " + std::to_string(y) + " )";
  }
};

// ============================ 自由函数 ============================

/// 点积
constexpr float dot(const Vector2 &a, const Vector2 &b) {
  return a.x * b.x + a.y * b.y;
}

/// 2D 叉积
constexpr float cross(const Vector2 &a, const Vector2 &b) {
  return a.x * b.y - a.y * b.x;
}

/// 垂直向量（逆时针旋转 90°）
constexpr Vector2 perp(const Vector2 &v) { return Vector2(-v.y, v.x); }

/// 长度平方
constexpr float lengthSquared(const Vector2 &v) { return dot(v, v); }

/// 长度
constexpr float length(const Vector2 &v) { return std::sqrt(lengthSquared(v)); }

/// 两点距离
constexpr float distance(const Vector2 &a, const Vector2 &b) {
  return length(a - b);
}

/// 归一化副本
constexpr Vector2 normalized(const Vector2 &v) {
  const float lenSq = lengthSquared(v);
  if (lenSq == 0.0f || lenSq == 1.0f) {
    return v;
  }
  return v * (1.0f / std::sqrt(lenSq));
}

/// 就地归一化
inline void normalize(Vector2 &v) {
  const float lenSq = lengthSquared(v);
  if (lenSq == 0.0f || lenSq == 1.0f) {
    return;
  }
  v *= 1.0f / std::sqrt(lenSq);
}

/// 2D 旋转）
constexpr Vector2 rotate(const Vector2 &v, float angleRad) {
  const float c = std::cos(angleRad);
  const float s = std::sin(angleRad);
  return Vector2(v.x * c - v.y * s, v.x * s + v.y * c);
}

/// 钳制长度。零向量安全。maxLen < 0 按 0 处理
constexpr Vector2 clampMagnitude(const Vector2 &v, float maxLen) {
  assert(maxLen >= 0.0f && "clampMagnitude: maxLen must be non-negative");
  const float clampedMax = maxLen < 0.0f ? 0.0f : maxLen;
  const float lenSq = lengthSquared(v);
  if (lenSq > clampedMax * clampedMax) {
    return v * (clampedMax / std::sqrt(lenSq));
  }
  return v;
}

/// 两向量弧度角，[0, π]。零向量返回 0
constexpr float angle(const Vector2 &a, const Vector2 &b) {
  const float lenSqA = lengthSquared(a);
  const float lenSqB = lengthSquared(b);
  if (lenSqA == 0.0f || lenSqB == 0.0f) {
    return 0.0f;
  }
  const float cosTheta =
      std::clamp(dot(a, b) / std::sqrt(lenSqA * lenSqB), -1.0f, 1.0f);
  return std::acos(cosTheta);
}

/// 左操作数标量乘法
constexpr Vector2 operator*(float scalar, const Vector2 &v) {
  return v * scalar;
}

} // namespace lCYC::math
